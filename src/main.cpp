#include <array>
#include <iostream>
#include <sstream>

#include <gl/glew.h>
#include <glfw/glfw3.h>
#include <glm/ext/matrix_transform.hpp>

#include "rendering/Shader.h"
#include "rendering/Camera.h"
#include "rendering/Model.h"

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
        Model model = loadVoxModel("assets/monu3.vox");

        Camera camera{
            glm::vec3{-1, 0.5f, -1} * glm::vec3{model.size},
            {model.size.x/2, model.size.y/2, model.size.z/2},
            screenWidth,
            screenHeight,
        };

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

        GLuint voxelIndexBufferId;
        glGenBuffers(1, &voxelIndexBufferId);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelIndexBufferId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, model.indices.size() * sizeof(uint8_t), model.indices.data(), GL_STATIC_DRAW);

        GLuint voxelPaletteBufferId;
        glGenBuffers(1, &voxelPaletteBufferId);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelPaletteBufferId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, model.palette.size() * sizeof(uint32_t), model.palette.data(), GL_STATIC_DRAW);

        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0, 1, 1, 1);

        int invViewId = glGetUniformLocation(voxelProgram.id, "invView");
        int invCenteredViewId = glGetUniformLocation(voxelProgram.id, "invCenteredView");
        int invProjectionId = glGetUniformLocation(voxelProgram.id, "invProjection");
        int mapSizeId = glGetUniformLocation(voxelProgram.id, "mapSize");
        int frameCountId = glGetUniformLocation(voxelProgram.id, "frameCount");
        int sampleCountId = glGetUniformLocation(voxelProgram.id, "sampleCount");

        glUseProgram(voxelProgram.id);
        glUniform3uiv(mapSizeId, 1, &model.size[0]);

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
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, voxelIndexBufferId);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxelPaletteBufferId);
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