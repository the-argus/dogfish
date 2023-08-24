#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec3 fragNormal;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Texel color fetching from texture sampler
    finalColor = vec4(fragNormal, 1.0);
}
