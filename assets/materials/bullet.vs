#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

in vec3 bulletPosition;
in vec4 bulletVelocity; // quaternion

// Input uniform values
uniform mat4 matNormal;
uniform mat4 mvp;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;

// NOTE: Add here your custom variables

void main()
{
    // TODO: test transposing rotation matrix
    mat4 rot;
    rot[0].xyzw = vec4(1.0f, 0.0f, 0.0f, 0.0f);
    rot[1].xyzw = vec4(0.0f, 1.0f, 0.0f, 0.0f);
    rot[2].xyzw = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    rot[3].xyzw = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    float a2 = bulletVelocity.x * bulletVelocity.x;
    float b2 = bulletVelocity.y * bulletVelocity.y;
    float c2 = bulletVelocity.z * bulletVelocity.z;
    float ac = bulletVelocity.x * bulletVelocity.z;
    float ab = bulletVelocity.x * bulletVelocity.y;
    float bc = bulletVelocity.y * bulletVelocity.z;
    float ad = bulletVelocity.w * bulletVelocity.x;
    float bd = bulletVelocity.w * bulletVelocity.y;
    float cd = bulletVelocity.w * bulletVelocity.z;
    
    // m0
    rot[0].x = 1 - 2*(b2 + c2);
    // m1
    rot[1].x = 2*(ab + cd);
    // m2
    rot[2].x = 2*(ac - bd);

    // m4
    rot[0].y = 2*(ab - cd);
    // m5
    rot[1].y = 1 - 2*(a2 + c2);
    // m6
    rot[2].y = 2*(bc + ad);
    
    rot[0].z = 2*(ac + bd);
    rot[1].z = 2*(bc - ad);
    rot[2].z = 1 - 2*(a2 + b2);

    mat4 pos;
    pos[0].xyzw = vec4(1.0f, 0.0f, 0.0f, 0.0f);
    pos[1].xyzw = vec4(0.0f, 1.0f, 0.0f, 0.0f);
    pos[2].xyzw = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    pos[3].xyzw = vec4(bulletPosition.x, bulletPosition.y, bulletPosition.z, 1.0f);

    //mat4 mvpi = pos * viewMat * projMat;
    mat4 mvpi = mvp * pos * rot;

    // Send vertex attributes to fragment shader
    vec4 posWithVertex = mvpi*vec4(vertexPosition, 1.0);
    fragPosition = vec3(posWithVertex);
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));

    // Calculate final vertex position
    gl_Position = posWithVertex;
}
