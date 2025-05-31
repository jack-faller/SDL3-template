#version 300 es

// Copy these from the fragment shader because they are not set by default here.
// Not sure if these are good values.
precision highp float;
precision highp int;
precision lowp sampler2D;
precision lowp samplerCube;

uniform float time;
uniform vec2 mouse_position;
uniform vec2 window_size;

layout (location = 0) out vec4 frag_color;

const float ring_count = 20.0;

void main() {
  vec2 mouse_position_fixed = mouse_position * vec2(1, -1) + vec2(0, window_size.y);
  float mouse_pixel_distance = length(mouse_position_fixed - gl_FragCoord.xy);
  float min_win = min(window_size.x, window_size.y);
  float d = min_win/mouse_pixel_distance/ring_count + fract(time / 10.0);
  d = fract(round(ring_count * d) / ring_count);
  d = abs(d * 2.0 - 1.0);
  frag_color = vec4(d, d, d, 1);
}
