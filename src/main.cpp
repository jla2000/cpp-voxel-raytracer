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
        const glm::uvec3 mapSize{32, 32, 32};
        Camera camera{
            //{-30, 30, -30},
            //{16, 12, 16},
            //{-mapSize.x, mapSize.y/2, -mapSize.z},
            glm::vec3{-1, 0.5f, -1} * glm::vec3{mapSize},
            {mapSize.x/2, mapSize.y/2, mapSize.z/2},
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

        uint8_t voxels[mapSize.x * mapSize.y * mapSize.z] {0};

        for (int z = 0; z < mapSize.z; ++z) {
            for (int y = 0; y < mapSize.y; ++y) {
                for (int x = 0; x < mapSize.x; ++x) {
                    int index = (z * mapSize.y * mapSize.x + y * mapSize.x + x);

                    const float sphereRadius = 14;
                    if (length(glm::vec3(x, y, z) - glm::vec3(16)) < sphereRadius){// && (x % 2 + y % 2 + z % 2 == 0)) {
                        voxels[index] = rand() % 256;
                    } else if (y == 0 || x == mapSize.x - 1 || z == mapSize.z - 1) {
                        voxels[index] = (rand() % 255) + 1;
                    }
                }
            }
        }

        uint32_t voxelPalette[256] {
            0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
            0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
            0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
            0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
            0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
            0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
            0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
            0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
            0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
            0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
            0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
            0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
            0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
            0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
            0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
            0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
        };

        GLuint voxelIndexBufferId;
        glGenBuffers(1, &voxelIndexBufferId);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelIndexBufferId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxels), &voxels[0], GL_STATIC_DRAW);

        GLuint voxelPaletteBufferId;
        glGenBuffers(1, &voxelPaletteBufferId);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelPaletteBufferId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxelPalette), &voxelPalette[0], GL_STATIC_DRAW);

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