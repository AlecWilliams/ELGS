#include "line.h"
#include <iostream>
#include "GLSL.h"


using namespace std;

bool Line::init()
{
	//first the shaders:
	GLint rc;

	// Create shader handles
	GLuint VS = glCreateShader(GL_VERTEX_SHADER);
	GLuint FS = glCreateShader(GL_FRAGMENT_SHADER);

	// Read shader sources
	std::string vShaderString = string("#version 330 core \n\
	layout(location = 0) in vec3 vertPos; \n\
	uniform mat4 P; \n\
	uniform mat4 V; \n\
	out vec3 vertex_pos;  \n\
	void main() \n\
	{ \n\
		gl_Position = P * V * vec4(vertPos, 1.0); \n\
	} \n\
	");
	std::string fShaderString = string("#version 330 core \n\
	out vec4 color; \n\
	uniform vec4 ucolor; \n\
	void main() \n\
	{ \n\
		color = ucolor; \n\
	} \n\
	");
	const char *vshader = vShaderString.c_str();
	const char *fshader = fShaderString.c_str();
	CHECKED_GL_CALL(glShaderSource(VS, 1, &vshader, NULL));
	CHECKED_GL_CALL(glShaderSource(FS, 1, &fshader, NULL));

	// Compile vertex shader
	CHECKED_GL_CALL(glCompileShader(VS));
	CHECKED_GL_CALL(glGetShaderiv(VS, GL_COMPILE_STATUS, &rc));
	if (!rc)
	{
		GLSL::printShaderInfoLog(VS);
		std::cout << "Error compiling >Line< vertex shader " << std::endl;		
		return false;
	}

	// Compile fragment shader
	CHECKED_GL_CALL(glCompileShader(FS));
	CHECKED_GL_CALL(glGetShaderiv(FS, GL_COMPILE_STATUS, &rc));
	if (!rc)
	{
		GLSL::printShaderInfoLog(FS);
		std::cout << "Error compiling >Line< fragment shader " << std::endl;
		return false;
	}

	// Create the program and link
	pid = glCreateProgram();
	CHECKED_GL_CALL(glAttachShader(pid, VS));
	CHECKED_GL_CALL(glAttachShader(pid, FS));
	CHECKED_GL_CALL(glLinkProgram(pid));
	CHECKED_GL_CALL(glGetProgramiv(pid, GL_LINK_STATUS, &rc));
	if (!rc)
	{
		GLSL::printProgramInfoLog(pid);
		std::cout << "Error linking >Line< shaders " << std::endl;
		
		return false;
	}
	
	ucolor = GLSL::getUniformLocation(pid, "ucolor", true);
	uP = GLSL::getUniformLocation(pid, "P", true);
	uV = GLSL::getUniformLocation(pid, "V", true);

	glGenVertexArrays(1, &crossvaoID);
	glBindVertexArray(crossvaoID);	
	glGenBuffers(1, &crossBufID);
	glBindBuffer(GL_ARRAY_BUFFER, crossBufID);
	vec3 cross[4];
	cross[0] = vec3(-0.1, 0, 0);
	cross[1] = vec3(0.1, 0, 0);
	cross[2] = vec3(0,-0.1, 0);
	cross[3] = vec3(0,0.1, 0);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vec3), cross, GL_STATIC_DRAW);
	glBindVertexArray(0);
	
	
	glGenVertexArrays(1, &vaoID);
	glBindVertexArray(vaoID);
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	//glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(vec3), newPt);
	vec3 p[1000]; 
	for(int ii=0;ii<1000;ii++)
		p[ii] = vec3(0, 0, 0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*1000, p, GL_DYNAMIC_DRAW);
	segment_count = 0;


	glGenVertexArrays(1, &planeVAO);
	glBindVertexArray(planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	vector<vec3> planepos;
	float w = 2. * 3.1415269 / 20.;
	float rad = 0.005;
	planepos.push_back(vec3(sin(0), cos(0), 0) * rad);
	for (int ii = 1; ii < 20; ii++)
		{
		if(ii>1)
			planepos.push_back(planepos.back());
		planepos.push_back(vec3(sin(w*ii), cos(w*ii), 0) * rad);
		}
	planepos.push_back(planepos.back());
	planepos.push_back(planepos[0]);

	planepos.push_back(vec3(rad,0,0));
	planepos.push_back(vec3(rad*2.6, 0, 0));
	planepos.push_back(vec3(-rad, 0, 0));
	planepos.push_back(vec3(-rad * 2.6, 0, 0));

	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * planepos.size(), planepos.data(), GL_DYNAMIC_DRAW);
	planesize = planepos.size();
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
bool Line::is_active()
{
	return (segment_count >= 2);
}
//*********************************************************************************
void Line::reset()
{
	segment_count = 0;
}
bool Line::re_init_line(std::vector<vec3> &points)
{
	glBindVertexArray(vaoID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferSubData(GL_ARRAY_BUFFER, 0,points.size() * sizeof(vec3), points.data());
	segment_count = points.size();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	//assert(glGetError() == GL_NO_ERROR);
	return true;
}
//*********************************************************************************
void Line::draw(mat4 &P, mat4 &V, vec4 &colorvec4)
{	
		if (segment_count < 2)
			return;
		int h_pos = 0;
		glUseProgram(pid);
		glBindVertexArray(vaoID);
		// Bind position buffer
		GLSL::enableVertexAttribArray(h_pos);
		glBindBuffer(GL_ARRAY_BUFFER, posBufID);
		glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
		glUniform4fv(ucolor, 1, &colorvec4[0]);
		glUniformMatrix4fv(uP, 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(uV, 1, GL_FALSE, &V[0][0]);
		// Draw
		glDrawArrays(GL_LINE_STRIP, 0, segment_count );

		GLSL::disableVertexAttribArray(h_pos);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glUseProgram(0);
}

void Line::draw_plane(mat4& P, mat4& V, vec4& colorvec4)
	{

	int h_pos = 0;
	glUseProgram(pid);

	glBindVertexArray(planeVAO);	
	// Bind position buffer
	GLSL::enableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
	glUniform4fv(ucolor, 1, &colorvec4[0]);
	glUniformMatrix4fv(uP, 1, GL_FALSE, &P[0][0]);
	glUniformMatrix4fv(uV, 1, GL_FALSE, &V[0][0]);
	// Draw
	glDrawArrays(GL_LINES, 0, planesize);

	GLSL::disableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glUseProgram(0);
	}

void Line::draw_cross(mat4& P, mat4& V, vec4& colorvec4)
	{

	int h_pos = 0;
	glUseProgram(pid);

	glBindVertexArray(crossvaoID);
	// Bind position buffer
	GLSL::enableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, crossBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
	glUniform4fv(ucolor, 1, &colorvec4[0]);
	glUniformMatrix4fv(uP, 1, GL_FALSE, &P[0][0]);
	glUniformMatrix4fv(uV, 1, GL_FALSE, &V[0][0]);
	// Draw
	glDrawArrays(GL_LINES, 0, 4);

	GLSL::disableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glUseProgram(0);
	}


void cardinal_curve(vector<vec3> &result_path, vector<vec3> &original_path,  int lod, float curly)
{
	
	if (original_path.size()<3) return;
	result_path.clear();
	int n1 = original_path.size() + 1;
	vec3 *P, *d, *A, first, last;
	vec4 *B;

	double *Bi = NULL;
	P = new vec3[n1];
	d = new vec3[n1];
	A = new vec3[n1];
	B = new vec4[lod];

	//punkte setzen
	for (int ii = 0; ii<original_path.size(); ii++)
		P[ii] = original_path[ii];



	d[0] = vec3(0,0,0);
	d[original_path.size() - 1] = vec3(0,0,0);

	Bi = new double[n1];
	double t = 0;
	double tt = 1. / (lod - 1.);
	for (int i = 0; i< lod; i++)
	{
		double t1 = 1 - t, t12 = t1*t1, t2 = t*t;
		B[i] = vec4(t1*t12, 3 * t*t12, 3 * t2*t1, t*t2);
		t += tt;
	}

	//findpoints
	Bi[1] = -0.25*(4. / curly);

	A[1].x = (P[2].x - P[0].x - d[0].x) / 4;
	A[1].y = (P[2].y - P[0].y - d[0].y) / 4;
	A[1].z = (P[2].z - P[0].z - d[0].z) / 4;
	for (int i = 2; i < original_path.size() - 1; i++)
	{
		Bi[i] = -1 / (4 + Bi[i - 1]);
		A[i].x = -(P[i + 1].x - P[i - 1].x - A[i - 1].x)*Bi[i];
		A[i].y = -(P[i + 1].y - P[i - 1].y - A[i - 1].y)*Bi[i];
		A[i].z = -(P[i + 1].z - P[i - 1].z - A[i - 1].z)*Bi[i];
	}
	for (int i = original_path.size() - 2; i > 0; i--)
	{
		d[i].x = A[i].x + d[i + 1].x*Bi[i];
		d[i].y = A[i].y + d[i + 1].y*Bi[i];
		d[i].z = A[i].z + d[i + 1].z*Bi[i];
	}
	//points

	float X, Y, Z;
	float Xo = (float)P[0].x;
	float Yo = (float)P[0].y;
	float Zo = (float)P[0].z;



	result_path.push_back(vec3(Xo, Yo, 0));
	
	int ii = 0;
	for (int i = 0; i < original_path.size() - 1; i++)
	{
		for (int k = 0; k < lod; k++)
		{
			X = (P[i].x						*	B[k].x
				+ (P[i].x + d[i].x)	*	B[k].y
				+ (P[i + 1].x - d[i + 1].x)	*	B[k].z
				+ P[i + 1].x					*	B[k].w);

			Y = (P[i].y						*	B[k].x
				+ (P[i].y + d[i].y)		*	B[k].y
				+ (P[i + 1].y - d[i + 1].y)	*	B[k].z
				+ P[i + 1].y					*	B[k].w);

			Z = (P[i].z						*	B[k].x
				+ (P[i].z + d[i].z)		*	B[k].y
				+ (P[i + 1].z - d[i + 1].z)	*	B[k].z
				+ P[i + 1].z					*	B[k].w);

			if (i == 0 && k == 0)	
				result_path.push_back(vec3(X, Y, Z));
			if (k>0)				
				result_path.push_back(vec3(X, Y, Z));
			
		}
	}


	if (P != NULL)	delete[] P;
	if (d != NULL)	delete[] d;
	if (A != NULL)	delete[] A;
	if (B != NULL)	delete[] B;
	if (Bi)			delete[] Bi;

}