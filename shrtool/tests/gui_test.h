#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "unit_test.h"
#include "singleton.h"
#include "exception.h"
#include "shading.h"

namespace shrtool {

class gui_fixture_base;

class gui_test_context : public generic_singleton<gui_test_context>
{
public:
    GLFWwindow* window = nullptr;
    gui_fixture_base* active_test = nullptr;

    static inline void key_callback(GLFWwindow* window,
            int key, int scancode, int action, int mods);

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
    virtual void operator() () = 0;

    enum { PENDING, RUNNING, FINISHED } state = PENDING;

    void main_loop() {
        state = RUNNING;
        while(state != FINISHED) {
            do_update();
            do_draw();
            glfwSwapBuffers(gui_test_context::inst().window);
            glfwPollEvents();

            // it would be better to put close detect after the first draw
            // or there may trigger some segfault.
            if(glfwWindowShouldClose(gui_test_context::inst().window))
                break;
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
}

struct singlefunc_fixture : gui_fixture_base
{
    std::function<void()> update;
    std::function<void()> draw;
    std::function<void(int, int)> keypress;

    void do_update() override { if(update) update(); }
    void do_draw() override { if(draw) draw(); }
    void do_keypress(int key, int modifiers) override
        { if(keypress) keypress(key, modifiers); }
};

}

