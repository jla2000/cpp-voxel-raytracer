#include <array>
#include <iostream>
#include <sstream>

#include <gl/glew.h>
#include <glfw/glfw3.h>

#include "rendering/Shader.h"
#include "rendering/Camera.h"
#include "glm/ext/matrix_transform.hpp"

const int screenWidth = 1920;
const int screenHeight = 1010;

const std::array<GLfloat, 18> quadVertices {
    -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f,
};

const std::array<GLfloat, 18> quadUvs {
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,
};

static bool dragging = false;
static double lastCursorX = 0;
static double lastCursorY = 0;

static void mouseHandler(GLFWwindow* window, int button, int action, int code) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            dragging = true;
            glfwGetCursorPos(window, &lastCursorX, &lastCursorY);
        } else if (action == GLFW_RELEASE) {
            dragging = false;
        }
    }
}

int main(int argc, char *argv[]) {
    try {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize Glfw");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "draft", nullptr, nullptr);

        if (!window) {
            throw std::runtime_error("Failed to open window");
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(0);

        glfwSetMouseButtonCallback(window, mouseHandler);

        glewExperimental = true;
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("Failed to initialize Glew");
        }

        ShaderProgram quadProgram({
            {"shaders/quad.vert", GL_VERTEX_SHADER},
            {"shaders/quad.frag", GL_FRAGMENT_SHADER},
        });
        ShaderProgram voxelProgram({
            {"shaders/voxel.comp", GL_COMPUTE_SHADER}
        });
        Camera camera{
            {-30, 30, -30},
            {16, 12, 16},
            screenWidth,
            screenHeight,
        };
        glm::uvec3 mapSize{32, 32, 32};

        GLuint vertexArrayId;
        glGenVertexArrays(1, &vertexArrayId);
        glBindVertexArray(vertexArrayId);

        GLuint vertexBufferId;
        glGenBuffers(1, &vertexBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
        glBufferData(GL_ARRAY_BUFFER, quadVertices.size() * sizeof(GLfloat), quadVertices.data(), GL_STATIC_DRAW);

        GLuint uvBufferId;
        glGenBuffers(1, &uvBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferId);
        glBufferData(GL_ARRAY_BUFFER, quadUvs.size() * sizeof(GLfloat), quadUvs.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferId);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        GLuint renderTextureId;
        glGenTextures(1, &renderTextureId);
        glBindTexture(GL_TEXTURE_2D, renderTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0, 1, 1, 1);

        int invViewId = glGetUniformLocation(voxelProgram.id, "invView");
        int invCenteredViewId = glGetUniformLocation(voxelProgram.id, "invCenteredView");
        int invProjectionId = glGetUniformLocation(voxelProgram.id, "invProjection");
        int mapSizeId = glGetUniformLocation(voxelProgram.id, "mapSize");
        int frameCountId = glGetUniformLocation(voxelProgram.id, "frameCount");
        int sampleCountId = glGetUniformLocation(voxelProgram.id, "sampleCount");

        glUseProgram(voxelProgram.id);
        glUniform3uiv(mapSizeId, 1, &mapSize[0]);

        double previousFrame = glfwGetTime();
        double currentFrame{};

        unsigned int globalFrameCounter = 0;
        unsigned int localFrameCounter = 0;
        unsigned int sampleCount = 1;

        while (!glfwWindowShouldClose(window)) {
            currentFrame = glfwGetTime();
            ++globalFrameCounter;
            ++localFrameCounter;

            if (currentFrame - previousFrame >=  1.0) {
                std::ostringstream oss;
                oss << "draft";
                oss << " Fps: " << localFrameCounter;
                oss << " Samples: " << (sampleCount - 1);

                glfwSetWindowTitle(window, oss.str().c_str());
                localFrameCounter = 0;
                previousFrame = currentFrame;
            }

            if (dragging) {
                double cursorX;
                double cursorY;
                glfwGetCursorPos(window, &cursorX, &cursorY);

                double deltaX = lastCursorX - cursorX;
                double deltaY = lastCursorY - cursorY;

                if (deltaX != 0.0 || deltaY != 0.0) {
                    camera.arcBallRotate(deltaX, deltaY, screenWidth, screenHeight);
                    lastCursorX = cursorX;
                    lastCursorY = cursorY;
                    sampleCount = 1;
                }
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(voxelProgram.id);

            glUniform1ui(frameCountId, globalFrameCounter);
            glUniform1ui(sampleCountId, sampleCount);
            glUniformMatrix4fv(invViewId, 1, false, &camera.m_invViewMat[0][0]);
            glUniformMatrix4fv(invCenteredViewId, 1, false, &camera.m_invCenteredMat[0][0]);
            glUniformMatrix4fv(invProjectionId, 1, false, &camera.m_invProjectionMat[0][0]);

            glBindImageTexture(0, renderTextureId, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
            glDispatchCompute(screenWidth / 10, screenHeight / 10, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            ++sampleCount;

            glUseProgram(quadProgram.id);
            glBindVertexArray(vertexArrayId);
            glDrawArrays(GL_TRIANGLES, 0, quadVertices.size() / 3);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}