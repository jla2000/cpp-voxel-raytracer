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
static bool denoise = true;
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
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        denoise = !denoise;
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

        glm::vec4 voxels[32*32*32] {};
        //voxels[0] = {1, 1, 0};
        //voxels[1] = {0, 1, 0};
        //voxels[2] = {0, 1, 1};
        //voxels[32] = {0, 1, 0};
        //voxels[64] = {0, 1, 0};
        for (int z = 0; z < 32; ++z) {
            for (int y = 0; y < 32; ++y) {
                for (int x = 0; x < 32; ++x) {
                    int index = (z * 32 * 32 + y * 32 + x);

                    const float sphereRadius = 14;
                    if (length(glm::vec3(x, y, z) - glm::vec3(16)) < sphereRadius && (x % 2 + y % 2 + z % 2 == 0)) {
                        voxels[index] = glm::vec4((glm::vec3(x, y, z) - glm::vec3(16)) / (2 * sphereRadius) + glm::vec3(0.6), 1);
                    } else if (y == 0 || x == 31 || z == 31) {
                        voxels[index] = glm::vec4(0.8, 0.8, 0.8, 2.0);
                    }
                }
            }
        }

        GLuint voxelBufferId;
        glGenBuffers(1, &voxelBufferId);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelBufferId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxels), &voxels[0][0], GL_STATIC_DRAW);

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

            if (!denoise) {
                sampleCount = 1;
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(voxelProgram.id);

            glUniform1ui(frameCountId, globalFrameCounter);
            glUniform1ui(sampleCountId, sampleCount);
            glUniformMatrix4fv(invViewId, 1, false, &camera.m_invViewMat[0][0]);
            glUniformMatrix4fv(invCenteredViewId, 1, false, &camera.m_invCenteredMat[0][0]);
            glUniformMatrix4fv(invProjectionId, 1, false, &camera.m_invProjectionMat[0][0]);

            glBindImageTexture(0, renderTextureId, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, voxelBufferId);
            glDispatchCompute(screenWidth / 10, screenHeight / 10, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
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