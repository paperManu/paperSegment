#version 330 core

in vec4 vVertex;
in vec2 vTexCoord;

uniform mat4 vMVP;
uniform vec2 vResolution;

smooth out vec2 finalTexCoord;

void main(void)
{
    gl_Position.xyz = (vMVP*vVertex).xyz;

    vec2 lTemp = vResolution;

    finalTexCoord = vTexCoord;
}
