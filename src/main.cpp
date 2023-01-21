#include <iostream>

#include <gl/glew.h>
#include <glfw/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include "rendering/AccelerationStructure.h"
#include "rendering/Shader.h"
#include "rendering/Camera.h"
#include "rendering/VoxelBuffer.h"

int main(int argc, char *argv[]) {
    try {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize Glfw");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow *window = glfwCreateWindow(800, 600, "draft", nullptr, nullptr);

        if (!window) {
            throw std::runtime_error("Failed to open window");
        }
        glfwMakeContextCurrent(window);

        glewExperimental = true;
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("Failed to initialize Glew");
        }

        //AccelerationStructure as(asCenter, 1.0f);
        AccelerationStructure as({0, 0, 0}, {8, 8, 8});
        ShaderProgram program({
            {"shaders/tracing.vert", GL_VERTEX_SHADER},
            {"shaders/tracing.frag", GL_FRAGMENT_SHADER},
        });

        const auto cameraRadius = 15.0f;
        glm::vec3 cameraPos{0, 6, cameraRadius};
        glm::vec3 camerTarget{4, 4, 4};
        Camera camera{
            cameraPos,
            camerTarget
        };

        VoxelBuffer buffer{};
        for (int i = 0; i < 8; ++i) {
            buffer.voxel(i, i, i) = glm::vec3(i / 8, i * 0.2f, i * 0.5f);
        }

        auto model = glm::mat4(1.0f);
        auto view = calculateView(camera);
        auto projection = calculateProjection(800, 600);

        glViewport(0, 0, 800, 600);
        glUseProgram(program.id);

        auto modelLocation = glGetUniformLocation(program.id, "model");
        auto viewLocation = glGetUniformLocation(program.id, "view");
        auto projectionLocation = glGetUniformLocation(program.id, "projection");
        auto cameraPositionLocation = glGetUniformLocation(program.id, "cameraPositionWorldSpace");
        auto screenWidthPosition = glGetUniformLocation(program.id, "screenWidth");
        auto screenHeightPosition = glGetUniformLocation(program.id, "screenHeight");
        auto voxelBufferPosition = glGetUniformLocation(program.id, "voxelBuffer");

        glUniformMatrix4fv(modelLocation, 1, false, &model[0][0]);
        glUniformMatrix4fv(viewLocation, 1, false, &view[0][0]);
        glUniformMatrix4fv(projectionLocation, 1, false, &projection[0][0]);
        glUniform3fv(cameraPositionLocation, 1, &camera.position[0]);
        glUniform1f(screenWidthPosition, 800);
        glUniform1f(screenHeightPosition, 600);
        glUniform3fv(voxelBufferPosition, 8*8*8, &buffer.data[0][0]);

        GLuint vertexBufferId;
        glGenBuffers(1, &vertexBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
        glBufferData(GL_ARRAY_BUFFER, 4 * as.vertices.size(), as.vertices.data(), GL_STATIC_DRAW);

        GLuint elementBufferId;
        glGenBuffers(1, &elementBufferId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * as.triangles.size(), as.triangles.data(), GL_STATIC_DRAW);

        GLuint vertexArrayId;
        glGenVertexArrays(1, &vertexArrayId);
        glBindVertexArray(vertexArrayId);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        glClearColor(1, 1, 1, 1);

        while (!glfwWindowShouldClose(window)) {
            auto rotationSpeed = glfwGetTime();

            auto camX = sin(rotationSpeed) * cameraRadius;
            auto camZ = cos(rotationSpeed) * cameraRadius;
            camera.position = camerTarget + glm::vec3{camX, cameraPos.y, camZ};

            view = calculateView(camera);
            glUniformMatrix4fv(viewLocation, 1, false, &view[0][0]);
            glUniform3fv(cameraPositionLocation, 1, &camera.position[0]);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindVertexArray(vertexArrayId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId);
            glDrawElements(GL_TRIANGLES, as.triangles.size(), GL_UNSIGNED_INT, nullptr);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}