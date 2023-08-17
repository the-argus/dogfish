#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragPosition;
in vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

//
layout(location = 0) out vec4 diffuse;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 depth;

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);

    // diffuse = texelColor*colDiffuse;
    // diffuse = vec4(1.0, 0.0, 0.0, 1.0);
    diffuse = vec4(fragNormal, 1.0);
    normal = vec4(1.0, 0.0, 0.0, 1.0);
    depth = vec4(fragPosition.z, fragPosition.z, fragPosition.z, 1.0);
}

