#include "Shader.h"

#include <fstream>
#include <sstream>

std::string loadShaderSource(const std::string& filename) {
    std::ostringstream iss;
    std::ifstream ifs(filename);

    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    iss << ifs.rdbuf();
    return iss.str();
}

Shader::Shader(const std::string &filename, GLenum type) {
    id = glCreateShader(type);

    auto srcCode = loadShaderSource(filename);
    auto srcCodeCstr = srcCode.c_str();

    glShaderSource(id, 1, &srcCodeCstr, nullptr);
    glCompileShader(id);

    GLint status{GL_TRUE};
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        GLint infoLogLength{0};
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<char> infoLog;
        infoLog.resize(infoLogLength + 1);

        glGetShaderInfoLog(id, infoLogLength, nullptr, infoLog.data());
        throw std::runtime_error(filename + ":\n" + infoLog.data());
    }
}

Shader::~Shader() {
    glDeleteShader(id);
}

ShaderProgram::ShaderProgram(const std::vector<Shader> &shaders) {
    id = glCreateProgram();

    for (const auto& shader : shaders) {
        glAttachShader(id, shader.id);
    }

    glLinkProgram(id);

    GLint status{GL_TRUE};
    glGetProgramiv(id, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint infoLogLength{0};
        glGetProgramiv(id, GL_LINK_STATUS, &infoLogLength);

        std::vector<char> infoLog;
        infoLog.resize(infoLogLength + 1);

        glGetProgramInfoLog(id, infoLogLength, nullptr, infoLog.data());
        throw std::runtime_error(infoLog.data());
    }

    for (const auto& shader : shaders) {
        glDetachShader(id, shader.id);
    }
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(id);
}