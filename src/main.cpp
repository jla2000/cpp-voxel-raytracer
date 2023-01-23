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

        glViewport(0, 0, screenWidth, screenHeight);

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

        glClearColor(0, 1, 1, 1);

        const auto cameraRadius = 30.0f;
        glm::vec3 cameraPos{-cameraRadius, cameraRadius, -cameraRadius};
        glm::vec3 camerTarget{16, 12, 16};
        Camera camera{
                cameraPos,
                camerTarget,
                {0, 1 ,0},
        };

        glm::uvec3 mapSize{32, 32, 32};

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

            auto view = calculateView(camera);
            auto projection = calculateProjection(screenWidth, screenHeight);

            auto invProjection = glm::inverse(projection);
            auto invView = glm::inverse(view);
            auto invCenteredView = invView;
            invCenteredView[3][0] = 0;
            invCenteredView[3][1] = 0;
            invCenteredView[3][2] = 0;

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
                    glm::vec4 position = glm::vec4(camera.position, 1.0f);
                    glm::vec4 pivot = glm::vec4(camera.target, 1.0f);
                    glm::vec3 right = glm::transpose(view)[0];

                    float deltaAngleX = (2 * M_PI / screenWidth);
                    float deltaAngleY = (M_PI / screenHeight);
                    float xAngle = deltaX * deltaAngleX;
                    float yAngle = deltaY * deltaAngleY;

                    glm::vec3 viewDir = -glm::transpose(view)[2];
                    float cosAngle = dot(viewDir, camera.up);
                    if (cosAngle * glm::sign(deltaAngleY) > 0.99f)
                        deltaAngleY = 0;

                    glm::mat4 rotationX(1.0f);
                    rotationX = glm::rotate(rotationX, xAngle, camera.up);
                    position = (rotationX * (position - pivot)) + pivot;

                    glm::mat4 rotationY(1.0f);
                    rotationY = glm::rotate(rotationY, yAngle, right);
                    position = (rotationY * (position - pivot)) + pivot;

                    camera.position = position;

                    lastCursorX = cursorX;
                    lastCursorY = cursorY;
                    sampleCount = 1;
                }
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(voxelProgram.id);

            glUniform1ui(frameCountId, globalFrameCounter);
            glUniform1ui(sampleCountId, sampleCount);
            glUniformMatrix4fv(invViewId, 1, false, &invView[0][0]);
            glUniformMatrix4fv(invCenteredViewId, 1, false, &invCenteredView[0][0]);
            glUniformMatrix4fv(invProjectionId, 1, false, &invProjection[0][0]);

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