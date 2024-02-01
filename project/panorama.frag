#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D panorama;
uniform bool nightVisionEnabled;

void main()
{
    vec4 color = texture(panorama, TexCoords);

    if (nightVisionEnabled) {
       // High contrast adjustment
        color.rgb = vec3((color.r + color.g + color.b) / 3.0 > 0.2 ? 0.0 : 1.0);

        // Convert to black and white
        color.rgb = vec3(color.r * 0.3 + color.g * 0.59 + color.b * 0.11);

        // Overlay green tint
        color.rgb = mix(color.rgb, vec3(0.0, 1.0, 0.0), 0.4);  
    }

    FragColor = color;
}