#if defined VERT

void main()
{
    gl_Position = vec4(-1.0 + float((gl_VertexID & 1) << 2), -1.0 + float((gl_VertexID & 2) << 1), 0.0, 1.0);
}

#elif defined FRAG

layout(location = 0) out vec3 outColor;

layout(location = 0) uniform ivec2 axis;

layout(binding = 4) uniform sampler2D sourceTexture;

#if SSAO_QUALITY == 1
#define SCALE 2
#else
#define SCALE 1
#endif
#define EDGE_SHARPNESS 1.0
#define R 3
#define VALUE_TYPE float
#define VALUE_COMPONENTS r
#define VALUE_IS_KEY 0
#define KEY_COMPONENTS gb

const float gaussian[R + 1] =
float[](0.356642, 0.239400, 0.072410, 0.009869);
//float[](0.398943, 0.241971, 0.053991, 0.004432, 0.000134);  // stddev = 1.0
//float[](0.153170, 0.144893, 0.122649, 0.092902, 0.062970);  // stddev = 2.0
//float[](0.111220, 0.107798, 0.098151, 0.083953, 0.067458, 0.050920, 0.036108); // stddev = 3.0

float unpackKey(vec2 p)
{
    return p.x * (256.0 / 257.0) + p.y * (1.0 / 257.0);
}

void main()
{
#if 1
    ivec2 ssC = ivec2(gl_FragCoord.xy);

    vec4 temp = texelFetch(sourceTexture, ssC, 0);

    outColor.KEY_COMPONENTS = temp.KEY_COMPONENTS;
    float key = unpackKey(outColor.KEY_COMPONENTS);

    VALUE_TYPE sum = temp.VALUE_COMPONENTS;

    float BASE = gaussian[0];
    float totalWeight = BASE;
    sum *= totalWeight;

    ivec2 texSize = textureSize(sourceTexture, 0);
    for (int r = -R; r <= R; ++r)
    {
        if (r != 0)
        {
            ivec2 coord = clamp(ssC + axis * (r * SCALE), ivec2(0, 0), texSize - 1);
            // original
            //ivec2 coord = ssC + axis * (r * SCALE);
            temp = texelFetch(sourceTexture, coord, 0);
            float tapKey = unpackKey(temp.KEY_COMPONENTS);
            VALUE_TYPE value = temp.VALUE_COMPONENTS;
            float weight = 0.3 + gaussian[abs(r)];
            weight *= max(0.0, 1.0 - (EDGE_SHARPNESS * 2000.0) * abs(tapKey - key));
            sum += value * weight;
            totalWeight += weight;
        }
    }

    const float epsilon = 0.0001;
    outColor.VALUE_COMPONENTS = sum / (totalWeight + epsilon);
#else
    ivec2 ssC = ivec2(gl_FragCoord.xy);
    outColor.r = texelFetch(sourceTexture, ssC, 0).r;
#endif
}
#endif
