#include <array>
#include <iostream>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
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
static double lastCursorX = 0;
static double lastCursorY = 0;

Camera camera{
        {0, 0, 0},
        {0, 0, 0},
        screenWidth,
        screenHeight,
};

static void mouseHandler(GLFWwindow* window, int button, int action, int code) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, code);
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            dragging = true;
            glfwGetCursorPos(window, &lastCursorX, &lastCursorY);
        } else if (action == GLFW_RELEASE) {
            dragging = false;
        }
    }
}

static void keyHandler(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    if (ImGui::GetIO().WantCaptureKeyboard)
        return;
}

static void charHandler(GLFWwindow* window, unsigned int codepoint) {
    ImGui_ImplGlfw_CharCallback(window, codepoint);

    if (ImGui::GetIO().WantCaptureKeyboard)
        return;
}

static void errorCallback(int error, const char* description)
{
    std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

int main(int argc, char *argv[]) {
    try {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize Glfw");
        }

        glfwSetErrorCallback(errorCallback);
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
        glfwSetKeyCallback(window, keyHandler);
        glfwSetCharCallback(window, charHandler);
        glfwMaximizeWindow(window);

        glewExperimental = true;
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("Failed to initialize Glew");
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        // Setup Dear ImGui style
        ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, false);
        ImGui_ImplOpenGL3_Init("#version 440");

        ShaderProgram quadProgram({
            {"shaders/quad.vert", GL_VERTEX_SHADER},
            {"shaders/quad.frag", GL_FRAGMENT_SHADER},
        });
        ShaderProgram voxelProgram({
            {"shaders/voxel.comp", GL_COMPUTE_SHADER}
        });
        Model model = loadVoxModel("assets/monu3.vox");

        camera.m_position = glm::vec3{-1, 0.5f, -1} * glm::vec3{model.size};
        camera.m_focusPoint = {model.size.x/2, model.size.y/2, model.size.z/2};
        camera.updateView();

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
        int numSamplesId = glGetUniformLocation(voxelProgram.id, "numSamples");
        int numRayBouncesId = glGetUniformLocation(voxelProgram.id, "numRayBounces");
        int maxDDADepthId = glGetUniformLocation(voxelProgram.id, "maxDDADepth");
        int sunDirId = glGetUniformLocation(voxelProgram.id, "sunDir");
        int enableShadowsId = glGetUniformLocation(voxelProgram.id, "enableShadows");
        int shadowMultiplierId = glGetUniformLocation(voxelProgram.id, "shadowMultiplier");

        unsigned int globalFrameCounter = 0;
        unsigned int numSamples = 1;
        int numRayBounces = 3;
        int maxDDADepth = 300;
        bool sample = true;
        glm::vec3 sunDir {100, 200, -100};
        bool enableShadows = false;
        float shadowMultiplier = 0.5;

        glUseProgram(voxelProgram.id);
        glUniform3uiv(mapSizeId, 1, &model.size[0]);
        glUniform1i(maxDDADepthId, maxDDADepth);
        glUniform1i(numRayBouncesId, numRayBounces);
        glUniform3fv(sunDirId, 1, &sunDir[0]);
        glUniform1i(enableShadowsId, enableShadows);
        glUniform1f(shadowMultiplierId, shadowMultiplier);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ++globalFrameCounter;

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
                    numSamples = 1;
                }
            }

            if (!sample) {
                numSamples = 1;
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(voxelProgram.id);

            ImGui::Begin("Settings");
            ImGui::Text("Ms/Frame: %.2f", 1000.0f / io.Framerate);
            ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
            ImGui::Text("Samples: %d", numSamples - 1);
            ImGui::Checkbox("Accumulate Samples", &sample);
            if (ImGui::InputInt("Num Ray Bounces", &numRayBounces, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
                glUniform1i(numRayBouncesId, numRayBounces);
                numSamples = 1;
            }
            if (ImGui::InputInt("Max DDA Depth", &maxDDADepth, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
                glUniform1i(maxDDADepthId, maxDDADepth);
                numSamples = 1;
            }
            if (ImGui::InputFloat3("Camera Position", &camera.m_position[0], "%.2f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                camera.updateView();
                numSamples = 1;
            }
            if (ImGui::InputFloat3("Camera Target", &camera.m_focusPoint[0], "%.2f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                camera.updateView();
                numSamples = 1;
            }
            if (ImGui::InputFloat3("Sun Direction", &sunDir[0], "%.2f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                glUniform3fv(sunDirId, 1, &sunDir[0]);
                numSamples = 1;
            }
            if (ImGui::Checkbox("Enable Shadows", &enableShadows)) {
                glUniform1i(enableShadowsId, enableShadows);
                numSamples = 1;
            }
            if (ImGui::InputFloat("Shadow Multiplier", &shadowMultiplier, 0.1f, 0.2f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                glUniform1f(shadowMultiplierId, shadowMultiplier);
                numSamples = 1;
            }
            ImGui::End();
            ImGui::Render();

            glUniform1ui(frameCountId, globalFrameCounter);
            glUniform1ui(numSamplesId, numSamples);
            glUniformMatrix4fv(invViewId, 1, false, &camera.m_invViewMat[0][0]);
            glUniformMatrix4fv(invCenteredViewId, 1, false, &camera.m_invCenteredMat[0][0]);
            glUniformMatrix4fv(invProjectionId, 1, false, &camera.m_invProjectionMat[0][0]);

            glBindImageTexture(0, renderTextureId, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, voxelIndexBufferId);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxelPaletteBufferId);
            glDispatchCompute(screenWidth / 10, screenHeight / 10, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
            ++numSamples;

            glUseProgram(quadProgram.id);
            glBindVertexArray(vertexArrayId);
            glDrawArrays(GL_TRIANGLES, 0, quadVertices.size() / 3);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}