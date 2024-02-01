#version 330 core

struct Light {
    vec3 pos;
    vec3 kA;
    vec3 kD;
    vec3 kS;
};

struct Material {
    vec3 kA;
    vec3 kD;
    vec3 kS;
    float shine;
};

in vec3 chNor;
in vec3 chFragPos;

out vec4 outCol;

uniform Light uLight;
uniform Material uMaterial;
uniform vec3 uViewPos; 
uniform bool nightVisionEnabled; 

void main()
{
    vec3 resA = uLight.kA * uMaterial.kA;

    vec3 normal = normalize(chNor);
    vec3 lightDirection = normalize(uLight.pos - chFragPos);
    float nD = max(dot(normal, lightDirection), 0.0);
    vec3 resD = uLight.kD * (nD * uMaterial.kD);

    vec3 viewDirection = normalize(uViewPos - chFragPos);
    vec3 reflectionDirection = reflect(-lightDirection, normal);
    float s = pow(max(dot(viewDirection, reflectionDirection), 0.0), uMaterial.shine);
    vec3 resS = uLight.kS * (s * uMaterial.kS);

    // Night Vision Effect
    if (nightVisionEnabled) {
        // High contrast adjustment
        vec3 color = resA + resD + resS;
        

        // Convert to black and white
        color.rgb = vec3(color.r * 0.3 + color.g * 0.59 + color.b * 0.11);

        // Overlay green tint
        color.rgb = mix(color.rgb, vec3(0.0, 1.0, 0.0), 0.4);  // Green tint with 40% intensity

        outCol = vec4(color, 1.0);
    } else {
        outCol = vec4(resA + resD + resS, 1.0);
    }
}
