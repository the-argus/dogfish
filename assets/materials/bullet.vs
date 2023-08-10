#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

in vec3 bulletPosition;
in vec4 bulletVelocity; // quaternion

// Input uniform values
uniform mat4 mvp;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;

// NOTE: Add here your custom variables

void main()
{
    // Compute MVP for current instance
    // TODO: factor in bullet velocity
    mat4 mvpi = mvp;

    // Send vertex attributes to fragment shader
    vec4 pos = mvpi*vec4(vertexPosition + bulletPosition, 1.0);
    fragPosition = vec3(pos);
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));

    // Calculate final vertex position
    gl_Position = pos;
}
