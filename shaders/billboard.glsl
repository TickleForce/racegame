#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec2 outTexCoord;

const vec2 vertices[6] = vec2[](
    vec2(-1, -1),
    vec2( 1, -1),
    vec2( 1,  1),

    vec2( 1,  1),
    vec2(-1,  1),
    vec2(-1, -1)
);

const vec2 uvs[6] = vec2[](
    vec2(0, 0),
    vec2(1, 0),
    vec2(1, 1),

    vec2(1, 1),
    vec2(0, 1),
    vec2(0, 0)
);

void main()
{
    outPosition = vec3(vertices[gl_VertexID], 0.0);
    outTexCoord = uvs[gl_VertexID];
}

#elif defined FRAG

#if defined LIT
#define NO_SSAO
#include "lighting.glsl"
#endif

layout(location = 0) in vec2 inTexCoord;
#if defined LIT
layout(location = 1) in vec3 inWorldPosition;
layout(location = 2) in vec3 inShadowCoord;
#endif

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform sampler2DArray depthSampler;

layout(location = 0) uniform vec4 color;

void main()
{
    outColor = texture(texSampler, inTexCoord) * color;
#if defined LIT
    outColor = lighting(outColor,
            vec3(0, 0, 1), inShadowCoord, inWorldPosition, 0.f, 0.f, vec3(1.0),
            0.f, 0.f, 0.f, vec3(0.f));
#endif

    float depth = texture(depthSampler, vec3(gl_FragCoord.xy / textureSize(depthSampler, 0).xy, gl_Layer)).r;
    float d = depth - gl_FragCoord.z;
    outColor.a *= clamp(d, 0.0, 0.002) * 800.0;
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inPosition[];
layout(location = 1) in vec2 inTexCoord[];

layout(location = 0) out vec2 outTexCoord;
#if defined LIT
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outShadowCoord;
#endif

layout(location = 1) uniform mat4 translation;
layout(location = 2) uniform vec3 scale;
layout(location = 3) uniform mat4 rotation;

void main()
{
    mat4 modelView = cameraView[gl_InvocationID] * translation;
    modelView[0][0] = scale.x;
    modelView[0][1] = 0;
    modelView[0][2] = 0;
    modelView[1][0] = 0;
    modelView[1][1] = scale.y;
    modelView[1][2] = 0;
    modelView[2][0] = 0;
    modelView[2][1] = 0;
    modelView[2][2] = scale.z;
    mat4 matrix = cameraProjection[gl_InvocationID] * modelView * rotation;

    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = matrix * vec4(inPosition[i], 1.0);
        outTexCoord = inTexCoord[i];
#if defined LIT
        outWorldPos = (translation * rotation * vec4(inPosition[i], 1.0)).xyz;
        outShadowCoord = (shadowViewProjectionBias[gl_InvocationID]
                * (translation * rotation * vec4(inPosition[i], 1.0))).xyz;
#endif
        EmitVertex();
    }

    EndPrimitive();
}

#endif