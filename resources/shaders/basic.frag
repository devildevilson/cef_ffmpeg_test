#version 330 core

uniform sampler2D tex;

in vec2 uv_coord;
out vec4 FragColor;

void main() {
  //FragColor = vec4(1.0, 0.5, 0.2, 1.0);
  FragColor = texture(tex, uv_coord);
}