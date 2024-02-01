#version 330 core

in vec2 chTex;
out vec4 outCol;

uniform sampler2D uTex;
uniform float time; 
uniform float duration; 

void main()
{
    // Calculate the alpha based on time and duration
    float alpha = 1.0 - smoothstep(0.0, 1.0, min(time / duration, 1.0));

    vec4 textureColor = texture(uTex, chTex);

    outCol = vec4(textureColor.rgb, textureColor.a * alpha);
}
