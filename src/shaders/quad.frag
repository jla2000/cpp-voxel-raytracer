#version 440 core

uniform sampler2D renderTexture;

out vec4 color;
in vec2 textureCoord;

void main() {
    color = vec4(texture(renderTexture, textureCoord).xyz, 1);
    //color.xyz = vec3(texture(renderTexture, textureCoord).w / 150);
    //color.w = 1;
}