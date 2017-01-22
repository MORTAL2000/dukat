#version 140
///
// Fullscreen fx pass vertex shader.
///
in vec4 a_position;
in vec2 a_texCoord;

// Scale factor (0 for no scaling)
uniform float u_scale;

out vec2 v_texCoord;

void main()
{
    gl_Position = a_position;
    v_texCoord = a_texCoord - gl_Position.xy * u_scale;
}