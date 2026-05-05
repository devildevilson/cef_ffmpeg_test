#version 330 core
#extension GL_ARB_shading_language_420pack : enable

const vec4 tri_arr[] = {
  // большой треугольник, клип спейс полностью покрыт стандартными координатами
  vec4(-3.0f,  1.0f, 0.0, 1.0),
  vec4( 1.0f,  1.0f, 0.0, 1.0),
  vec4( 1.0f, -3.0f, 0.0, 1.0),
};

const vec2 uv_arr[] = {
  vec2(-1.0f, 0.0f),
  vec2( 1.0f, 0.0f),
  vec2( 1.0f, 2.0f),
};

//layout (location = 0) in vec2 aPos;
out vec2 uv_coord;

void main() {
  //gl_Position = vec4(aPos, 0.0, 1.0);
  gl_Position = tri_arr[gl_VertexID];
  uv_coord = uv_arr[gl_VertexID];
}