// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/affine.hpp>
#include <common/geometry.hpp>
#include <common/arcball.hpp>
#include <common/mouse.h>

float g_groundSize = 100.0f;
float g_groundY = -2.5f;

GLint lightLocGround, lightLocRed, lightLocGreen, lightLocArc;

// View properties
glm::mat4 Projection;
float windowWidth = 1024.0f;
float windowHeight = 768.0f;
int frameBufferWidth = 0;
int frameBufferHeight = 0;
float fov = 45.0f;
float fovy = fov;
int viewpoint = 0;
int number_of_frames = 0;
int thisFrame = 0;
int aFrame_num = 0;

// Model properties
Model ground, redCube, greenCube;
glm::mat4 skyRBT;
glm::mat4 g_objectRbt[2] = { glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.5f, 0.0f)) * glm::rotate(glm::mat4(1.0f), -90.0f, glm::vec3(0.0f, 1.0f, 0.0f)), // RBT for redCube
                            glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.5f, 0.0f)) * glm::rotate(glm::mat4(1.0f), 90.0f, glm::vec3(0.0f, 1.0f, 0.0f))}; // RBT for greenCube
glm::mat4 eyeRBT;
glm::mat4 worldRBT = glm::mat4(1.0f);
glm::mat4 *objRBT;
glm::mat4 aFrame;

// manipulation helping variables
MouseStatus *mouse = new MouseStatus();
double delta_x = 0;
double delta_y = 0;

// Arcball manipulation
Model arcBall;
glm::mat4 arcballRBT = glm::mat4(1.0f);
float arcBallScreenRadius = 0.25f * min(windowWidth, windowHeight); // for the initial assignment
float arcBallScale = 0.01f;

// Function definition
static void window_size_callback(GLFWwindow*, int, int);
static void mouse_button_callback(GLFWwindow*, int, int, int);
static void cursor_pos_callback(GLFWwindow*, double, double);
static void keyboard_callback(GLFWwindow*, int, int, int, int);
void update_fovy(void);

// Helper functions
void do_rbt_respectTO(glm::mat4, glm::mat4, glm::mat4*);

// Helper function: Update the vertical field-of-view(float fovy in global)
void update_fovy()
{
    if (frameBufferWidth >= frameBufferHeight)
    {
        fovy = fov;
    }
    else {
        const float RAD_PER_DEG = 0.5f * glm::pi<float>() / 180.0f;
        fovy = (float) atan2(sin(fov * RAD_PER_DEG) * ((float) frameBufferHeight / (float) frameBufferWidth), cos(fov * RAD_PER_DEG)) / RAD_PER_DEG;
    }
}

// TODO: Modify GLFW window resized callback function
static void window_size_callback(GLFWwindow* window, int width, int height)
{
    // Get resized size and set to current window
    windowWidth = (float)width;
    windowHeight = (float)height;

    // glViewport accept pixel size, please use glfwGetFramebufferSize rather than window size.
    // window size != framebuffer size
    glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, (GLsizei) frameBufferWidth, (GLsizei) frameBufferHeight);

    // Update projection matrix
    Projection = glm::perspective(fov, ((float) frameBufferWidth / (float) frameBufferHeight), 0.1f, 100.0f);
}

// TODO: Fill up GLFW mouse button callback function
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) // mouse was pressed
    {
        mouse->set_isPressed(true);
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            mouse->set_pressed(GLFW_MOUSE_BUTTON_LEFT);
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            mouse->set_pressed(GLFW_MOUSE_BUTTON_RIGHT);
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
        {
            mouse->set_pressed(GLFW_MOUSE_BUTTON_MIDDLE);
        }
    }
    else if (action == GLFW_RELEASE) // mouse released
    {
        mouse->set_isPressed(false);
    }
}

// TODO: Fill up GLFW cursor position callback function
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    // to manipulate the object, apply tanslation and rotation on thisRBT!
    // (x, y) = (0,0) is the lefthand top corner of the screen
    delta_x = mouse->get_deltaX(xpos);
    delta_y = mouse->get_deltaY(ypos);
    if (mouse->get_isPressed())
    {
        if (mouse->get_pressed() == GLFW_MOUSE_BUTTON_LEFT)
        {
            // TODO: to be implemented with arcball
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
            do_rbt_respectTO(aFrame, rotation, objRBT);
        }
        if (mouse->get_pressed() == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if ((objRBT == &skyRBT) && (eyeRBT == skyRBT) && (aFrame_num == 0))
            {
                delta_x = -delta_x;
                delta_y = -delta_y;
            }
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(delta_x / 20.0f, -delta_y / 20.0f, 0.0f));
            do_rbt_respectTO(aFrame, translation, objRBT);
            do_rbt_respectTO(aFrame, translation, &arcballRBT);
        }
        if (mouse->get_pressed() == GLFW_MOUSE_BUTTON_MIDDLE)
        {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, delta_y/20.0f));
            do_rbt_respectTO(aFrame, translation, objRBT);
            do_rbt_respectTO(aFrame, translation, &arcballRBT);
        }
    }

}

static void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_H:
                std::cout << "CS380 Homework Assignment 2" << std::endl;
                std::cout << "keymaps:" << std::endl;
                std::cout << "h\t\t Help command" << std::endl;
                std::cout << "v\t\t Change eye frame (your viewpoint)" << std::endl;
                std::cout << "o\t\t Change current manipulating object" << std::endl;
                std::cout << "m\t\t Change auxiliary frame between world-sky and sky-sky" << std::endl;
                std::cout << "c\t\t Change manipulation method" << std::endl;
                break;
            case GLFW_KEY_V:
                // changes viewpoint
                viewpoint = (++viewpoint) % number_of_frames;
                break;
            case GLFW_KEY_O:
                // this has to change the frame
                thisFrame = (++thisFrame) % number_of_frames; // should include sky
                if (thisFrame == 0) {
                    objRBT = &skyRBT;
                    arcballRBT = worldRBT;
                }
                else {
                    objRBT = &g_objectRbt[thisFrame - 1];
                    arcballRBT = *objRBT;
                }
                break;
            // change the frame with which the rotation operates
            case GLFW_KEY_M:
                // TODO: Change auxiliary frame between world-sky and sky-sky
                // manipulate aFrame
                if (aFrame_num == 0) {
                    aFrame = transFact(worldRBT) * linearFact(skyRBT);
                    aFrame_num++;
                }
                else {
                    aFrame = transFact(skyRBT) * linearFact(skyRBT);
                    aFrame_num--;
                }
                break;
            case GLFW_KEY_X: {
                glm::mat4 m_rotation2 = glm::rotate(glm::mat4(1.0f), 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
                *objRBT = aFrame * m_rotation2 * glm::inverse(aFrame) * *objRBT;
                break;
            }
            case GLFW_KEY_C: {
                // TODO: Add an additional manipulation method
                glm::mat4 m_rotation = glm::rotate(glm::mat4(1.0f), 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                *objRBT = aFrame * m_rotation * glm::inverse(aFrame) * *objRBT;
                break;
            }
        }
    }
}


/*
 * Helper function : applies an RBT with respect to current auxillary frame
 */
void do_rbt_respectTO(glm::mat4 aFrame, glm::mat4 transform, glm::mat4 *objRBT)
{
    *objRBT = aFrame * transform * glm::inverse(aFrame) * *objRBT;
}

int main(void)
{
    // Initialise GLFW
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow((int)windowWidth, (int)windowHeight, "HW2 - 20111044", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = (GLboolean) true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        return -1;
    }

    // Callback functions
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetKeyCallback(window, keyboard_callback);

    glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
    // Update arcBallScreenRadius with frame buffer size
    arcBallScreenRadius = 0.25f * min((float) frameBufferWidth, (float) frameBufferHeight); // for the initial assignment

    // Clear with sky color
    glClearColor((GLclampf)(128. / 255.), (GLclampf)(200. / 255.), (GLclampf)(255. / 255.), (GLclampf) 0.);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    // Enable backface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // (fovy, aspect, near, far)
    Projection = glm::perspective(fov, ((float) frameBufferWidth / (float) frameBufferHeight), 0.1f, 100.0f);
    skyRBT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.25, 4.0));

    // initial eye frame = sky frame;
    eyeRBT = skyRBT;
    objRBT = &skyRBT;
    number_of_frames++;

    // Initialize Ground Model
    ground = Model();
    init_ground(ground);
    ground.initialize(DRAW_TYPE::ARRAY, "VertexShader.glsl", "FragmentShader.glsl"); //
    ground.set_projection(&Projection);
    ground.set_eye(&eyeRBT);
    glm::mat4 groundRBT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, g_groundY, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(g_groundSize, 1.0f, g_groundSize));
    ground.set_model(&groundRBT);

    redCube = Model();
    init_cube(redCube, glm::vec3(1.0, 0.0, 0.0));
    redCube.initialize(DRAW_TYPE::ARRAY, "VertexShader.glsl", "FragmentShader.glsl");

    redCube.set_projection(&Projection);
    redCube.set_eye(&eyeRBT);
    redCube.set_model(&g_objectRbt[0]);
    number_of_frames++;


    greenCube = Model();
    init_cube(greenCube, glm::vec3(0.0, 1.0, 0.0));
    greenCube.initialize(DRAW_TYPE::ARRAY, "VertexShader.glsl", "FragmentShader.glsl");

    greenCube.set_projection(&Projection);
    greenCube.set_eye(&eyeRBT);
    greenCube.set_model(&g_objectRbt[1]);
    number_of_frames++;

    // TODO: Initialize arcBall
    // Initialize your arcBall with DRAW_TYPE::INDEX (it uses GL_ELEMENT_ARRAY_BUFFER to draw sphere)
    arcBall = Model();
    init_sphere(arcBall);
    arcBall.initialize(DRAW_TYPE::INDEX, "VertexShader.glsl", "FragmentShader.glsl");

    arcBall.set_projection(&Projection);
    arcBall.set_eye(&eyeRBT);
    arcBall.set_model(&arcballRBT);




    // Setting Light Vectors
    glm::vec3 lightVec = glm::vec3(0.0f, 1.0f, 0.0f);
    lightLocGround = glGetUniformLocation(ground.GLSLProgramID, "uLight");
    glUniform3f(lightLocGround, lightVec.x, lightVec.y, lightVec.z);

    lightLocRed = glGetUniformLocation(redCube.GLSLProgramID, "uLight");
    glUniform3f(lightLocRed, lightVec.x, lightVec.y, lightVec.z);

    lightLocGreen = glGetUniformLocation(greenCube.GLSLProgramID, "uLight");
    glUniform3f(lightLocGreen, lightVec.x, lightVec.y, lightVec.z);

    lightLocArc = glGetUniformLocation(arcBall.GLSLProgramID, "uLight");
    glUniform3f(lightLocArc, lightVec.x, lightVec.y, lightVec.z);

    do {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // update Eye
        switch(viewpoint) {
            case 0:
                eyeRBT = skyRBT;
                break;
            case 1:
                eyeRBT = g_objectRbt[0];
                break;
            case 2:
                eyeRBT = g_objectRbt[1];
                break;
            default: // should not happen!
                break;
        }
        // update auxillary frame
        aFrame = transFact(*objRBT) * linearFact(eyeRBT);



        redCube.draw();
        greenCube.draw();
        ground.draw();

        // TODO: Draw wireframe of arcball with dynamic radius
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // draw wireframe
        arcBall.draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // draw filled models again.

        // Swap buffers (Double buffering)
        glfwSwapBuffers(window);
        glfwPollEvents();
    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);

    // Clean up data structures and glsl objects
    ground.cleanup();
    redCube.cleanup();
    greenCube.cleanup();
    arcBall.cleanup();

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}
