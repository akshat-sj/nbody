#version 430
precision mediump float;

layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_mass;

out float v_brightness;

uniform mat4 u_mvp;

void main()
{
    v_brightness = a_mass / 10.0;
    gl_PointSize = a_mass * 2.0 + 1.0;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}