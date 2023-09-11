#version 330 core
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexTexCoord;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    vec4 worldPos = model * vec4(vertexPosition, 1.0);
    FragPos = worldPos.xyz; 
    TexCoords = vertexTexCoord;
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalMatrix * vertexNormal;

    gl_Position = proj * view * worldPos;
}
