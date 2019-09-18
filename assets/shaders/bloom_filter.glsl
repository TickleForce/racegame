#if defined VERT

layout(location = 0) out vec2 outTexCoord;

void main()
{
    gl_Position = vec4(-1.0 + float((gl_VertexID & 1) << 2), -1.0 + float((gl_VertexID & 2) << 1), 0.0, 1.0);
    outTexCoord = vec2((gl_Position.x + 1.0) * 0.5, (gl_Position.y + 1.0) * 0.5);
}

#elif defined FRAG

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2DArray tex;

void main()
{
    vec4 color = texture(tex, vec3(inTexCoord, gl_Layer));
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 0.99)
    {
        outColor = color;
    }
    else
    {
        outColor = vec4(color.rgb * 0.125, 1);
    }
}

#elif defined GEOM

layout(location = 0) in vec2 inTexCoord[];

layout(location = 0) out vec2 outTexCoord;

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = gl_in[i].gl_Position;
        outTexCoord = inTexCoord[i];
        EmitVertex();
    }

    EndPrimitive();
}

#endif