#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 TexCoords;

// Uniform variable for rectangle offset
uniform float offsetX;
uniform float offsetY;
uniform float textureScale;

void main(){
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoords = vec2((aPos.x - offsetX - 0.5) * textureScale + 0.5, (aPos.y - offsetY) * textureScale);
}