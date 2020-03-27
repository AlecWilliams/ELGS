#version 330 core
out vec4 color;
in vec3 vertex_pos;
in vec2 vertex_tex;
uniform vec3 campos;

uniform sampler2D tex;

uniform vec2 offset;

void main()
{
vec2 texcoord = vertex_tex;///3. + offset;
color = texture(tex, texcoord);


}
