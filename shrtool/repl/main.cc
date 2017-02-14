#include <poll.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <functional>
#include <map>

#include "scm.h"
#include "exception.h"
#include "shading.h"
#include "render_queue.h"

#define DEFAULT_PROMPT "> "
#define INCOMPLETE_PROMPT "... "

using namespace shrtool;

class display_window {
    GLFWwindow* window = nullptr;

    static std::map<GLFWwindow*, display_window*> map_;

    static void key_callback(GLFWwindow* window,
            int key, int scancode, int action, int mods) {
        auto i = map_.find(window);
        if(i == map_.end()) return;

        if(action == GLFW_PRESS)
            i->second->keypress(key, mods);
        else if(action == GLFW_RELEASE)
            i->second->keyrelease(key, mods);
    }

    static void fbsize_callback(GLFWwindow* window,
            int width, int height) {
        render_target::screen.force_set_viewport_(rect::from_size(width, height));
    }

    static void do_nothing_v() { }
    static void do_nothing_ii(int, int) { }

    bool should_close = false;

public:
    std::function<void()> update;
    std::function<void()> draw;
    std::function<void(int, int)> keypress;
    std::function<void(int, int)> keyrelease;

    display_window(const std::string ver_str = "330 core",
            std::string title = "Shader Tool",
            size_t w = 800, size_t h = 600) :
        update(do_nothing_v),
        draw(do_nothing_v),
        keypress(do_nothing_ii),
        keyrelease(do_nothing_ii)
    {
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

        window = glfwCreateWindow(w, h, title.c_str(), NULL, NULL);

        glfwMakeContextCurrent(window);

        glewExperimental = GL_TRUE;
        state = glewInit();
        if(state != GLEW_OK)
            throw driver_error("Failed to init " + ver_str + ": " +
                    (const char* const)glewGetErrorString(state));

        glfwSetKeyCallback(window, key_callback);
        glfwSetFramebufferSizeCallback(window, fbsize_callback);

        map_[window] = this;
        render_target::screen.force_set_viewport_(rect::from_size(w, h));
    }

    void main_loop() {
        while(!glfwWindowShouldClose(window) && !should_close) {
            update();
            draw();
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    void quit() {
        std::cout << "quit" << std::endl;
        should_close = true;
    }

    ~display_window() {
        map_.erase(map_.find(window));
    }
};

std::map<GLFWwindow*, display_window*> display_window::map_;

display_window dw;
std::string full_input;
render_task* main_rtask = nullptr;

void has_input(char* line)
{
    rl_callback_handler_remove();
    if(!line) return dw.quit();
    full_input += line;

    SCM val = scm_call_1(
            scm_c_public_ref("shrtool", "shrtool-repl-body"),
            scm_from_latin1_string(full_input.data()));

    if(scm_is_false(scm_car(val)) &&
            scm::is_symbol_eq(scm_cadr(val), "read-error")) {
        rl_callback_handler_install(INCOMPLETE_PROMPT, has_input);
    } else if(scm_is_false(scm_car(val)) &&
            scm::is_symbol_eq(scm_cadr(val), "quit")) {
        dw.quit();
        return;
    } else {
        rl_callback_handler_install(DEFAULT_PROMPT, has_input);

        for(char& c : full_input)
            c = c == '\n' ? ' ' : c;
        add_history(full_input.c_str());
        next_history();
        full_input.clear();
    }
}

int main(int argc, char* argv[])
{
    scm_init_guile();
    //refl::meta_manager::init();
    //scm::init_scm();
    scm_c_eval_string("(import (shrtool))");
    scm_c_eval_string("(define main-rtask #nil)");

    render_target::screen.set_bgcolor(color(51, 51, 51));

    if(argc > 1)
        scm_c_primitive_load(argv[1]);

    rl_callback_handler_install(DEFAULT_PROMPT, has_input);

    struct pollfd fd = { 0, POLLIN, 0 };

    dw.update = [&]() {
        while(poll(&fd, 1, 0), fd.revents & POLLIN) {
            rl_callback_read_char();
            fd.revents = 0;
        }

        SCM prob_mr = scm_c_eval_string(
                "(if (defined? 'main-rtask) main-rtask #nil)");

        if(!scm_is_null(prob_mr)) {
            refl::instance* ins = scm::extract_instance(prob_mr);

            if(ins->get_meta().is_same<queue_render_task>())
                main_rtask = &scm::extract_instance(prob_mr)
                    ->get<queue_render_task>();
            if(ins->get_meta().is_same<provided_render_task>())
                main_rtask = &scm::extract_instance(prob_mr)
                    ->get<provided_render_task>();
        } else
            main_rtask = nullptr;
    };

    dw.draw = [&]() {
        render_target::screen.clear_buffer(render_target::COLOR_BUFFER);
        render_target::screen.clear_buffer(render_target::DEPTH_BUFFER);

        if(main_rtask)
            main_rtask->render();
    };

    dw.main_loop();
}

