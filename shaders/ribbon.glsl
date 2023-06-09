#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec4 attrColor;
layout(location = 3) in vec2 attrTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outWorldPosition;
layout(location = 4) out vec3 outShadowCoord;

void main()
{
    outColor = attrColor;
    outNormal = attrNormal;
    outTexCoord = attrTexCoord;
    outWorldPosition = attrPosition;
    gl_Position = cameraViewProjection * vec4(outWorldPosition, 1.0);
    outShadowCoord = (shadowViewProjectionBias * vec4(outWorldPosition, 1.0)).xyz;
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inWorldPosition;
layout(location = 4) in vec3 inShadowCoord;

layout(binding = 0) uniform sampler2D texSampler;

void main()
{
    outColor = lighting(texture(texSampler, inTexCoord) * inColor, normalize(inNormal), inShadowCoord,
            inWorldPosition, 0.0, 0.0, vec3(1.0), 0.0, 0.0, 0.0, vec3(0, 0, 0), 0.0, 0.0, 0.0);
}
#endif
