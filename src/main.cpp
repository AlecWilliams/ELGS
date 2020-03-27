/*
Emergency Landing Guidance System (ELGS)
Alec Williams, Christian Eckhardt
*/


#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <mutex>
#include <thread>
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include <algorithm>
#include "findpath.h"
#include "fonts.h"
#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


using namespace std;
using namespace glm;

#define PI 3.14159
shared_ptr<Shape> shape;

//Yikes, global variable bad. fix later!
float currentHeight = 0.17f;
float currentSpeed = 0.08;
static float scaleFactor = 8.1f;
static float scaleRatio = scaleFactor / 2.1;

struct landingPath
	{
	vec2 a;
	vec2 b;
	float length;
	int closestPoint = 0;
	float landingCondition = 0;
	};

class path_n_landing_
	{
	public:
		vector<vec3> path;
		vec3 strip_a, strip_b;
		path_n_landing_() {}

	};
struct landingLine
	{
	vector<vec3> line;
	};
class posdir_
	{
	public:
		float dotp = 0;
		vec3 pos_a, pos_b, dir;
		posdir_()
			{
			pos_a = pos_b= dir = vec3(0, 0, 0);
			}
		posdir_(vec3 p1, vec3 p2, vec3 d, float dotp_ = 0)
			{
			pos_a = p1;
			pos_b = p2;
			dir = d;
			dotp = dotp_;
			}
	};
bool compareLandings(posdir_ i1, posdir_ i2)
	{
	return (i1.dotp < i2.dotp);
	}
vector<vec3> greenstuff;
int height, width;
vector<landingPath> paths;

class draw_landingstrip_
	{
	private:
		unsigned int pid;
		unsigned int ucolor, uP, uV;
		vec4 linecolor, tricolor;
	public:
		draw_landingstrip_() 
			{
			linecolor = vec4(0, 0.6, 0.6, 1);
			tricolor = vec4(0, 0.6, 0.6, 0.6);
			}
		GLuint VAO, VBO,IBOtri,IBOlines;
		void re_init(vec3 a, vec3 b,vec4 colorA,vec4 colorB)
			{
			linecolor = colorA;
			tricolor = colorB;
			vec3 edges[5];
			vec3 direction = b - a;
			direction.z = 0;
			vec3 sidevec = normalize(vec3(direction.y, -direction.x, 0)) * 0.03f;
			edges[0] = a + sidevec;
			edges[1] = b + sidevec;
			edges[2] = b - sidevec;
			edges[3] = a - sidevec;
			edges[4] = a + sidevec;
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * 5, edges);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			}
		void re_init_rect(vec3 a, vec3 b, vec4 colorA, vec4 colorB)
			{
			linecolor = colorA;
			tricolor = colorB;
			vec3 edges[5];
			edges[0] = a;
			edges[1] = vec3(b.x,a.y,a.z);
			edges[2] = b;
			edges[3] = vec3(a.x, b.y, a.z);
			edges[4] = a;
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * 5, edges);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			}
		bool init_generic(vec3 a, vec3 b)
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
			const char* vshader = vShaderString.c_str();
			const char* fshader = fShaderString.c_str();
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
				std::cout << "Error linking >landing< shaders " << std::endl;

				return false;
				}

			ucolor = GLSL::getUniformLocation(pid, "ucolor", true);
			uP = GLSL::getUniformLocation(pid, "P", true);
			uV = GLSL::getUniformLocation(pid, "V", true);

			//--------------------------------
			vec3 edges[5];
			vec3 direction = b - a;
			direction.z = 0;
			vec3 sidevec = normalize(vec3(direction.y, -direction.x, 0))*0.01f;
			edges[0] = a + sidevec;
			edges[1] = b + sidevec;
			edges[2] = b - sidevec;
			edges[3] = a - sidevec;
			edges[4] = a + sidevec;
			unsigned int indices_tri[6];
			unsigned int indices_linestrip[5];
			indices_tri[0] = 1;
			indices_tri[1] = 0;
			indices_tri[2] = 2;
			indices_tri[3] = 0;
			indices_tri[4] = 2;
			indices_tri[5] = 3;
			for(int ii=0;ii<5;ii++)
				indices_linestrip[ii] = ii;	
			glGenVertexArrays(1, &VAO);
			glBindVertexArray(VAO);
			glGenBuffers(1, &VBO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 5, edges, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			//-----------------------------------------------------------------------------
			glGenBuffers(1, &IBOtri);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOtri);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices_tri), indices_tri, GL_STATIC_DRAW);
			//-----------------------------------------------------------------------------
			glGenBuffers(1, &IBOlines);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOlines);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices_linestrip), indices_linestrip, GL_STATIC_DRAW);

			glBindVertexArray(0);
			return true;
			}
//-------------------------------------------------------------------
		void draw(mat4& P, mat4& V)
			{
			int h_pos = 0;
			glUseProgram(pid);
			// Bind position buffer
			glBindVertexArray(VAO);
			GLSL::enableVertexAttribArray(h_pos);			
			glUniformMatrix4fv(uP, 1, GL_FALSE, &P[0][0]);
			glUniformMatrix4fv(uV, 1, GL_FALSE, &V[0][0]);
			glUniform4fv(ucolor, 1, &tricolor[0]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOtri);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
			glUniform4fv(ucolor, 1, &linecolor[0]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOlines);
			glDrawElements(GL_LINE_STRIP, 5, GL_UNSIGNED_INT, (const void*)0);
			GLSL::disableVertexAttribArray(h_pos);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			glUseProgram(0);
			}

		void draw()
			{
			glBindVertexArray(VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOtri);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
			glBindVertexArray(0);			
			}

	};

double get_last_elapsed_time()
	{
	static double lasttime = glfwGetTime();
	double actualtime = glfwGetTime();
	double difference = actualtime - lasttime;
	lasttime = actualtime;
	return difference;
	}
class camera
	{
	public:
		glm::vec3 pos, rot;
		glm::vec4 dir;
		vec3 currentDir;
		int w, a, s, d, q, e;
		camera()
			{
			w = a = s = d = q = e = 0;
			rot = glm::vec3(0, 0, 0);
			pos = glm::vec3(0, 0, -5);
			currentDir = vec3(0, -1, 0);
			}
		glm::mat4 process(double ftime)
			{
			float speed = currentSpeed * ftime;
			if (w == 1)
				{
				speed = 0.8 * ftime;
				}
			else if (s == 1)
				{
				speed =  -0.8 * ftime;
				}
			float yangle = 0;
			rot.y += yangle;
			float zangle = 0;
			if (q == 1)
				zangle = -0.6 * ftime;
			else if (e == 1)
				zangle = 0.6 * ftime;
			rot.z += zangle;
			glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
			glm::mat4 Rz = glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));
			dir = glm::vec4(0, speed, 0, 1);

			R = Rz * R;
			dir = dir * R;
			currentDir = dir;
			//glm::vec4 dir = glm::vec4(0, 0, speed,1);
			//dir = dir*R;
			pos -= glm::vec3(dir.x, dir.y, dir.z);
			glm::mat4 T = glm::translate(glm::mat4(1), pos);
			return R * T;
			}


	};

vector<vec3> controlpoints;
bool curve_unvalid = false;
camera mycam;

class Application : public EventCallbacks
	{

	public:
		bool running = true;
		WindowManager* windowManager = nullptr;

		// Our shader program
		std::shared_ptr<Program> prog, plane, linesshader;

		// Contains vertex information for OpenGL
		GLuint VertexArrayID;
		GLuint VAO, VBO;
		GLuint VAOP, VBOP;
		// Data necessary to give our box to OpenGL
		GLuint VertexBufferID, VertexColorIDBox, IndexBufferIDBox;

		//texture data
		GLuint Texture;
		GLuint Texture2;

		vec2 offset;
		float offX, offY;
		vec2 viewPos;

		bmpfont plane_height_text;
		bmpfont plane_speed_text,plane_indist_text;


		float heightOff = 0;
		float rotation;
		vec3 movement;
		vector<landingPath> viablePaths;
		vector<landingLine> lines;
		vector<Line> drawLines;
		Line drawplane;
		landingLine ll1, ll2, ll3, ll4, ll5;
		Line l1, l2, l3, l4, l5;
		draw_landingstrip_ draw_landingstrips[3];
		draw_landingstrip_ landingrect;

		float glidingRatio = 12.0f;
		int gliding_on = 0;
		float mindist = 1000;


		void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
			{
			if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
				{
				glfwSetWindowShouldClose(window, GL_TRUE);
				}

			if (key == GLFW_KEY_G && action == GLFW_PRESS)
				{


				}
			if (key == GLFW_KEY_G && action == GLFW_RELEASE)
				{
				gliding_on = !gliding_on;
				if(currentHeight <= 0.05)
					currentHeight = 0.17f;
				currentSpeed = 0.09;
				}

			if (key == GLFW_KEY_W && action == GLFW_PRESS)
				{
				mycam.w = 1;
				//offset = vec2(0, -0.005);
				//offY = -0.005;
				}
			if (key == GLFW_KEY_W && action == GLFW_RELEASE)
				{
				mycam.w = 0;
				offset = vec2(0, 0);
				offY = 0;
				}
			if (key == GLFW_KEY_S && action == GLFW_PRESS)
				{
				mycam.s = 1;
				offset = vec2(0, 0.005);
				offY = 0.005;
				}
			if (key == GLFW_KEY_S && action == GLFW_RELEASE)
				{
				mycam.s = 0;
				offset = vec2(0, 0);
				offY = 0;
				}
			if (key == GLFW_KEY_A && action == GLFW_PRESS)
				{
				//rotation = -0.005;
				mycam.q = 1;
				}
			if (key == GLFW_KEY_A && action == GLFW_RELEASE)
				{
				//rotation = 0.00;
				mycam.q = 0;
				}
			if (key == GLFW_KEY_D && action == GLFW_PRESS)
				{
				//rotation = 0.005;
				mycam.e = 1;
				}
			if (key == GLFW_KEY_D && action == GLFW_RELEASE)
				{
				//rotation = 0.0;
				mycam.e = 0;
				}

			if (key == GLFW_KEY_E && action == GLFW_PRESS)
				{
				heightOff = -0.005f;
				}
			if (key == GLFW_KEY_E && action == GLFW_RELEASE)
				{
				heightOff = 0.0f;
				cout << "Height: " << currentHeight << endl;
				}
			if (key == GLFW_KEY_Q && action == GLFW_PRESS)
				{
				heightOff = 0.005f;
				}
			if (key == GLFW_KEY_Q && action == GLFW_RELEASE)
				{
				heightOff = 0.0f;
				cout << "Height: " << currentHeight << endl;
				}

			if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
				{

				}
			}




		void calcPaths(float scaleRatio)
			{
			viablePaths.clear();

			vec2 planePos = -mycam.pos;
			for (landingLine l : lines)
				l.line.clear();

			float maxDistance = (currentHeight * glidingRatio);

			//calculate landing paths within max distance
			for (landingPath p : paths)
				{
				vec2 a, b;

				a = getWorldPos(vec3(p.a, 0), scaleRatio);
				b = getWorldPos(vec3(p.b, 0), scaleRatio);


				float distA = distance(a, planePos);
				float distB = distance(b, planePos);


				//get the closer of the two points for the landing path
				float min = std::min(distA, distB);
				if (min == distB)
					{
					p.closestPoint = 1;
					}
				if (min < maxDistance)
					{

					viablePaths.push_back(p);
					}
				}
			}

		// callback for the mouse when clicked move the triangle when helper functions
		// written
		void mouseCallback(GLFWwindow* window, int button, int action, int mods)
			{
		
			}
		//if the window is resized, capture the new size and reset the viewport
		void resizeCallback(GLFWwindow* window, int in_width, int in_height)
			{
			//get the window size - may be different then pixels for retina
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			glViewport(0, 0, width, height);
			}

		/*Note that any gl calls must always happen after a GL state is initialized */
		void initGeom()
			{

			offset = vec2(0, 0);
			string resourceDirectory = "../resources";
			// Initialize mesh.

			for(int ii=0;ii<3;ii++)
				draw_landingstrips[ii].init_generic(vec3(1,1,0),vec3(0,0,0));

			landingrect.init_generic(vec3(1, 1, 0), vec3(0, 0, 0));
			landingrect.re_init_rect(vec3(-0.05, -0.1, 0.0), vec3(0.05, -0.2, 0.0), vec4(1, 0.3, 1, 0.9), vec4(0, 0.0, 0.0, 0.7));

			//Billboard
			glGenVertexArrays(1, &VAO);
			glBindVertexArray(VAO);

			glGenBuffers(1, &VBO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);

			vec3 pos[6];
			pos[0] = vec3(-1, -1, 0);
			pos[1] = vec3(1, -1, 0);
			pos[2] = vec3(-1, 1, 0);

			pos[3] = vec3(-1, 1, 0);
			pos[4] = vec3(1, -1, 0);
			pos[5] = vec3(1, 1, 0);

			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 6, pos, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glGenBuffers(1, &VBO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			vec2 tex[6];
			tex[0] = vec2(0, 1);
			tex[1] = vec2(1, 1);
			tex[2] = vec2(0, 0);

			tex[3] = vec2(0, 0);
			tex[4] = vec2(1, 1);
			tex[5] = vec2(1, 0);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * 6, tex, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glBindVertexArray(0);

			//Plane VAO
			glGenVertexArrays(1, &VAOP);
			glBindVertexArray(VAOP);

			glGenBuffers(1, &VBOP);
			glBindBuffer(GL_ARRAY_BUFFER, VBOP);


			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 6, pos, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glGenBuffers(1, &VBOP);
			glBindBuffer(GL_ARRAY_BUFFER, VBOP);

			glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * 6, tex, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glBindVertexArray(1);

			int width, height, channels;
			char filepath[1000];

			//texture 1
			string str = resourceDirectory + "/Map2.png";
			strcpy(filepath, str.c_str());
			unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
			glGenTextures(1, &Texture);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);


			//texture 2
			str = resourceDirectory + "/airplane.png";
			strcpy(filepath, str.c_str());
			data = stbi_load(filepath, &width, &height, &channels, 4);
			glGenTextures(1, &Texture2);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, Texture2);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			//[TWOTEXTURES]
			//set the 2 textures to the correct samplers in the fragment shader:
			GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex");//tex, tex2... sampler in the fragment shader
			GLuint Tex2Location = glGetUniformLocation(prog->pid, "tex2");


			// Then bind the uniform samplers to texture units:
			glUseProgram(prog->pid);
			glUniform1i(Tex1Location, 0);
			glUniform1i(Tex2Location, 1);


			Tex2Location = glGetUniformLocation(plane->pid, "tex");

			glUseProgram(plane->pid);
			glUniform1i(Tex2Location, 0);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			
			//Init the gemoetry behind the lines
			l1.init();
			l2.init();
			l3.init();
			l4.init();
			l5.init();

			lines.push_back(ll1);
			lines.push_back(ll2);
			lines.push_back(ll3);
			lines.push_back(ll4);
			lines.push_back(ll5);

			drawLines.push_back(l1);
			drawLines.push_back(l2);
			drawLines.push_back(l3);
			drawLines.push_back(l4);
			drawLines.push_back(l5);

			for (Line l : drawLines)
				l.init();

			drawplane.init();
			}

		//General OGL initialization - set OGL state here
		void init(const std::string& resourceDirectory)
			{
			GLSL::checkVersion();

			// Set background color.
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			// Enable z-buffer test.
			glEnable(GL_DEPTH_TEST);

			// Initialize the GLSL program.
			prog = std::make_shared<Program>();
			prog->setVerbose(true);
			prog->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
			if (!prog->init())
				{
				std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
				exit(1);
				}
			prog->addUniform("campos");
			prog->addUniform("offset");
			prog->addUniform("M");
			prog->addUniform("P");
			prog->addUniform("V");
			prog->addAttribute("vertPos");
			prog->addAttribute("vertNor");
			prog->addAttribute("vertTex");

			//Init plane shader
			plane = std::make_shared<Program>();
			plane->setVerbose(true);
			plane->setShaderNames(resourceDirectory + "/plane_vertex.glsl", resourceDirectory + "/plane_fragment.glsl");
			if (!plane->init())
				{
				std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
				exit(1);
				}
			plane->addUniform("P");
			plane->addUniform("V");
			plane->addUniform("M");
			plane->addUniform("campos");
			//plane->addUniform("offset");
			plane->addAttribute("vertPos");
			plane->addAttribute("vertTex");


			plane_height_text.init();
			plane_height_text.set_font_size(.03f);

			plane_speed_text.init();
			plane_speed_text.set_font_size(.03f);

			plane_indist_text.init();
			plane_indist_text.set_font_size(.03f);
			//plane_height_text.draw(0, 0, 0, "==========TEST", 255, 0, 0);

			}

		//Translate screen pos into world pos
		vec3 getWorldPos(vec3 p, float scaleRatio)
			{
			p.x /= width;
			p.y /= height;
			p.y *= -1;
			p.y *= 4.2;
			p.y += 2.1;

			p.x *= 4.2;
			p.x -= 2.1;
			p *= scaleRatio;

			return p;
			}

		//Sorts all possible landing path and selects the 3 most viable to display
		void sort_possible_landing(vector<landingPath>& paths, vector<posdir_>& possible_paths, float scaleRatio)
			{
			vec3 plane_pos = vec3(-mycam.pos.x, -mycam.pos.y, currentHeight);
			vec3 plane_dir = normalize(mycam.currentDir);
			float maxDistance = (currentHeight * glidingRatio);

			vector<posdir_> front, rest;

			for (int i = 0; i < paths.size(); i++)
				{

				vec3 landingPathStart;
				vec3 landingPathEnd;


				vec3 a, b;

				//Get both end points of the landing line
				a = getWorldPos(vec3(paths[i].a, 0), scaleRatio);
				b = getWorldPos(vec3(paths[i].b, 0), scaleRatio);

				//get distance to each end from plane_pos
				float distA = distance(a, plane_pos);
				float distB = distance(b, plane_pos);

				//get min of the two distances
				float min = std::min(distA, distB);

				//find the closer
				if (min == distB)
					{
					landingPathStart = getWorldPos(vec3(paths[i].b.x, paths[i].b.y, 0), scaleRatio);
					landingPathEnd = getWorldPos(vec3(paths[i].a.x, paths[i].a.y, 0), scaleRatio);
					}
				//path.a is closer
				else
					{
					landingPathStart = getWorldPos(vec3(paths[i].a.x, paths[i].a.y, 0), scaleRatio);
					landingPathEnd = getWorldPos(vec3(paths[i].b.x, paths[i].b.y, 0), scaleRatio);
					}
				float distance_to_landing = distance(landingPathStart, plane_pos);
				if (distance_to_landing > maxDistance)continue;

				vec3 landingDir = normalize(landingPathStart - landingPathEnd);

				//further checks
				vec3 plane_landingpos = normalize(landingPathStart - plane_pos);
				float dotp = dot(-plane_landingpos, plane_dir);

				double angleInRadians = std::atan2(landingDir.y, landingDir.x);

				double angleInDegrees = (angleInRadians / PI) * 180.0;



				if (distance_to_landing > maxDistance * 0.7 && dotp < -0.707)
					{

					front.push_back(posdir_(landingPathStart, landingPathEnd, landingDir, dotp));
					}
				else
					rest.push_back(posdir_(landingPathStart, landingPathEnd, landingDir, dotp));
				}

			if (front.size() > 2)
				std::sort(front.begin(), front.end(), compareLandings);
			if (rest.size() > 2)
				std::sort(rest.begin(), rest.end(), compareLandings);
			for (int i = 0; i < front.size(); i++)
				possible_paths.push_back(front[i]);
			for (int i = 0; i < rest.size(); i++)
				possible_paths.push_back(rest[i]);

			int z;
			z = 0;
			}
		//***************************************************************
	
		void setgetmindistance(float& value, bool set)
			{
			static std::mutex mtx;
			mtx.lock();
			if (set)
				mindist = value;
			else
				value = mindist;
			mtx.unlock();
			}

		void copy_to_curves_mtx(vector<path_n_landing_>& curves, bool fromto)
			{
			static std::mutex mtx;
			static vector<path_n_landing_ > threadcurves;
			mtx.lock();
			if (fromto == false)
				for (int i = 0; i < threadcurves.size(); i++)
					curves.push_back(threadcurves[i]);
			else
				{
				for (int i = 0; i < threadcurves.size(); i++)
					threadcurves[i].path.clear();
				threadcurves.clear();
				for (int i = 0; i < curves.size(); i++)
					threadcurves.push_back(curves[i]);
				}
			mtx.unlock();
			}
		//**************************************************************************************************
		void search_for_path()
			{

			vector<path_n_landing_> curves;
			static int found_paths = 0;
			float mindistance;
			setgetmindistance(mindistance, false);
			if ((gliding_on && found_paths != 0 && mindistance <0.03) || currentHeight <0.01)
				{
				return;
				}
			found_paths = 0;

			vec3 plane_pos = vec3(-mycam.pos.x, -mycam.pos.y, currentHeight);
			vec3 plane_dir = mycam.currentDir;
			for (int i = 0; i < curves.size(); i++)
				curves[i].path.clear();
			curves.clear();

			vector<posdir_> possible_paths;
			sort_possible_landing(paths, possible_paths, scaleRatio);

			vector<vec3> directions_of_valid_landings;

			for (int i = 0; i < possible_paths.size() && found_paths < 3; i++) //float distance_to_landing = distance(landingPathStart, plane_pos);
				{
				path_n_landing_ bezier;
				vec3 direction = normalize(possible_paths[i].pos_a - plane_pos);

				bool valid = true;
				for (int u = 0; u < directions_of_valid_landings.size(); u++)
					if (dot(direction, directions_of_valid_landings[u]) > 0.7)
						valid = false;
				if (!valid)continue;
				bezier.strip_a = possible_paths[i].pos_a;
				bezier.strip_b = possible_paths[i].pos_b;
				if (find_path(bezier.path, plane_pos, plane_dir, possible_paths[i].pos_a, possible_paths[i].dir, currentHeight, glidingRatio))
					{

					directions_of_valid_landings.push_back(direction);
					found_paths++;
					//push onto vector of all curves
					curves.push_back(bezier);
					//break;
					}
				}

			copy_to_curves_mtx(curves, true);
			}
		//**************************************************************************************************

		
		//**************************************************************************************************
		void render()
			{
			double frametime = get_last_elapsed_time();

		
			// Get current frame buffer size.
			int width, height;
			glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
			float aspect = width / (float)height;
			glViewport(0, 0, width, height);
			// Clear framebuffer.
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			// Create the matrix stacks - please leave these alone for 
			glm::mat4  M, P, V, Mline; //View, Model and Perspective matrix

			M = glm::mat4(1);
			V = glm::mat4(1);
			glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.04f));

			vec3 oldpos = mycam.pos;
			V = mycam.process(frametime);

			float distance_made = glm::length(vec2(oldpos) - vec2(mycam.pos));

			if (gliding_on && currentHeight>0)
				currentHeight -= distance_made / glidingRatio;

			P = glm::ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, -2.0f, 100.0f);
			if (width < height)
				{
				P = glm::ortho(-1.0f, 1.0f, -1.0f / aspect, 1.0f / aspect, -2.0f, 100.0f);
				}
			// ...but we overwrite it (optional) with a perspective projection.
			P = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones




			//=====================================================================================//
			//								MAP													   //
			//=====================================================================================//
			//Draw Billboard for Map
			prog->bind();

			glm::mat4 sMap = glm::scale(glm::mat4(1.f), glm::vec3(scaleFactor));
			glm::mat4 tMap = glm::translate(glm::mat4(1.0f), vec3(0,0,-.01));

			M =tMap* sMap;

			glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
			glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);
			glUniform2fv(prog->getUniform("offset"), 1, &viewPos[0]);

			glBindVertexArray(VAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texture);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			prog->unbind();




			//=====================================================================================//
			//								LINE												   //
			//=====================================================================================//

			//Calc max distance plane can travel given height and current speed
			float maxDistance = (currentHeight * glidingRatio);

			//Reduce height to simulate gliding
			currentHeight += heightOff;

			//init
			int drawLineCounter = 0;
			viablePaths.clear();
			vector<path_n_landing_> curves;
			copy_to_curves_mtx(curves, false);

			
			//loop through all stored viable curves (max range of curves[0...4] right now)
			for (int i = 0; i < curves.size() && i < 3; i++)
				{
				drawLines[i].re_init_line(curves[i].path);
				}

			glm::vec4 linecolor = glm::vec4(1, 0, 0, 1);

			linecolor = glm::vec4(1,0.7, 1, 1);
			if (curve_unvalid)
				linecolor = glm::vec4(1, 0, 0, 1);

		
			if (curves.size() > 0)
				for (int i = 0; i < curves.size(); i++)
					{
					if(i!=0) linecolor = glm::vec4(1, 0.7, 1, 0.45);
					drawLines[i].draw(P, V, linecolor);				
						
					}
			
		
			// ******************   find minimum distance:
			vec3 distancemin = vec3(0, 0, 0);
			if(curves.size() > 0 && gliding_on)
				{
				float newmindist = 10000;
				vec3 p,d;
				for (int u = 0; u < curves[0].path.size() - 1; u++)
					{
					
					float cand = FindDistanceToSegment(-mycam.pos, curves[0].path[u], curves[0].path[u+1],d,p);
					if (cand < newmindist)
						{
						newmindist = cand;
						distancemin = d;
						}
					}
				float cand = FindDistanceToSegment(-mycam.pos, curves[0].strip_a, curves[0].strip_b, d, p);
				if (cand < newmindist)
					{
					newmindist = cand;
					distancemin = d;
					}
			
				setgetmindistance(newmindist, true);
				
				}

			//=====================================================================================//
			//								landing strip  										   //
			//=====================================================================================//

			glDisable(GL_DEPTH_TEST);
			for (int i = 0; i < curves.size() && i < 3; i++)
				{
				if (i == 0)
					draw_landingstrips[i].re_init(curves[i].strip_a, curves[i].strip_b, vec4(0, 0.9, 0.9, 0.8), vec4(0, 0.6, 0.9, 0.5));
				else
					draw_landingstrips[i].re_init(curves[i].strip_a, curves[i].strip_b, vec4(0, 0.6, 0.6, 0.4), vec4(0, 0.6, 0.6, 0.2));


				draw_landingstrips[i].draw(P, V);
				}
			//=====================================================================================//
			//								precision 											   //
			//=====================================================================================//

			if (gliding_on)
				{

				mat4 VV = translate(mat4(1), vec3(0, 0, -0.5));
				mat4 MD = translate(mat4(1), distancemin * 20.5f);
				landingrect.draw(P, VV);
				VV = translate(mat4(1), vec3(0, -0.15, -0.5));
				mat4 SS = scale(mat4(1), vec3(0.1, 0.1, 0.1));
				drawplane.draw_cross(P, VV * SS * MD, glm::vec4(1, 1, 1, 1));
				drawplane.draw_plane(P, VV, glm::vec4(1, 0, 1, 1));
				}

			glEnable(GL_DEPTH_TEST);
			
			//=====================================================================================//
			//								PLANE												   //
			//=====================================================================================//
			plane->bind();
			glBindVertexArray(VAOP);
			glm::mat4 Trans = glm::translate(glm::mat4(1.0f), mycam.pos);

			M = S;
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texture2);
			glUniformMatrix4fv(plane->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glUniform3fv(plane->getUniform("campos"), 1, &mycam.pos[0]);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			plane->unbind();

			
		//=====================================================================================//
		//								TEXT												   //
		//=====================================================================================//

			char sheight[100];
			char sspeed[100];
			sprintf(sheight, "%.2f ft", currentHeight * 670.);
			sprintf(sspeed, "%.1f kn", currentSpeed * 670.);
			string heightReading = "elev: ";
			heightReading  += sheight;
			string speedReading = "vel: "; 
			speedReading += sspeed;

			float mindistance;
			setgetmindistance(mindistance, false);

			//string mindistReading = "DISTANCE: " + to_string(mindistance);
			
			plane_height_text.draw(-0.1, -0.85, 0, heightReading, 155, 155, 155);
			plane_speed_text.draw(-0.1, -0.9, 0, speedReading, 155, 155, 155);
			
			plane_height_text.draw();
			plane_speed_text.draw(); 
			

			if (currentHeight <= 0 && currentSpeed >= 0) {
				currentSpeed -= 0.01 * frametime;

				currentHeight = 0;
				}
	
}
	};



inline bool operator==(const vec2 point1, const vec2 point2)
	{
	return (point1.x == point2.x && point1.y == point2.y);
	}

bool compareByDistance(const landingPath& a, const landingPath& b)
	{
	return a.length < b.length;
	}

void calc_secondary_landings(vector<vec3> greenZone, unsigned char*& data, vector<landingPath>& paths, unsigned int centerX, unsigned int centerY, float radius, float condition)
	{
	unsigned int totalPixels = greenZone.size();


	if (totalPixels < 100)
		{
		cout << "2ndary landing path is too small" << endl;
		return;
		}

	//10 degrees
	float angle = 0.f;
	//float radius2 = 100.f;

	vec2 pTop;
	vec2 pBottom;
	bool topIn;
	bool bottomIn;
	bool directional = false;
	float landingAngle = (condition * PI) / 180.;

	//Allow for 10 degree deviation in either direction
	float deviation = 0.174533 * 0.5;

	if (condition > 0) {
		directional = true;
		//cout << "conditional " << condition<< endl;
	}

	while (angle < 3.14159)
	{

		landingPath path;

		vec3 center = vec3(centerX, centerY, 0);

		unsigned int newXTop = 0;
		unsigned int newYTop = 0;
		unsigned int newXBottom = 0;
		unsigned int newYBottom = 0;

		//Top half of circle
		newXTop = center.x - cos(angle) * radius;
		newYTop = center.y - sin(angle) * radius;

		//boundary checks
		if (newYTop < 0)
			newYTop = 0;
		if (newXTop < 0)
			newXTop = 0;

		//Bottom half of circle
		newXBottom = center.x + cos(angle) * radius;
		newYBottom = center.y + sin(angle) * radius;

		//Boundary checks
		if (newYBottom > height)
			newYBottom = height;
		if (newXBottom > width)
			newXBottom = width;

		pTop = vec2(newXTop, newYTop);
		pBottom = vec2(newXBottom, newYBottom);



		//Are both points inside greenZone
		if (data[((unsigned int)pTop.x + (unsigned int)pTop.y * width) * 4] == 255 && data[((unsigned int)pBottom.x + (unsigned int)pBottom.y * width) * 4] == 255)
		{

			//for top point
			vec2 dir = normalize(pTop - pBottom);

			//while still within the greenzone, find max top position
			//unsigned int pos = ((unsigned int)pTop.x + (unsigned int)pTop.y * width);
			while (data[((unsigned int)pTop.x + (unsigned int)pTop.y * width) * 4] == 255)
			{
				if ((pTop + dir).y >= 0 && (pTop + dir).y <= height && (pTop + dir).x >= 0 && (pTop + dir).x <= width)
					pTop += dir;
				else
					break;
			}

			//while still within the greenzone, find max bottom position
			//pos = ((unsigned int)pBottom.x + (unsigned int)pBottom.y * width);
			while (data[((unsigned int)pBottom.x + (unsigned int)pBottom.y * width) * 4] == 255)
			{
				if ((pBottom - dir).y >= 0 && (pBottom - dir).y <= height && (pBottom - dir).x >= 0 && (pBottom - dir).x <= width)
					pBottom -= dir;
				else
					break;
			}

			if (directional == true && ((landingAngle - deviation) > angle || angle > (landingAngle + deviation)))
			{
				//cout << "currentAngle: " << angle << "landingAgnle: " << landingAngle << endl;
				//angle += 0.17343277777;

			}
			else
			{
				//create a new landing path
				pTop = vec2(pTop.x, pTop.y);
				path.a = pTop;

				pBottom = vec2(pBottom.x, pBottom.y);
				path.b = pBottom;

				path.length = sqrt(pow((path.b.x - path.a.x), 2) + pow((path.b.y - path.a.y), 2));

				if (path.length >= 200)
					paths.push_back(path);
			}
		}

		

		//increment 10 degrees
		angle += 0.17508833333 * 2;


	}
}

void calc_possible_landings(vector<vec3> greenZone, unsigned char*& data, vector<landingPath>& paths, float radius, int condition)
	{
	unsigned int centerX = 0;
	unsigned long centerY = 0;

	unsigned int totalPixels = greenZone.size();

	//cout << "GreenZone Size: " << totalPixels << endl;
	//sharp corners occasionally return a small green area, this should stop that
	if (totalPixels < 100)
		{
		//cout << "shape is too small" << endl;
		return;
		}

	//================ NOTE =====================
	// I'm calculating the center pixel for the top
	// and bottom half of the polygon seperately and then
	// averging the two.
	//
	// This is because in a large polygon, adding the pixel
	//	x and y values will exceed the max size of an unsigned
	// int before it is averaged.

	//calculate average of top half of polygon
	for (int i = 0; i < greenZone.size() / 2; i++)
		{
		centerX += greenZone[i].x;
		centerY += greenZone[i].y;
		}
	centerX /= (greenZone.size() / 2.f);
	centerY /= (greenZone.size() / 2.f);

	//calculate average of bottom half of polygon
	for (int i = greenZone.size() / 2.f; i < greenZone.size(); i++)
		{
		centerX += greenZone[i].x;
		centerY += greenZone[i].y;
		}
	centerX /= (greenZone.size() / 2.f);
	centerY /= (greenZone.size() / 2.f);

	
	vec3 center = vec3(centerX, centerY, 0);
	//10 degrees
	float angle = 0.f;

	vec2 pTop;
	vec2 pBottom;

	bool topIn;
	bool bottomIn;
	bool directional = false;
	float landingAngle = (condition * PI) / 180.;

	//Allow for 10 degree deviation in either direction
	float deviation = 0.174533 * 1;

	if (condition > 0) {
		directional = true;
	}
	while (angle <= 3.14159)
		{
		
		landingPath path;

		center = vec3(centerX, centerY, 0);

		int newXTop = 0;
		int newYTop = 0;
		int newXBottom = 0;
		int newYBottom = 0;

		//Top half of circle
		newXTop = center.x - cos(angle) * radius;
		newYTop = center.y - sin(angle) * radius;

		if (newYTop < 0)
			newYTop = 0;
		if (newXTop < 0)
			newXTop = 0;

		//Bottom half of circle
		newXBottom = center.x + cos(angle) * radius;
		newYBottom = center.y + sin(angle) * radius;

		if (newYBottom > height)
			newYBottom = height;
		if (newXBottom > width)
			newXBottom = width;


		pTop = vec2(newXTop, newYTop);
		pBottom = vec2(newXBottom, newYBottom);

		//Are both points inside greenZone
		if (data[((unsigned int)pTop.x + (unsigned int)pTop.y * width) * 4] == 255 && data[((unsigned int)pBottom.x + (unsigned int)pBottom.y * width) * 4] == 255)
		{

			float step = angle / (PI / 2.f);
			float delta = 0.1;

			
			//cout << angle << endl;
			if (step == 0 || step < 1 + delta && step > 1 - delta)
			{
				if (data[((unsigned int)pTop.x + (unsigned int)pTop.y * width) * 4] == 255)
					calc_secondary_landings(greenZone, data, paths, pTop.x, pTop.y, radius, condition);

				if (data[((unsigned int)pBottom.x + (unsigned int)pBottom.y * width) * 4] == 255)
					calc_secondary_landings(greenZone, data, paths, pBottom.x, pBottom.y, radius, condition);

			}

			//for top point
			vec2 dir = normalize(pTop - pBottom);

			//while still within the greenzone, find max top position
			while (data[((unsigned int)pTop.x + (unsigned int)pTop.y * width) * 4] == 255)
			{
				if ((pTop + dir).y >= 0 && (pTop + dir).y <= height && (pTop + dir).x >= 0 && (pTop + dir).x <= width)
					pTop += dir;
				else
					break;

			}

			//while still within the greenzone, find max bottom position
			while (data[((unsigned int)pBottom.x + (unsigned int)pBottom.y * width) * 4] == 255)
			{

				if ((pBottom - dir).y >= 0 && (pBottom - dir).y <= height && (pBottom - dir).x >= 0 && (pBottom - dir).x <= width)
					pBottom -= dir;
				else
					break;


			}

			//create a new landing path
			if (directional == true && ((landingAngle - deviation) > angle || angle > (landingAngle + deviation)))
			{
				
			}
			else
			{
				//turn pixelspace coords into screenspace coords
				pTop = vec2(pTop.x, pTop.y);
				path.a = pTop;
				pBottom = vec2(pBottom.x, pBottom.y);
				path.b = pBottom;

				path.length = sqrt(pow((path.b.x - path.a.x), 2) + pow((path.b.y - path.a.y), 2));

				path.landingCondition = condition;

				if (path.length >= 200)
					paths.push_back(path);
			}			
		}
	//increment 10 degrees
	angle += 0.17343277777;
	}
}

void drawPathLines(unsigned char*& data, vector<landingPath> paths, int numPaths)
	{
	vec2 a, b;
	int deltaX, deltaY;
	float error = 0;
	float deltaError;

	for (int i = 0; i < numPaths; i++)
		{
		a = paths.back().a;
		b = paths.back().b;
		paths.pop_back();

		//draw a
		data[((int)a.x + (int)a.y * width) * 4] = 0;
		data[((int)a.x + (int)a.y * width) * 4 + 1] = 0;
		data[((int)a.x + (int)a.y * width) * 4 + 2] = 255;

		//draw b
		data[((int)b.x + (int)b.y * width) * 4] = 0;
		data[((int)b.x + (int)b.y * width) * 4 + 1] = 0;
		data[((int)b.x + (int)b.y * width) * 4 + 2] = 255;


		int x1 = a.x;
		int y1 = a.y;
		int x2 = b.x;
		int y2 = b.y;

		// Use Bresenham's line algorithm to draw landing paths
		const bool steep = (fabs(y2 - y1) > fabs(x2 - x1));
		if (steep)
			{
			swap(x1, y1);
			swap(x2, y2);
			}

		if (x1 > x2)
			{
			swap(x1, x2);
			swap(y1, y2);
			}

		const float dx = x2 - x1;
		const float dy = fabs(y2 - y1);

		float error = dx / 2.0f;
		const int ystep = (y1 < y2) ? 1 : -1;
		int y = (int)y1;

		const unsigned int maxX = (int)x2;

		for (int x = (int)x1; x < maxX; x++)
			{
			if (steep)
				{
				unsigned int pos = ((int)y + (int)x * width);

				data[pos * 4] = 0;
				data[pos * 4 + 1] = 0;
				data[pos * 4 + 2] = 255;
				}
			else
				{
				unsigned int pos = ((int)x + (int)y * width);

				data[((int)x + (int)y * width) * 4] = 0;
				data[((int)x + (int)y * width) * 4 + 1] = 0;
				data[((int)x + (int)y * width) * 4 + 2] = 255;
				}

			error -= dy;
			if (error < 0)
				{
				y += ystep;
				error += dx;
				}
			}

		}

	}

void iterativeFlood(unsigned char* data, int x, int y)
	{
	vector<vec2> stack;
	stack.push_back(vec2(x, y));
	vec2 pixel;
	while (!stack.empty()) {

		//check top of stack
		vec2 p = stack.back();
		stack.pop_back();

		//if not visited and green
		if (data[((unsigned int)p.x + (unsigned int)p.y * width) * 4] != 255 && data[(((unsigned int)p.x + (unsigned int)p.y * width) * 4) + 1] >= 200)
			{
			//set to visited
			data[((int)p.x + (int)p.y * width) * 4] = 255;
			//push onto green											  
			greenstuff.push_back(vec3(p.x, p.y, 0));

			//push neighboring pixels onto stack
			if ((int)p.y > 1) stack.push_back(p - vec2(0, 1));
			if ((int)p.x > 1) stack.push_back(p - vec2(1, 0));
			if ((int)p.x < width - 1) stack.push_back(p + vec2(1, 0));
			if ((int)p.y < height - 1) stack.push_back(p + vec2(0, 1));
			}


		}
	}

Application* application = NULL;

void start_pathsearching()
	{
	while (application->running)
		application->search_for_path();
	}
//******************************************************************************************
int main(int argc, char** argv)
	{



	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
		{
		resourceDir = argv[1];
		}

	application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager* windowManager = new WindowManager();
	windowManager->init(1000, 1000);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;


	//===========================================================
	//=
	//=					Landing Path Calc
	//============================================================

	cout << "Speed: " << currentSpeed << endl;
	cout << "Height: " << currentHeight << endl;


	//run algorithms
	int channels;
	int numChannels = 4;


	//texture 1
	char filepath[1000];
	string str = resourceDir + "/Tex.png";
	strcpy(filepath, str.c_str());
	unsigned char* data = stbi_load(filepath, &width, &height, &channels, numChannels);

	//cout << width << " and " << height << endl;

	for (unsigned int y = 0; y < height; y++)
		{
		for (unsigned int x = 0; x < width; x++)
			{
			//clear landing zone buffer
			greenstuff.clear();
			float condition = 0;
	
			//if there is some green and not full red (which would mean visited)
			if (data[((x + y * width) * numChannels) + 1] >= 200 && data[((x + y * width) * numChannels)] != 255)
				{
				//Check if special conditions for this zone
				if (data[((x + y * width) * numChannels) + 2] >= 10)
				{
					condition = data[((x + y * width) * numChannels) + 2];
				}

				//run iterative flood to find all greenzones			
				iterativeFlood(data, x, y);
				
				//calc all landings based off greenzones
				calc_possible_landings(greenstuff, data, paths, 30.f, condition);

				//If any paths exist, draw them
				if (paths.size() != 0)
					{
					drawPathLines(data, paths, paths.size());
					}
				
				}


			}
		}

	std::vector<unsigned char> buffer(width * height * numChannels);
	str = resourceDir + "/output.png";
	strcpy(filepath, str.c_str());
	stbi_write_png(filepath, width, height, numChannels, data, 0);

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
		// Initialize scene.
	application->init(resourceDir);
	application->initGeom();
	thread t1(start_pathsearching);
	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(windowManager->getHandle()))
		{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
		}
	application->running = false;
	t1.join();
	// Quit program.
	windowManager->shutdown();
	return 0;
	}
