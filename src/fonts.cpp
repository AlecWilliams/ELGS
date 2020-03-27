#include "fonts.h"
#include <iostream>

#include "GLSL.h"
#include "Program.h"
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;
//-----------------------------------------------------------------------------
bool bmpfont::init()
	{
	
	//texture **********************************************************************************
	int width, height, channels;
	unsigned char* data = stbi_load("../resources/cleanfont.png", &width, &height, &channels, 4);
	glGenTextures(1, &textureID);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	//letter rectangle **********************************************************************************
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	// position VBO
	glGenBuffers(1, &vbo_pos);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
	vec3 vertices[6];
	vertices[1] = vec3(0, 0, 0);
	vertices[0] = vec3(1, 0, 0);
	vertices[2] = vec3(0, -1, 0);
	vertices[4] = vec3(1, 0, 0);
	vertices[3] = vec3(1, -1, 0);
	vertices[5] = vec3(0, -1, 0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 6, vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// texpos VBO
	glGenBuffers(1, &vbo_tex);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_tex);
	vec2 texpos[6];
	float asciibmp_factxy = 0.1;
	texpos[1] = vec2(0, 0)*asciibmp_factxy;
	texpos[0] = vec2(1, 0)*asciibmp_factxy;
	texpos[2] = vec2(0, 1)*asciibmp_factxy;
	texpos[4] = vec2(1, 0)*asciibmp_factxy;
	texpos[3] = vec2(1, 1)*asciibmp_factxy;
	texpos[5] = vec2(0, 1)*asciibmp_factxy;
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * 6, texpos, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindVertexArray(0);
	//shaders **********************************************************************************
	GLuint VS = glCreateShader(GL_VERTEX_SHADER);
	GLuint FS = glCreateShader(GL_FRAGMENT_SHADER);
	const char* vshader = R"glsl(
#version 330 
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;
out vec2 frag_tex;
uniform vec4 offset_pos;
uniform vec2 offset_texpos;
void main()
{
	vec4 pos =  vec4(vertPos, 1.0);
	pos.xy *= offset_pos.w;//scaling
	pos.xyz += offset_pos.xyz;
	gl_Position = pos;
	frag_tex = vertTex + offset_texpos;
}
)glsl";
	const char* fshader = R"glsl(
#version 330 
out vec4 color;
in vec2 frag_tex;
uniform sampler2D tex;
uniform vec3 texcolor;
void main()
{
vec4 tcol = texture(tex, frag_tex);
color = tcol;
if(length(color.rgb) > 0.5)
{
	color.rgb = texcolor;
//vec3(1, 0.8, 0);
	color.a = 0.9;
}
else
	discard;

//color.a = (color.r+color.g+color.b)/3.0;
}
)glsl";
	CHECKED_GL_CALL(glShaderSource(VS, 1, &vshader, NULL));
	CHECKED_GL_CALL(glShaderSource(FS, 1, &fshader, NULL));
	// Compile vertex shader
	GLint rc;
	CHECKED_GL_CALL(glCompileShader(VS));
	CHECKED_GL_CALL(glGetShaderiv(VS, GL_COMPILE_STATUS, &rc));
	if (!rc)
		{
		GLSL::printShaderInfoLog(VS);
		std::cout << "Error compiling vertex shader " << std::endl;
		return false;
		}

	// Compile fragment shader
	CHECKED_GL_CALL(glCompileShader(FS));
	CHECKED_GL_CALL(glGetShaderiv(FS, GL_COMPILE_STATUS, &rc));
	if (!rc)
		{
		GLSL::printShaderInfoLog(FS);
		std::cout << "Error compiling fragment shader " << std::endl;
		return false;
		}
	pid = glCreateProgram();
	CHECKED_GL_CALL(glAttachShader(pid, VS));
	CHECKED_GL_CALL(glAttachShader(pid, FS));
	CHECKED_GL_CALL(glLinkProgram(pid));
	CHECKED_GL_CALL(glGetProgramiv(pid, GL_LINK_STATUS, &rc));
	if (!rc)
		{
		GLSL::printProgramInfoLog(pid);
		std::cout << "Error linking bmpfont shaders " << std::endl;
		return false;
		}
	uniform_tex = glGetUniformLocation(pid, "tex");
	uniform_pos_offset = glGetUniformLocation(pid, "offset_pos");
	uniform_tex_offset = glGetUniformLocation(pid, "offset_texpos");
	uniform_tex_color = glGetUniformLocation(pid, "texcolor");
	return true;
	}
//-----------------------------------------------------------------------------
void bmpfont::draw(float x, float y, float z, std::string text,float r, float g, float b)
	{
	fontpositios.push_back(fontpos(x,y,z,text, r,g,b));
	}
void bmpfont::draw()
{

	glDisable(GL_DEPTH_TEST);
	CHECKED_GL_CALL(glUseProgram(pid));
	glBindVertexArray(vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	for (int i = 0; i < fontpositios.size(); i++)
		{
		vec4 offset_position = vec4(fontpositios[i].x, fontpositios[i].y, fontpositios[i].z, fontsize);
		int letters = fontpositios[i].t.size();
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		for (int t = 0; t < letters; t++)
			{
			glUniform4fv(uniform_pos_offset, 1, &offset_position.x);
			fontvec3 offset_texposition = get_texturecoord_offset(fontpositios[i].t[t]);
			glUniform3fv(uniform_tex_color, 1, fontpositios[i].color);
			glUniform2fv(uniform_tex_offset, 1, &offset_texposition.x);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			offset_position.x += fontsize * offset_texposition.width;
			}
		}
	glBindVertexArray(0);
	CHECKED_GL_CALL(glUseProgram(0));
	glEnable(GL_DEPTH_TEST);
	fontpositios.clear();
}
//-----------------------------------------------------------------------------
fontvec3 bmpfont::get_texturecoord_offset(char c)
{
	double charOffsets[] = {0.5, 0.35, 0.35, 0.7, 0.7, 0.7, 0.7, 0.25, 0.35, 0.35,
							0, 0, 0, 0.35, 0.23, 0, 0.57, 0.57, 0.57, 0.57,
							0.57, 0.57, 0.57, 0.57, 0.57, 0.57, 0.2, 0, 0, 0,
							0, 0, 0, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7,
							0.7, 0.2, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.65, 0.7,
							0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0,
							0, 0, 0, 0, 0, 0.6, 0.6, 0.55, 0.6, 0.6,
							0.4, 0.55, 0.55, 0.2, 0.35, 0.35, 0.2, 0.85, 0.5, 0.6,
							0.55, 0.35, 0.35, 0.55, 0.35, 0.55, 0.55, 0.85, 0.55, 0.5, 
							0.35, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	fontvec3 off;
	off.x = 0;
	off.y = 0;
	off.width = charOffsets[(unsigned int)c - 32];/*0.55; /* 0.35;*/
	int asciiindex = c - 32;
	if (asciiindex <= 0 || asciiindex >=100) return off;
	//if (c >= 65 && c <= 90) off.width = 0.7;
	int x = asciiindex % 10;
	int y = asciiindex / 10;
	off.x = (float)x * 0.1;
	off.y = (float)y * 0.1;
	return off;
}