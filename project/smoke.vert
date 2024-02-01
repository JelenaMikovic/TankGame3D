#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTex;
out vec2 chTex;

uniform float time;
uniform float duration;

void main()
{
    // Calculate the scale based on time and duration
    float scale = smoothstep(0.0, 1.0, min(time / duration, 1.0));

    vec2 scaledPos = inPos * scale;

    gl_Position = vec4(scaledPos.x, scaledPos.y, 0.0, 1.0);

    chTex = inTex;
}
