﻿#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/mesh.h>
#include <learnopengl/model.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

#include "skybox.h"
#include "scene.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, int &shadow_mode, bool &SSR_test, bool &SSR_ON, bool& scatter_ON);
void print_info(float delta_time, int shadow_mode, bool SSR_test, bool SSR_ON, bool scatter_ON);

// camera
Camera camera(glm::vec3(2.0f, 1.0f, 4.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "A Midsummer Night", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Skybox skybox;
    Scene scene;

    // render loop
    // -----------
    int shadow_mode = 2, last_shadow_mode = 2;
    bool SSR_test = false, SSR_ON = false, scatter_ON = false, last_SSR_test = false, last_SSR_ON = false, last_scatter_ON = false;
    float last_print = 0.0f;

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    std::cout << "shadows samples = " << SHADOWS_SAMPLES << std::endl;
    std::cout << "SSR samples = " << SSR_SAMPLES << std::endl;
    std::cout << "scatter samples = " << SCATTER_SAMPLES << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (currentFrame - last_print > 1.0f || shadow_mode != last_shadow_mode ||
           (SSR_test && SSR_ON) != (last_SSR_test && last_SSR_ON) || scatter_ON != last_scatter_ON) {
            last_shadow_mode = shadow_mode;
            last_SSR_test = SSR_test;
            last_SSR_ON = SSR_ON;
            last_scatter_ON = scatter_ON;
            if (last_print > 0.0f)
                std::cout << "\x1b[A" << "\x1b[A" << "\x1b[A" << "\x1b[A";
            last_print = currentFrame;
            print_info(deltaTime, shadow_mode, SSR_test, SSR_ON, scatter_ON);
        }
        // input
        // -----
        processInput(window, shadow_mode, SSR_test, SSR_ON, scatter_ON);

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        scene.render(camera.Position, view, projection, shadow_mode, SSR_test, SSR_ON, scatter_ON, deltaTime, (float)glfwGetTime());
        skybox.render(glm::mat4(glm::mat3(view)), projection);
        
        
        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void print_info(float deltaTime, int shadow_mode, bool SSR_test, bool SSR_ON, bool scatter_ON)
{
    std::cout << "FPS : " << int(1.0f / deltaTime) << std::endl;

    std::cout << "shadow mode : ";
    if (shadow_mode == 0)
        std::cout << "hard shadow" << std::endl;
    else if (shadow_mode == 1)
        std::cout << "PCF        " << std::endl;
    else
        std::cout << "PCSS       " << std::endl;

    std::cout << "SSR ";
    if (SSR_test && SSR_ON)
        std::cout << "ON " << std::endl;
    else
        std::cout << "OFF" << std::endl;

    std::cout << "volumetric light ";
    if (scatter_ON)
        std::cout << "ON " << std::endl;
    else
        std::cout << "OFF" << std::endl;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window, int &shadow_mode, bool& SSR_test, bool& SSR_ON, bool &scatter_ON)
{
    static double last_change = 0.0;
    const double min_delta_time = 0.4;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        double now = glfwGetTime();
        if (now - last_change >= min_delta_time) {
            last_change = now;
            SSR_test = !SSR_test;
            if (SSR_test)
                SSR_ON = false;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        double now = glfwGetTime();
        if (now - last_change >= min_delta_time) {
            last_change = now;
            SSR_ON = !SSR_ON;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        double now = glfwGetTime();
        if (now - last_change >= min_delta_time) {
            last_change = now;
            shadow_mode = 0;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        double now = glfwGetTime();
        if (now - last_change >= min_delta_time) {
            last_change = now;
            shadow_mode = 1;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        double now = glfwGetTime();
        if (now - last_change >= min_delta_time) {
            last_change = now;
            shadow_mode = 2;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        double now = glfwGetTime();
        if (now - last_change >= min_delta_time) {
            last_change = now;
            scatter_ON = !scatter_ON;
        }
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}