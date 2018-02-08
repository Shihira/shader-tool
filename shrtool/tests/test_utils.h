#ifndef TEST_UTILS_INCLUDED 
#define TEST_UTILS_INCLUDED 

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <fstream>
#include <chrono>

#include "common/unit_test.h"
#include "common/utilities.h"
#include "common/singleton.h"
#include "common/exception.h"
#include "shading.h"

namespace shrtool {

#ifndef NO_GUI_TEST

struct gui_fixture_base;

class gui_test_context : public generic_singleton<gui_test_context>
{
public:
    std::string title;

    GLFWwindow* window = nullptr;
    gui_fixture_base* active_test = nullptr;

    static inline void key_callback(GLFWwindow* window,
            int key, int scancode, int action, int mods);

    static inline void mouse_callback(GLFWwindow* window,
            int button, int action, int mods);

    static inline void mouse_move_callback(GLFWwindow* window,
            double xpos, double ypos);

    static inline void scroll_callback(GLFWwindow* window,
            double xpos, double ypos);

    static void fbsize_callback(GLFWwindow* window,
            int width, int height) {
        render_target::screen.set_viewport(rect::from_size(width, height));
    }

    static void init(const std::string ver_str = "330 core",
            std::string title_ = "Test", size_t w = 800, size_t h = 600) {
        inst().title = title_;

        unsigned state = glfwInit();
        if(!state) throw driver_error("Failed to init GLFW");

        std::stringstream ss(ver_str);
        unsigned ver; std::string profile_str;
        ss >> ver >> profile_str;
        unsigned ver_1 = ver / 100, ver_2 = ver % 100 / 10,
                 prof = profile_str == "core" ?
                     GLFW_OPENGL_CORE_PROFILE : GLFW_OPENGL_ANY_PROFILE;

        if(inst().title.empty()) glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, ver_1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, ver_2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, prof);

        inst().window = glfwCreateWindow(w, h, inst().title.c_str(), NULL, NULL);

        glfwMakeContextCurrent(inst().window);

        glewExperimental = GL_TRUE;
        state = glewInit();
        if(state != GLEW_OK)
            throw driver_error("Failed to init " + ver_str + ": " +
                    (const char* const)glewGetErrorString(state));

        glfwSetKeyCallback(inst().window, key_callback);
        glfwSetMouseButtonCallback(inst().window, mouse_callback);
        glfwSetCursorPosCallback(inst().window, mouse_move_callback);
        glfwSetFramebufferSizeCallback(inst().window, fbsize_callback);
        glfwSetScrollCallback(inst().window, scroll_callback);

        render_target::screen.set_viewport(rect::from_size(w, h));
    }

    static void geometry_source(size_t& w, size_t& h) {
        int iw, ih;
        glfwGetWindowSize(inst().window, &iw, &ih);
        w = iw; h = ih;
    }

    ~gui_test_context() {
        glfwTerminate();
    }

};

struct gui_fixture_base
{
private:
public:
    gui_fixture_base() {
        gui_test_context::inst().active_test = this;
    }

    ~gui_fixture_base() {
        gui_test_context::inst().active_test = nullptr;
    }

    virtual void do_update() { }
    virtual void do_draw() = 0;
    virtual void do_keypress(int key, int modifiers) { }
    virtual void do_keyrelease(int key, int modifiers) { }
    virtual void do_mousedown(int key, int x, int y) { }
    virtual void do_mouseup(int key, int x, int y) { }
    virtual void do_mousemove(int x, int y) { }
    virtual void do_scroll(int x, int y) { }
    virtual void operator() () = 0;

    enum { PENDING, RUNNING, FINISHED } state = PENDING;

    void main_loop() {
        int fps = 0;
        auto last_count = now();

        state = RUNNING;
        while(state != FINISHED &&
                !glfwWindowShouldClose(gui_test_context::inst().window)) {
            do_update();
            do_draw();
            glfwPollEvents();
            glfwSwapBuffers(gui_test_context::inst().window);

            fps += 1;

            if(now() - last_count >= 1) {
                glfwSetWindowTitle(gui_test_context::inst().window,
                    (gui_test_context::inst().title + " (fps: " +
                    std::to_string(fps) + ")").c_str());

                fps = 0;
                last_count = now();
            }
        }
    }

    double now() { return glfwGetTime(); }
};

void gui_test_context::key_callback(GLFWwindow* window,
        int key, int scancode, int action, int mods) {
    if(!gui_test_context::inst().active_test)
        glfwTerminate();

    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        inst().active_test->state = gui_fixture_base::FINISHED;
    }

    if(action == GLFW_PRESS)
        inst().active_test->do_keypress(key, mods);
    else if(action == GLFW_RELEASE)
        inst().active_test->do_keyrelease(key, mods);
}

void gui_test_context::mouse_callback(GLFWwindow* window,
        int button, int action, int mods) {
    if(!gui_test_context::inst().active_test)
        glfwTerminate();

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if(action == GLFW_PRESS)
        inst().active_test->do_mousedown(button, x, y);
    else if(action == GLFW_RELEASE)
        inst().active_test->do_mouseup(button, x, y);
}

void gui_test_context::mouse_move_callback(GLFWwindow* window,
        double xpos, double ypos) {
    if(!gui_test_context::inst().active_test)
        glfwTerminate();

    inst().active_test->do_mousemove(xpos, ypos);

}

void gui_test_context::scroll_callback(GLFWwindow* window,
        double xpos, double ypos) {
    if(!gui_test_context::inst().active_test)
        glfwTerminate();

    inst().active_test->do_scroll(xpos, ypos);

}

struct singlefunc_fixture : gui_fixture_base
{
    std::function<void()> update;
    std::function<void()> draw;
    std::function<void(int, int)> keypress;
    std::function<void(int, int)> keyrelease;
    std::function<void(int, int, int)> mousedown;
    std::function<void(int, int, int)> mouseup;
    std::function<void(int, int)> mousemove;
    std::function<void(int, int)> wheelscroll;

    void do_update() override { if(update) update(); }
    void do_draw() override { if(draw) draw(); }
    void do_keypress(int key, int modifiers) override
        { if(keypress) keypress(key, modifiers); }
    void do_keyrelease(int key, int modifiers) override
        { if(keyrelease) keyrelease(key, modifiers); }
    void do_mousedown(int key, int x, int y) override
        { if(mousedown) mousedown(key, x, y); }
    void do_mouseup(int key, int x, int y) override
        { if(mouseup) mouseup(key, x, y); }
    void do_mousemove(int x, int y) override
        { if(mousemove) mousemove(x, y); }
    void do_scroll(int x, int y) override
        { if(wheelscroll) wheelscroll(x, y); }
};

struct camera_helper {
    camera* cam;
    singlefunc_fixture* fix;

    math::col3 rotate_axis;

    int down_btn = -1;
    transfrm init_tf;
    math::col3 init_ra;
    int init_x = -1;
    int init_y = -1;

    camera_helper(singlefunc_fixture* fix_, camera* cam_) {
        using namespace shrtool::math;

        cam = cam_;
        fix = fix_;

        fix_->mouseup = [this](int button, int x, int y) {
            down_btn = -1;
            init_x = -1;
            init_y = -1;
        };

        fix_->mousedown = [this](int button, int x, int y) {
            if(down_btn < 0) {
                down_btn = button;
                init_tf = cam->transformation();
                init_x = x;
                init_y = y;
                init_ra = rotate_axis;
            }
        };

        fix->mousemove = [this](int x, int y) {
            if(down_btn < 0) {
                init_x = x;
                init_y = y;
            }

            int dx = x - init_x;
            int dy = y - init_y;

            if(down_btn == GLFW_MOUSE_BUTTON_1) {
                transfrm tf;
                tf.translate(-dx / 30.0, dy / 30.0, 0);
                tf *= init_tf;

                cam->transformation() = tf;

                col3 rotate_axis_diff =
                    col3(tf.get_translation() - init_tf.get_translation());
                rotate_axis[0] = init_ra[0] + rotate_axis_diff[0];
                rotate_axis[2] = init_ra[2] + rotate_axis_diff[2];
            }

            if(down_btn == GLFW_MOUSE_BUTTON_3) {
                transfrm tf = init_tf;

                /*
                col4 _p0 = tf.get_mat() * col4 { 0, 0, 0, 1 };
                col3 p0 = col3(_p0 / _p0[3]);
                // find the intersection of sightline and 0-plane
                col4 _p1 = tf.get_mat() * col4 { 0, 0, -1, 1 };
                col3 p1 = col3(_p1 / _p1[3]);
                double diff_y = p1[1] - p0[1];
                col3 intersect = p0 + (p1 - p0) * (-p0[1] / diff_y);
                */

                tf.translate(-rotate_axis);
                tf.rotate(dx / 120.0, tf::zOx);
                // find the real x axis using cross
                col4 pos = tf.get_mat() * col4 { 0, 0, 1, 0 };
                col3 axis = cross(-col3(pos), col3 { 0, 1, 0 });
                if(norm(axis) < 0.001)
                    axis = col3 { 1, 0, 0 };
                tf.rotate_axis(-dy / 120.0, axis);
                tf.translate(rotate_axis);

                cam->transformation() = tf;
            }
        };

        fix->wheelscroll = [this](int x, int y) {
            double s = 1 - y / 36.0;
            cam->transformation().translate(-rotate_axis);
            cam->transformation().scale(s, s, s);
            cam->transformation().translate(rotate_axis);
        };
    }
};

#endif

inline std::string locate_assets(const std::string& fn)
{
    const char* pcp_1 = std::getenv("SHRTOOL_TEST_ASSETS_DIR");
    const char* pcp_2 = std::getenv("SHRTOOL_ASSETS_DIR");
    std::string cp_1 = pcp_1 ? pcp_1 : "";
    std::string cp_2 = pcp_2 ? pcp_2 : "";

    std::string path_1 = cp_1 +
        (cp_1.back() == '/' || cp_1.back() == '\\' || cp_1.empty() ? "" : "/") + fn;
    std::string path_2 = cp_2 +
        (cp_2.back() == '/' || cp_2.back() == '\\' || cp_2.empty() ? "" : "/") + fn;

    std::ifstream if_1(path_1);
    debug_log << "checking path " << path_1 << " ..."
        << (if_1.good() ? " good" : "bad") << std::endl;
    if(if_1.good()) return path_1;

    std::ifstream if_2(path_2);
    debug_log << "checking path " << path_2 << " ..."
        << (if_2.good() ? " good" : "bad") << std::endl;
    if(if_2.good()) return path_2;

    throw std::runtime_error("Cannot read file: " + fn);
}

}

#endif // TEST_UTILS_INCLUDED 

