#version 430
precision mediump float;

in vec2 v_texcoord;

out vec4 frag_color;

uniform int u_color_type;
uniform sampler2D u_galaxy;
uniform sampler2D u_blur;

void main()
{
    float stars = texture(u_galaxy, v_texcoord).r;
    float glow = texture(u_blur, v_texcoord).r * 0.2;
    
    vec3 color;
    
    if (u_color_type == 0) {
        color = vec3(0.1, 0.7, 1.0) * (stars + glow);
    } else if (u_color_type == 1) {
        color = vec3(1.0, 0.1, 0.1) * (stars + glow);
    } else {
        color = vec3(0.8, 0.2, 1.0) * (stars + glow);
    }
    
    color += vec3(1.0, 1.0, 1.0) * stars * 0.5;
    
    color = pow(color, vec3(0.9)); 
    
    frag_color = vec4(color, 1.0);
}