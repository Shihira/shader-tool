#ifndef TEST_UTILS_INCLUDED 
#define TEST_UTILS_INCLUDED 

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <fstream>

#include "unit_test.h"
#include "singleton.h"
#include "exception.h"
#include "shading.h"

namespace shrtool {

#ifndef NO_GUI_TEST

struct gui_fixture_base;

class gui_test_context : public generic_singleton<gui_test_context>
{
public:
    GLFWwindow* window = nullptr;
    gui_fixture_base* active_test = nullptr;

    static inline void key_callback(GLFWwindow* window,
            int key, int scancode, int action, int mods);

    static void fbsize_callback(GLFWwindow* window,
            int width, int height) {
        render_target::screen.set_viewport(rect::from_size(width, height));
    }

    static void init(const std::string ver_str = "330 core",
            std::string title = "Test", size_t w = 800, size_t h = 600) {
        unsigned state = glfwInit();
        if(!state) throw driver_error("Failed to init GLFW");

        std::stringstream ss(ver_str);
        unsigned ver; std::string profile_str;
        ss >> ver >> profile_str;
        unsigned ver_1 = ver / 100, ver_2 = ver % 100 / 10,
                 prof = profile_str == "core" ?
                     GLFW_OPENGL_CORE_PROFILE : GLFW_OPENGL_ANY_PROFILE;

        if(title.empty()) glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, ver_1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, ver_2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, prof);

        inst().window = glfwCreateWindow(w, h, title.c_str(), NULL, NULL);

        glfwMakeContextCurrent(inst().window);

        glewExperimental = GL_TRUE;
        state = glewInit();
        if(state != GLEW_OK)
            throw driver_error("Failed to init " + ver_str + ": " +
                    (const char* const)glewGetErrorString(state));

        glfwSetKeyCallback(inst().window, key_callback);
        glfwSetFramebufferSizeCallback(inst().window, fbsize_callback);

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
    virtual void operator() () = 0;

    enum { PENDING, RUNNING, FINISHED } state = PENDING;

    void main_loop() {
        state = RUNNING;
        while(state != FINISHED &&
                !glfwWindowShouldClose(gui_test_context::inst().window)) {
            do_update();
            do_draw();
            glfwSwapBuffers(gui_test_context::inst().window);
            glfwPollEvents();
        }
    }
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

struct singlefunc_fixture : gui_fixture_base
{
    std::function<void()> update;
    std::function<void()> draw;
    std::function<void(int, int)> keypress;
    std::function<void(int, int)> keyrelease;

    void do_update() override { if(update) update(); }
    void do_draw() override { if(draw) draw(); }
    void do_keypress(int key, int modifiers) override
        { if(keypress) keypress(key, modifiers); }
    void do_keyrelease(int key, int modifiers) override
        { if(keyrelease) keyrelease(key, modifiers); }
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

