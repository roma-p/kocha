#include "platform.h"

#if KPLATFORM_APPLE 

// Include vulkan before GLFW. 
#include <vulkan/vulkan.h>

#include "core/logger.h"

// FIXME : ? 
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>


typedef struct internal_state {
    GLFWwindow* glfw_window;
} internal_state;

static f64 start_time = 0;

static void platform_console_write_file(FILE* file, const char* message, u8 colour);

static void platform_error_callback(int error, const char* description);
static void platform_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void platform_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void platform_cursor_position_callback(GLFWwindow* window, f64 xpos, f64 ypos);
static void platform_scroll_callback(GLFWwindow* window, f64 xoffset, f64 yoffset);
static void platform_framebuffer_size_callback(GLFWwindow* window, int width, int height);

// FIXME: input. 
//static keys translate_key(int key);

// PLATFORM STARTUP / SHUTDOWN -----------------------------------------------

b8 platform_startup(
	platform_state* platform_state,
	const char* application_name,
	i32 x,
	i32 y, 
	i32 width,
	i32 height) {

    //??? 
    platform_state->internal_state = malloc(sizeof(internal_state));
    internal_state* state = (internal_state*)platform_state->internal_state;

    glfwSetErrorCallback(platform_error_callback);
    if (!glfwInit()) {
        LOG_FATAL("Failed to initialise GLFW");
        return FALSE;
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // required for vulkan.

    state->glfw_window = glfwCreateWindow(width, height, application_name, 0, 0);
    if (!state->glfw_window) {
	LOG_FATAL("Failed to create a window");
	glfwTerminate();
	return FALSE;
    }

    glfwSetKeyCallback(state->glfw_window, platform_key_callback);
    glfwSetMouseButtonCallback(state->glfw_window, platform_mouse_button_callback);
    glfwSetCursorPosCallback(state->glfw_window, platform_cursor_position_callback);
    glfwSetScrollCallback(state->glfw_window, platform_scroll_callback);
    glfwSetFramebufferSizeCallback(state->glfw_window, platform_framebuffer_size_callback);

    glfwSetWindowPos(state->glfw_window, x, y);
    glfwShowWindow(state->glfw_window);
    start_time = glfwGetTime();

    return TRUE;
}

void platform_shutdown(platform_state* platform_state) {
    internal_state* state = (internal_state*)platform_state->internal_state;
    glfwDestroyWindow(state->glfw_window);
    state->glfw_window = 0;
    glfwSetErrorCallback(0);
    glfwTerminate();
}


b8 platform_pump_messages(platform_state* plat_state) {
    internal_state* state = (internal_state*)plat_state->internal_state;

    glfwPollEvents();
    b8 should_continue = !glfwWindowShouldClose(state->glfw_window);
    return should_continue;
}


// PLATFORM CALLBAKCS --------------------------------------------------------

static void platform_error_callback(int error, const char* description) {
    platform_console_write_error(description, 0);
}

static void platform_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
}

static void platform_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
}

static void platform_cursor_position_callback(GLFWwindow* window, f64 xpos, f64 ypos) {
}

static void platform_scroll_callback(GLFWwindow* window, f64 xoffset, f64 yoffset) {
}

static void platform_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
}

// MEMORY MANAGEMENT ---------------------------------------------------------

void* platform_allocate(u64 size, b8 aligned) {
    return malloc(size);
}

void platform_free(void* block, b8 aligned) {
    free(block);
}

void* platform_zero_memory(void* block, u64 size) {
    return memset(block, 0, size);
}

void* platform_copy_memory(void* dest, const void* source, u64 size) {
    return memcpy(dest, source, size);
}

void* platform_set_memory(void* dest, i32 value, u64 size) {
    return memset(dest, value, size);
}

// WRITE TO CONSOLE ----------------------------------------------------------

void platform_console_write(const char* message, u8 colour) {
    platform_console_write_file(stdout, message, colour);
}

void platform_console_write_error(const char* message, u8 colour) {
    platform_console_write_file(stderr, message, colour);
}

static void platform_console_write_file(FILE* file, const char* message, u8 colour) {
    // Colours: FATAL, ERROR, WARN, INFO, DEBUG, TRACE.
    const char* colour_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    fprintf(file, "\033[%sm%s\033[0m", colour_strings[colour], message);
}

// TIME ----------------------------------------------------------------------

f64 platform_get_absolute_time(void) {
    f64 time = glfwGetTime();
    return time;
}

void platform_sleep(u64 ms) {
    struct timespec ts = {0};
    ts.tv_sec = (long)((f64)ms * 0.001);
    ts.tv_nsec = ((long)ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
}

#endif //KPLATFORM_APPLE
