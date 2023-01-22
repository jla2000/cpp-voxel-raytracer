#include <array>
#include <iostream>

#include <gl/glew.h>
#include <glfw/glfw3.h>

#include "rendering/Shader.h"
#include "rendering/Camera.h"

const int screenWidth = 800;
const int screenHeight = 600;

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
        glfwGetInputMode(window, GLFW_STICKY_KEYS);

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

        const auto cameraRadius = 40.0f;
        glm::vec3 cameraPos{0, 16, cameraRadius};
        glm::vec3 camerTarget{16, 16, 16};
        Camera camera{
                cameraPos,
                camerTarget
        };


        glm::uvec3 mapSize{32, 32, 32};

        int invViewId = glGetUniformLocation(voxelProgram.id, "invView");
        int invCenteredViewId = glGetUniformLocation(voxelProgram.id, "invCenteredView");
        int invProjectionId = glGetUniformLocation(voxelProgram.id, "invProjection");
        int mapSizeId = glGetUniformLocation(voxelProgram.id, "mapSize");
        int frameId = glGetUniformLocation(voxelProgram.id, "frame");

        glUseProgram(voxelProgram.id);
        glUniform3uiv(mapSizeId, 1, &mapSize[0]);

        double previousFrame = glfwGetTime();
        double currentFrame{};
        int frameCount = 0;

        bool rotate = true;

        while (!glfwWindowShouldClose(window)) {
            currentFrame = glfwGetTime();
            ++frameCount;

            if (currentFrame - previousFrame >=  1.0) {
                std::cout << "Fps: " << frameCount << "\n";
                frameCount = 0;
                previousFrame = currentFrame;
            }
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                rotate = !rotate;
            }

            if (rotate) {
                auto rotationSpeed = glfwGetTime();
                auto camX = sin(rotationSpeed) * cameraRadius;
                auto camY = sin(rotationSpeed) * 14;
                auto camZ = cos(rotationSpeed) * cameraRadius;
                camera.position = camerTarget + glm::vec3{camX, camY, camZ};
            }

            auto view = calculateView(camera);
            auto projection = calculateProjection(screenWidth, screenHeight);

            auto invProjection = glm::inverse(projection);
            auto invView = glm::inverse(view);
            auto invCenteredView = invView;
            invCenteredView[3][0] = 0;
            invCenteredView[3][1] = 0;
            invCenteredView[3][2] = 0;

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(voxelProgram.id);

            glUniform1ui(frameId, frameCount);
            glUniformMatrix4fv(invViewId, 1, false, &invView[0][0]);
            glUniformMatrix4fv(invCenteredViewId, 1, false, &invCenteredView[0][0]);
            glUniformMatrix4fv(invProjectionId, 1, false, &invProjection[0][0]);

            glBindImageTexture(0, renderTextureId, 0, false, 0, GL_WRITE_ONLY, GL_RGBA32F);
            glDispatchCompute(screenWidth / 10, screenHeight / 10, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

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