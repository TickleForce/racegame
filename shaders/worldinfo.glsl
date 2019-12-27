layout (std140, binding = 0) uniform WorldInfo
{
    mat4 orthoProjection;
    vec3 sunDirection;
    float time;
    vec3 sunColor;
    mat4 cameraViewProjection;
    mat4 cameraProjection;
    mat4 cameraView;
    vec3 cameraPosition;
    mat4 shadowViewProjectionBias;
};

