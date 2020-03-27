#pragma once


#include <string>
#include <vector>
#include <memory>
#include <iostream>
using namespace std;
class Program;
struct fontvec3
	{
	float x, y;
	float width;
	};
class fontpos
	{
	public:
	float x, y, z;
	string t;
	float color[3];
	fontpos(float xp,float yp, float zp, string text,  float r, float g, float b)
		{
		color[0] = r;
		color[1] = g;
		color[2] = b;
		t = text;
		y = yp; 
		x = xp;
		z = zp;
		}
	};
class bmpfont
{

public:
	//stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp)
	bool init();
	void draw(float x, float y, float z, std::string text, float r, float g, float b);
	void draw();
	void set_font_size(float fs) { fontsize = fs; }
private:
	vector<fontpos> fontpositios;
	fontvec3 get_texturecoord_offset(char c);
	float fontsize = 0.06;//rel to win size

	unsigned int vao = 0;
	unsigned int vbo_pos = 0;
	unsigned int vbo_tex = 0;
	unsigned int textureID = 0;
	unsigned int pid = 0;

	unsigned int attrib_pos = 0;
	unsigned int attrib_tex = 0;
	unsigned int uniform_tex = 0;
	unsigned int uniform_pos_offset = 0;
	unsigned int uniform_tex_offset = 0; 
	unsigned int uniform_tex_color = 0;
};


