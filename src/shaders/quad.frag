#version 440 core

uniform sampler2D renderTexture;

out vec4 color;
in vec2 textureCoord;

void main() {
    color = vec4(texture(renderTexture, textureCoord).xyz, 1);
    //color = vec4(1, 0, 0, 1);
}