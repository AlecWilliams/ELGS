


#pragma once

#ifndef LAB474_LINE_H_INCLUDED
#define LAB474_LINE_H_INCLUDED

#include <string>
#include <vector>
#include <memory>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;
using namespace std;



class Program;

class Line
{

public:
	//stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp)
	bool init();
	bool re_init_line(std::vector<vec3> &points);
	void draw(mat4 &P, mat4 &V, vec4 &colorvec3);
	bool is_active();
	void reset();
	void draw_cross(mat4& P, mat4& V, vec4& colorvec4);
	void draw_plane(mat4& P, mat4& V, vec4& colorvec4);
private:
	int segment_count = 0;
	int planesize = 0;
	unsigned int posBufID = 0;	
	unsigned int crossvaoID = 0;
	unsigned int crossBufID = 0;
	unsigned int planeVAO = 0;
	unsigned int planeVBO = 0;
	unsigned int vaoID;
	unsigned int pid;
	unsigned int ucolor,uP,uV;
};
void cardinal_curve(vector<vec3> &result_path, vector<vec3> &original_path, int lod, float curly);

#endif // LAB471_SHAPE_H_INCLUDED
