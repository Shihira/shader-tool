#define EXPOSE_EXCEPTION

#include "test_utils.h"

#include "common/matrix.h"
#include "common/mesh.h"
#include "render_queue.h"

using namespace shrtool;
using namespace shrtool::math;
using namespace shrtool::render_assets;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

const char* vs_gbuffer_pass = R"EOF(
#version 330 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;

layout (std140) uniform camera {
    mat4 vMatrix;
    mat4 vMatrix_inv;
    mat4 vpMatrix;
};

layout (std140) uniform extra {
    mat4 mMatrix;
    int row_count;
};

#define glossiness x
#define metallic y

out vec3 fragNormal;
out float depth;
out vec2 material;
out vec3 baseColor;

void main() {
    int x = gl_InstanceID / row_count;
    int z = gl_InstanceID % row_count;

    mat4 m = mMatrix;
    m[3].x = x * 3;
    m[3].z = z * 3;

    fragNormal = (transpose(inverse(m)) * vec4(normal, 0)).xyz;
    gl_Position = vpMatrix * m * position;
    depth = gl_Position.z / abs(gl_Position.w);

    material.glossiness = float(x) / (row_count - 1) * 0.6 + 0.2;
    material.metallic = float(z) / (row_count - 1) * 0.8 + 0.1;

    baseColor = vec3(1.00, 0.71, 0.29);
}
)EOF";

const char* fs_gbuffer_pass = R"EOF(
#version 330 core

in vec3 fragNormal;
in float depth;
in vec2 material;
in vec3 baseColor;

layout (location = 0) out vec4 gbuffer_1;
layout (location = 1) out vec4 gbuffer_2;

void main() {
    gbuffer_1.xy = normalize(fragNormal).xy;
    gbuffer_1.z = depth;
    gbuffer_1.w = material.x;
    gbuffer_2.x = material.y;
    gbuffer_2.yzw = baseColor;

    if(fragNormal.z < 0)
        gbuffer_1.w = -gbuffer_1.w;
}
)EOF";

////////////////////////////////////////////////////////////////////////////////

const char* vs_final_pass = R"EOF(
#version 330 core

layout (location = 0) in vec4 position;

out vec2 uv;

void main() {
    gl_Position = vec4(position.x, position.z, 0, 1);
    uv = vec2(position.x, position.z);
}
)EOF";

const char* fs_final_pass_common = R"EOF(
#version 330 core

layout (std140) uniform illum {
    vec4 lightPosition;
    vec4 lightColor;
    vec3 illumFunc;
};

layout (std140) uniform camera {
    mat4 vMatrix;
    mat4 vMatrix_inv;
    mat4 vpMatrix;
};

uniform sampler2D gbuffer_1;
uniform sampler2D gbuffer_2;

in vec2 uv;
out vec4 outColor;

#define PI 3.14156265359

float ranged(vec4 pos, float strength) {
    float d = length(pos - lightPosition);
    return strength / (illumFunc.x + illumFunc.y * d + illumFunc.z * d * d);
}

vec4 ranged(vec4 pos, vec4 strength) {
    float d = length(pos - lightPosition);
    float m = 1.0 / (illumFunc.x + illumFunc.y * d + illumFunc.z * d * d);

    return m * strength;
}

struct geometryProperties {
    vec3 normal;
    vec4 position;
    float metallic;
    float glossiness;
    vec4 baseColor;
};

bool extractProperties(out geometryProperties p) {
    vec4 gfrag = texture(gbuffer_1, uv / 2 + 0.5);
    vec4 mfrag = texture(gbuffer_2, uv / 2 + 0.5);

    p.glossiness = gfrag.w;
    p.metallic = mfrag.x;
    p.normal = vec3(gfrag.xy, 0);
    p.baseColor = vec4(mfrag.yzw, 1);
    p.normal.z = sqrt(1 - p.normal.x * p.normal.x - p.normal.y * p.normal.y);

    if(gfrag.w < 0) {
        p.normal.z = -p.normal.z;
        p.glossiness = -p.glossiness;
    }

    if(gfrag.z == 2) {
        return false;
    }

    p.position = vec4(uv.xy, gfrag.z, 1);
    p.position = inverse(vpMatrix) * p.position;
    p.position = p.position / p.position.w;

    return true;
}
)EOF";

const char* fs_final_pass = R"EOF(
void main() {
    geometryProperties p;
    if(!extractProperties(p)) {
        outColor = vec4(0, 0, 0, 0);
        return;
    }

    vec4 cameraPosition = vMatrix_inv[3];
    cameraPosition /= cameraPosition.w;

    vec3 lightDir = normalize((p.position - lightPosition).xyz);
    vec3 viewDir = normalize((p.position - cameraPosition).xyz);

    float intDiffuse = dot(-p.normal, lightDir);
    float intSpecular = dot(-p.normal, normalize(lightDir + viewDir));

    intDiffuse = ranged(p.position, max(intDiffuse, 0));
    intSpecular = pow(intSpecular, max(intSpecular, 24));
    intSpecular = ranged(p.position, intSpecular);

    vec4 lightDiffuse = lightColor * intDiffuse;
    vec4 lightSpecular = lightColor * intSpecular;

    outColor = lightDiffuse + lightSpecular;
}
)EOF";

TEST_CASE_FIXTURE(test_deferred_shading, singlefunc_fixture) {
    size_t sqr = 8;

    render_target::screen.set_depth_test(false);
    render_target::screen.set_bgcolor(fcolor(0.1, 0.1, 0.1, 1));

    shader gbuffer_pass;
    gbuffer_pass.add_sub_shader(shader::VERTEX).compile(vs_gbuffer_pass);
    gbuffer_pass.add_sub_shader(shader::FRAGMENT).compile(fs_gbuffer_pass);
    gbuffer_pass.link();

    shader final_pass;
    final_pass.add_sub_shader(shader::VERTEX).compile(vs_final_pass);
    final_pass.add_sub_shader(shader::FRAGMENT).compile(string(fs_final_pass_common) + fs_final_pass);
    final_pass.link();

    mesh_uv_sphere mesh(1, 32, 16);
    mesh_plane tex_plane(2, 2, 1, 1);

    const rect& vp = render_target::screen.get_viewport();
    texture2d gbuffer_1(vp.width(), vp.height(), texture::RGBA_F32);
    texture2d gbuffer_2(vp.width(), vp.height(), texture::RGBA_F32);
    texture2d depth_tex(vp.width(), vp.height(), texture::DEPTH_F32);

    camera gbuffer_cam;
    gbuffer_cam.set_visible_angle(PI / 3);
    gbuffer_cam.transformation()
        //.translate((sqr - 1) * 3 / 2, 0, sqr * 3 * 1.2)
        .translate((sqr - 1) * 3 / 2.0, double(sqr) * -1.2, sqr * 3 * 1.2)
        .rotate(PI / 3, tf::yOz);
    gbuffer_cam.set_depth_test(true);
    gbuffer_cam.set_bgcolor(fcolor(2, 2, 2, 2));
    gbuffer_cam.attach_texture(render_target::COLOR_BUFFER_0, gbuffer_1);
    gbuffer_cam.attach_texture(render_target::COLOR_BUFFER_1, gbuffer_2);
    gbuffer_cam.attach_texture(render_target::DEPTH_BUFFER, depth_tex);

    provided_render_task::provider_bindings bindings;
    provided_render_task gbuffer_rtask(bindings);
    provided_render_task final_rtask(bindings);

    universal_property<fmat4, int> extra {
        tf::identity(), int(sqr),
    };

    gbuffer_rtask.set_shader(gbuffer_pass);
    gbuffer_rtask.set_target(gbuffer_cam);
    gbuffer_rtask.set_attributes(mesh);
    gbuffer_rtask.set_property("camera", gbuffer_cam);
    gbuffer_rtask.set_property("extra", extra);
    gbuffer_rtask.set_render_count(sqr * sqr);

    final_rtask.set_shader(final_pass);
    final_rtask.set_target(render_target::screen);
    final_rtask.set_attributes(tex_plane);
    final_rtask.set_property("camera", gbuffer_cam);
    final_rtask.set_texture_property("gbuffer_1", gbuffer_1);
    final_rtask.set_texture_property("gbuffer_2", gbuffer_2);

    render_target::screen.set_blend_func(render_target::PLUS_BLEND);

    std::vector<universal_property<fcol4, fcol4, fcol3>> illum(sqr * sqr);

    for(size_t i = 0; i < sqr; i++)
    for(size_t j = 0; j < sqr; j++) {
        size_t idx = i * sqr + j;
        size_t seed = i * j + idx * 1779 + 1857;

        item_get<0>(!illum[idx]) = fcol4 { 3.0f * i, 1.2, 3.0f * j, 1 };
        item_get<1>(!illum[idx]) = fcol4 { seed % 17 / 17.0f, seed % 23 / 23.0f, seed % 47 / 47.0f, 1 };
        item_get<2>(!illum[idx]) = fcol3 { 1, 0.35, 0.44 };
    }

    draw = [&]() {
        gbuffer_cam.clear_buffer(render_target::COLOR_BUFFER);
        gbuffer_cam.clear_buffer(render_target::DEPTH_BUFFER);

        render_target::screen.clear_buffer(render_target::COLOR_BUFFER);
        render_target::screen.clear_buffer(render_target::DEPTH_BUFFER);

        gbuffer_rtask.render();

        for(int i = 0; i < sqr * sqr; i++) {
            final_rtask.set_property("illum", illum[i]);
            final_rtask.render();
        }
    };

    camera_helper ch(this, &gbuffer_cam);
    ch.rotate_axis = col3 { 3.0 * sqr / 2 - 1.5, 0, 3.0 * sqr / 2 - 1.5};

    main_loop();
}

////////////////////////////////////////////////////////////////////////////////

const char* fs_pbr_pass = R"EOF(
float clamp01(float v) {
    return clamp(v, 0, 1);
}

float dwc(vec3 x, vec3 y) {
    return clamp01(dot(x, y));
}

float sq(float f) { return f * f; }

float g1(vec3 normal, vec3 x, float k) {
    float nx = dwc(normal, x);
    return nx / (nx * (1 - k) + k);
}

void main() {
    geometryProperties p;
    if(!extractProperties(p)) {
        outColor = vec4(0, 0, 0, 0);
        return;
    }

    vec4 cameraPosition = vMatrix_inv[3];
    cameraPosition /= cameraPosition.w;

    vec3 lightDir = -normalize((p.position - lightPosition).xyz);
    vec3 viewDir = -normalize((p.position - cameraPosition).xyz);

    vec3 halfVec = normalize(lightDir + viewDir);

    ////////////////////////////////////////////////////////////////////////////

    vec4 recieved = PI * dwc(lightDir, p.normal) * lightColor;
    recieved = ranged(p.position, recieved);
    vec4 albedoColor = (1 - p.metallic) * p.baseColor;
    vec4 specularColor = p.metallic * p.baseColor + (1 - p.metallic) * vec4(0.04, 0.04, 0.04, 1);

    float a = (1 - p.glossiness) * (1 - p.glossiness);
    float distribution = a * a / (PI * sq(sq(dwc(p.normal, halfVec)) * (a * a - 1) + 1));

    float k = sq(2 - p.glossiness) / 8;
    float geometry = g1(p.normal, lightDir, k) * g1(p.normal, viewDir, k);

    vec4 fresnel = specularColor + pow(1 - dwc(lightDir, halfVec), 5) * (vec4(1, 1, 1, 1) - specularColor);

    vec4 microfacetColor =
        fresnel * geometry * distribution /
        (4 * dwc(p.normal, lightDir) * dwc(p.normal, viewDir));

    outColor = (albedoColor / PI + microfacetColor) * recieved;
}
)EOF";

TEST_CASE_FIXTURE(test_pbr, singlefunc_fixture) {
    size_t sqr = 8;

    render_target::screen.set_depth_test(false);
    render_target::screen.set_bgcolor(fcolor(0.1, 0.1, 0.1, 1));

    shader gbuffer_pass;
    gbuffer_pass.add_sub_shader(shader::VERTEX).compile(vs_gbuffer_pass);
    gbuffer_pass.add_sub_shader(shader::FRAGMENT).compile(fs_gbuffer_pass);
    gbuffer_pass.link();

    shader pbr_pass;
    pbr_pass.add_sub_shader(shader::VERTEX).compile(vs_final_pass);
    pbr_pass.add_sub_shader(shader::FRAGMENT).compile(string(fs_final_pass_common) + fs_pbr_pass);
    pbr_pass.link();

    mesh_uv_sphere mesh(1, 32, 16);
    mesh_plane tex_plane(2, 2, 1, 1);

    const rect& vp = render_target::screen.get_viewport();
    texture2d gbuffer_1(vp.width(), vp.height(), texture::RGBA_F32);
    texture2d gbuffer_2(vp.width(), vp.height(), texture::RGBA_F32);
    texture2d depth_tex(vp.width(), vp.height(), texture::DEPTH_F32);

    camera gbuffer_cam;
    gbuffer_cam.set_visible_angle(PI / 3);
    gbuffer_cam.transformation()
        //.translate((sqr - 1) * 3 / 2, 0, sqr * 3 * 1.2)
        .translate((sqr - 1) * 3 / 2.0, double(sqr) * -1.2, sqr * 3 * 1.2)
        .rotate(PI / 3, tf::yOz);
    gbuffer_cam.set_depth_test(true);
    gbuffer_cam.set_bgcolor(fcolor(2, 2, 2, 2));
    gbuffer_cam.attach_texture(render_target::COLOR_BUFFER_0, gbuffer_1);
    gbuffer_cam.attach_texture(render_target::COLOR_BUFFER_1, gbuffer_2);
    gbuffer_cam.attach_texture(render_target::DEPTH_BUFFER, depth_tex);

    provided_render_task::provider_bindings bindings;
    provided_render_task gbuffer_rtask(bindings);
    provided_render_task pbr_rtask(bindings);

    universal_property<fmat4, int> extra {
        tf::identity(), int(sqr),
    };

    gbuffer_rtask.set_shader(gbuffer_pass);
    gbuffer_rtask.set_target(gbuffer_cam);
    gbuffer_rtask.set_attributes(mesh);
    gbuffer_rtask.set_property("camera", gbuffer_cam);
    gbuffer_rtask.set_property("extra", extra);
    gbuffer_rtask.set_render_count(sqr * sqr);

    pbr_rtask.set_shader(pbr_pass);
    pbr_rtask.set_target(render_target::screen);
    pbr_rtask.set_attributes(tex_plane);
    pbr_rtask.set_property("camera", gbuffer_cam);
    pbr_rtask.set_texture_property("gbuffer_1", gbuffer_1);
    pbr_rtask.set_texture_property("gbuffer_2", gbuffer_2);

    render_target::screen.set_blend_func(render_target::PLUS_BLEND);

    std::vector<universal_property<fcol4, fcol4, fcol3>> illum(3);

    for(size_t i = 0; i < illum.size(); i++) {
        int x = ((i * 1143 + 2931) % 57 - 57/2 + sqr/2) * 3;
        int z = ((i * 2931 + 1143) % 57 - 57/2 + sqr/2) * 3;

        cout << x << ',' << z << endl;

        item_get<0>(!illum[i]) = fcol4 { float(x), 5, float(z), 1 };
        item_get<1>(!illum[i]) = fcol4 { 1, 1, 1, 1 };
        item_get<2>(!illum[i]) = fcol3 { 1, 0.0075, 0.00125 };
    }

    draw = [&]() {
        gbuffer_cam.clear_buffer(render_target::COLOR_BUFFER);
        gbuffer_cam.clear_buffer(render_target::DEPTH_BUFFER);

        render_target::screen.clear_buffer(render_target::COLOR_BUFFER);
        render_target::screen.clear_buffer(render_target::DEPTH_BUFFER);

        gbuffer_rtask.render();

        for(int i = 0; i < illum.size(); i++) {
            pbr_rtask.set_property("illum", illum[i]);
            pbr_rtask.render();
        }
    };

    camera_helper ch(this, &gbuffer_cam);
    ch.rotate_axis = col3 { 3.0 * sqr / 2 - 1.5, 0, 3.0 * sqr / 2 - 1.5};

    main_loop();
}

////////////////////////////////////////////////////////////////////////////////

const char* fs_ibl_pass = R"EOF(
uniform samplerCube envMap;

void main() {
    geometryProperties p;
    if(!extractProperties(p)) {
        discard;
    }

    vec4 cameraPosition = vMatrix_inv[3];
    cameraPosition /= cameraPosition.w;

    vec3 lightDir = -normalize((p.position - lightPosition).xyz);
    vec3 viewDir = -normalize((p.position - cameraPosition).xyz);

    vec3 halfVec = normalize(lightDir + viewDir);
    vec4 envColor = texture(envMap, reflect(-viewDir, p.normal));

    ////////////////////////////////////////////////////////////////////////////

    vec4 specularColor =
        (p.metallic * p.baseColor +
        (1 - p.metallic) * vec4(0.04, 0.04, 0.04, 1)) * envColor;

    outColor = specularColor;
}
)EOF";

TEST_CASE_FIXTURE(test_ibl, singlefunc_fixture) {
    size_t sqr = 8;

    render_target::screen.set_depth_test(false);
    render_target::screen.set_bgcolor(fcolor(0.1, 0.1, 0.1, 1));

    shader gbuffer_pass;
    gbuffer_pass.add_sub_shader(shader::VERTEX).compile(vs_gbuffer_pass);
    gbuffer_pass.add_sub_shader(shader::FRAGMENT).compile(fs_gbuffer_pass);
    gbuffer_pass.link();

    shader pbr_pass;
    pbr_pass.add_sub_shader(shader::VERTEX).compile(vs_final_pass);
    pbr_pass.add_sub_shader(shader::FRAGMENT).compile(string(fs_final_pass_common) + fs_pbr_pass);
    pbr_pass.link();

    shader ibl_pass;
    ibl_pass.add_sub_shader(shader::VERTEX).compile(vs_final_pass);
    ibl_pass.add_sub_shader(shader::FRAGMENT).compile(string(fs_final_pass_common) + fs_ibl_pass);
    ibl_pass.link();

    ifstream env_map_fs("../assets/textures/environ-map.ppm");
    image env_map = image::load_cubemap_from(image_io_netpbm::load(env_map_fs));

    mesh_uv_sphere mesh(1, 32, 16);
    mesh_plane tex_plane(2, 2, 1, 1);

    const rect& vp = render_target::screen.get_viewport();
    texture2d gbuffer_1(vp.width(), vp.height(), texture::RGBA_F32);
    texture2d gbuffer_2(vp.width(), vp.height(), texture::RGBA_F32);
    texture2d depth_tex(vp.width(), vp.height(), texture::DEPTH_F32);

    camera gbuffer_cam;
    gbuffer_cam.set_visible_angle(PI / 3);
    gbuffer_cam.transformation()
        //.translate((sqr - 1) * 3 / 2, 0, sqr * 3 * 1.2)
        .translate((sqr - 1) * 3 / 2.0, double(sqr) * -1.2, sqr * 3 * 1.2)
        .rotate(PI / 3, tf::yOz);
    gbuffer_cam.set_depth_test(true);
    gbuffer_cam.set_bgcolor(fcolor(2, 2, 2, 2));
    gbuffer_cam.attach_texture(render_target::COLOR_BUFFER_0, gbuffer_1);
    gbuffer_cam.attach_texture(render_target::COLOR_BUFFER_1, gbuffer_2);
    gbuffer_cam.attach_texture(render_target::DEPTH_BUFFER, depth_tex);

    provided_render_task::provider_bindings bindings;
    provided_render_task gbuffer_rtask(bindings);
    provided_render_task ibl_rtask(bindings);
    provided_render_task pbr_rtask(bindings);

    universal_property<fmat4, int> extra {
        tf::identity(), int(sqr),
    };

    gbuffer_rtask.set_shader(gbuffer_pass);
    gbuffer_rtask.set_target(gbuffer_cam);
    gbuffer_rtask.set_attributes(mesh);
    gbuffer_rtask.set_property("camera", gbuffer_cam);
    gbuffer_rtask.set_property("extra", extra);
    gbuffer_rtask.set_render_count(sqr * sqr);

    ibl_rtask.set_shader(ibl_pass);
    ibl_rtask.set_target(render_target::screen);
    ibl_rtask.set_attributes(tex_plane);
    ibl_rtask.set_property("camera", gbuffer_cam);
    ibl_rtask.set_texture_property("gbuffer_1", gbuffer_1);
    ibl_rtask.set_texture_property("gbuffer_2", gbuffer_2);
    ibl_rtask.set_texture_property<texture_cubemap>("envMap", env_map);

    pbr_rtask.set_shader(pbr_pass);
    pbr_rtask.set_target(render_target::screen);
    pbr_rtask.set_attributes(tex_plane);
    pbr_rtask.set_property("camera", gbuffer_cam);
    pbr_rtask.set_texture_property("gbuffer_1", gbuffer_1);
    pbr_rtask.set_texture_property("gbuffer_2", gbuffer_2);

    render_target::screen.set_blend_func(render_target::PLUS_BLEND);

    std::vector<universal_property<fcol4, fcol4, fcol3>> illum(3);

    for(size_t i = 0; i < illum.size(); i++) {
        int x = ((i * 1143 + 2931) % 57 - 57/2 + sqr/2) * 3;
        int z = ((i * 2931 + 1143) % 57 - 57/2 + sqr/2) * 3;

        cout << x << ',' << z << endl;

        item_get<0>(!illum[i]) = fcol4 { float(x), 5, float(z), 1 };
        item_get<1>(!illum[i]) = fcol4 { 1, 1, 1, 1 };
        item_get<2>(!illum[i]) = fcol3 { 1, 0.0075, 0.00125 };
    }

    draw = [&]() {
        gbuffer_cam.clear_buffer(render_target::COLOR_BUFFER);
        gbuffer_cam.clear_buffer(render_target::DEPTH_BUFFER);

        render_target::screen.clear_buffer(render_target::COLOR_BUFFER);
        render_target::screen.clear_buffer(render_target::DEPTH_BUFFER);

        gbuffer_rtask.render();

        ibl_rtask.render();
        for(int i = 0; i < illum.size(); i++) {
            pbr_rtask.set_property("illum", illum[i]);
            pbr_rtask.render();
        }
    };

    camera_helper ch(this, &gbuffer_cam);
    ch.rotate_axis = col3 { 3.0 * sqr / 2 - 1.5, 0, 3.0 * sqr / 2 - 1.5};

    main_loop();
}

int main(int argc, char* argv[])
{
    gui_test_context::init();
    return unit_test::test_main(argc, argv);
}
