#version 430
precision mediump float;

in float v_brightness;

out vec4 frag_color;

void main()
{
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float dist = dot(circCoord, circCoord);
    
    if (dist > 1.0) discard;
    
    float alpha = smoothstep(1.0, 0.0, dist) * v_brightness;
    
    vec3 color = mix(vec3(0.8, 0.9, 1.0), vec3(1.0, 0.7, 0.3), v_brightness);
    
    frag_color = vec4(color, alpha);
}