#version 320

// Copy these from the fragment shader because they are not set by default here.
// Not sure if these are good values.
precision highp float;
precision highp int;
precision lowp sampler2D;
precision lowp samplerCube;

layout (location = 0) out vec4 frag_color;
void main() {
  frag_color = vec4(1, 0, 1, 1);
};
