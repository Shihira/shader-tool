#include "gui_test.h"
#include "matrix.h"

using namespace shrtool;
using namespace shrtool::math;
using namespace shrtool::render_assets;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_FIXTURE(test_rectangle, singlefunc_fixture) {
    render_target::screen.initial_color(0.2, 0.2, 0.2, 1);

    shader shr;
    sub_shader& vs = shr.add_sub_shader(shader::VERTEX);
    sub_shader& fs = shr.add_sub_shader(shader::FRAGMENT);

    vs.compile(R"EOF(
    #version 330 core

    layout (location = 0) in vec4 position;

    void main() {
        gl_Position = position;
    }
    )EOF");

    fs.compile(R"EOF(
    #version 330 core

    layout (std140) uniform material {
        vec4 color;
    };

    out vec4 outColor;

    void main() {
        outColor = color;
    }
    )EOF");

    shr.link();

    float prop_color_data[4] = { 1, 0, 0, 1 };
    float attr_position_data[4 * 6] = {
         0.5,  0.5, 0, 1,
        -0.5,  0.5, 0, 1,
        -0.5, -0.5, 0, 1,

        -0.5, -0.5, 0, 1,
         0.5, -0.5, 0, 1,
         0.5,  0.5, 0, 1,
    };


    property_buffer prop_color;
    vertex_attr_buffer attr_position;
    vertex_attr_vector attr_vec;
    bool incDirection;

    // Write buffers
    attr_position.write(attr_position_data, sizeof(attr_position_data));

    // Set buffers
    shr.property("material", prop_color);
    attr_vec.primitives_count(6);
    attr_vec.input(0, attr_position);

    incDirection = true;

    update = [&]() {
        if(prop_color_data[1] >= 1 && incDirection) incDirection = false;
        if(prop_color_data[1] <= 0 && !incDirection) incDirection = true;

        prop_color_data[1] += incDirection ? 0.01 : -0.01;
        prop_color.write(prop_color_data, sizeof(prop_color_data));
    };

    draw = [&]() {
        render_target::screen.clear_buffer(render_target::COLOR_BUFFER);
        shr.draw(attr_vec);
    };

    main_loop();
}

////////////////////////////////////////////////////////////////////////////////

void copy_matrix_to_buffer(const mat4& mat, void* buf_)
{
    float* buf = static_cast<float*>(buf_);
    for(size_t i = 0; i < 4; i++) {
        for(size_t j = 0; j < 4; j++) {
            buf[j * 4 + i] = mat[i][j];
        }
    }
}

TEST_CASE_FIXTURE(test_cube, singlefunc_fixture) {
    render_target::screen.initial_color(0.2, 0.2, 0.2, 1);
    render_target::screen.enable_depth_test(true);

    shader shr;
    sub_shader& vs = shr.add_sub_shader(shader::VERTEX);
    sub_shader& fs = shr.add_sub_shader(shader::FRAGMENT);

    vs.compile(R"EOF(
    #version 330 core

    layout (location = 0) in vec4 position;
    layout (location = 1) in vec3 normal;
    layout (location = 2) in vec3 uv;

    layout (std140) uniform mvp {
        mat4 mvpMatrix;
        mat4 mMatrix;
    };

    out vec4 fragPosition;
    out vec3 fragNormal;

    void main() {
        fragNormal = (transpose(inverse(mMatrix)) * vec4(normal, 0)).xyz;

        gl_Position = mvpMatrix * position;
        fragPosition = mMatrix * position;
        fragPosition /= fragPosition.w;
    }
    )EOF");

    fs.compile(R"EOF(
    #version 330 core

    layout (std140) uniform illum {
        vec4 lightPosition;
        vec4 lightColor;
        vec4 cameraPosition;
    };

    layout (std140) uniform material {
        vec4 diffuseColor;
        vec4 ambientColor;
        vec4 specularColor;
    };

    in vec4 fragPosition;
    in vec3 fragNormal;

    out vec4 outColor;

    void main() {
        vec3 lightDir = normalize((fragPosition - lightPosition).xyz);
        vec3 viewDir = normalize((fragPosition - cameraPosition).xyz);

        float intDiffuse = 1;
        float intAmbient = dot(-fragNormal, lightDir);
        float intSpecular = dot(-fragNormal, normalize(lightDir + viewDir));

        intAmbient = max(intAmbient, 0);
        intSpecular = pow(intSpecular, 3);
        intSpecular = max(intSpecular, 0);

        outColor =
            diffuseColor * intDiffuse +
            ambientColor * intAmbient +
            specularColor * intSpecular;
    }
    )EOF");

    shr.link();

    const float vertices[4 * 8] = {
         1,  1,  1, 1,
         1,  1, -1, 1,
         1, -1,  1, 1,
         1, -1, -1, 1,
        -1,  1,  1, 1,
        -1,  1, -1, 1,
        -1, -1,  1, 1,
        -1, -1, -1, 1,
    };

    const float normals[3 * 6] = {
        1, 0, 0,
        -1, 0, 0,
        0, 1, 0,
        0, -1, 0,
        0, 0, 1,
        0, 0, -1,
    };

    const float uvs[2 * 4] = {
        0, 0,
        0, 1,
        1, 0,
        1, 1,
    };

    static constexpr size_t num_vertices = 3 * 2 * 6;

    // 0 1 3 2
    // 4 5 7 6
    const size_t vindices[num_vertices] = {
        2, 3, 1,    1, 0, 2,
        4, 5, 7,    7, 6, 4,
        1, 5, 4,    4, 0, 1,
        2, 6, 7,    7, 3, 2,
        0, 4, 6,    6, 2, 0,
        3, 7, 5,    5, 1, 3,
    };

    const size_t nindices[num_vertices] = {
        0, 0, 0,    0, 0, 0,
        1, 1, 1,    1, 1, 1,
        2, 2, 2,    2, 2, 2,
        3, 3, 3,    3, 3, 3,
        4, 4, 4,    4, 4, 4,
        5, 5, 5,    5, 5, 5,
    };

    const size_t uindices[num_vertices] = {
        0, 1, 3,    3, 2, 0,
        0, 1, 3,    3, 2, 0,
        0, 1, 3,    3, 2, 0,
        0, 1, 3,    3, 2, 0,
        0, 1, 3,    3, 2, 0,
        0, 1, 3,    3, 2, 0,
    };

    vertex_attr_vector attr_vec;

    vertex_attr_buffer attr_position;
    vertex_attr_buffer attr_normal;
    vertex_attr_buffer attr_uv;
    property_buffer prop_mvp;
    property_buffer prop_material;
    property_buffer prop_illum;

    float attr_position_data[4 * num_vertices];
    float attr_normal_data[3 * num_vertices];
    float attr_uv_data[3 * num_vertices];
    float prop_mvp_data[16 * 2];
    float prop_material_data[12] = {
        0.1f, 0.1f, 0.13f, 1,
        0.7f, 0.7f, 0.7f, 1,
        0.2f, 0.2f, 0.5f, 1,
    };
    float prop_illum_data[12] = {
        0, 3, 4, 1,
        1, 1, 1, 1,
        0, 1, 5, 1
    };

    mat4 model_mat = tf::identity();
    mat4 view_mat = inverse(
            tf::rotate(M_PI / 8, tf::yOz) *
            tf::translate(col4 {
                prop_illum_data[8 + 0],
                prop_illum_data[8 + 1],
                prop_illum_data[8 + 2],
                prop_illum_data[8 + 3],
            }));
    mat4 proj_mat = tf::perspective(M_PI / 4, 4.0 / 3, 1, 100);

    // fill attributes
    for(size_t i = 0; i < num_vertices; i++) {
        for(size_t vi = 0; vi < 4; vi++)
            attr_position_data[i * 4 + vi] = vertices[vindices[i] * 4 + vi];
        for(size_t vi = 0; vi < 3; vi++)
            attr_normal_data[i * 3 + vi] = normals[nindices[i] * 3 + vi];
        for(size_t vi = 0; vi < 3; vi++)
            attr_uv_data[i * 3 + vi ] = uvs[uindices[i] * 3 + vi];
    }

    attr_position.write(attr_position_data, sizeof(attr_position_data));
    attr_normal.write(attr_normal_data, sizeof(attr_normal_data));
    attr_uv.write(attr_uv_data, sizeof(attr_uv_data));
    prop_material.write(prop_material_data, sizeof(prop_material_data));
    prop_illum.write(prop_illum_data, sizeof(prop_illum_data));

    attr_vec.primitives_count(num_vertices);
    attr_vec.input(0, attr_position);
    attr_vec.input(1, attr_normal);
    attr_vec.input(2, attr_uv);

    shr.property("mvp", prop_mvp);
    shr.property("illum", prop_illum);
    shr.property("material", prop_material);

    update = [&]() {
        model_mat *= tf::rotate(M_PI / 120, tf::zOx);

        copy_matrix_to_buffer(model_mat, prop_mvp_data + 16);
        copy_matrix_to_buffer(proj_mat * view_mat * model_mat, prop_mvp_data);
        prop_mvp.write(prop_mvp_data, sizeof(prop_mvp_data));
    };

    draw = [&]() {
        render_target::screen.clear_buffer(render_target::COLOR_BUFFER);
        render_target::screen.clear_buffer(render_target::DEPTH_BUFFER);

        shr.draw(attr_vec);
    };

    main_loop();
}

int main(int argc, char* argv[])
{
    gui_test_context::init();
    return unit_test::test_main(argc, argv);
}
