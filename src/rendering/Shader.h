#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>

struct Shader {
    Shader(const std::string& filename, GLenum type);
    ~Shader();

    GLuint id;
};

struct ShaderProgram {
    ShaderProgram(const std::vector<Shader>& shaders);
    ~ShaderProgram();

    GLuint id;
};
