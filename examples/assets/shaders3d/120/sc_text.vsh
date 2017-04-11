#version 120
///
// Vertex shader for 2D text.
///
uniform mat4 u_cam_proj_pers;
uniform mat4 u_cam_proj_orth;
uniform mat4 u_cam_view;
uniform mat4 u_cam_view_inv;
uniform vec4 u_cam_position;
uniform vec4 u_cam_dir;
uniform vec4 u_cam_up;
uniform vec4 u_cam_left;

uniform mat4 u_model;

void main()
{
	gl_FrontColor = gl_Color;
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_Position = u_cam_proj_orth * u_model * vec4(gl_Vertex.x, -gl_Vertex.y, 0.0, 1.0);
}