#version 320

// These are the default values, they're just here for reference.
precision highp float;
precision highp int;
precision lowp sampler2D;
precision lowp samplerCube;

in vec3 pos;
void main() {
  gl_Position = vec4(pos, 1);
}
