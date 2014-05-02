#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

#define ROT_SPEED  .0003

#define NEAR ((GLfloat).1)
#define FAR  ((GLfloat)100.)
#define FOV  ((GLfloat)57.)
GLfloat aspect = 640./480.;

static int window_width = 640;
static int window_height = 480;

typedef struct {
	GLuint vao;
	GLuint cnt;
} model_t;

static model_t make_model(GLfloat * verts, GLfloat * norms, GLfloat * uv_coords, GLuint vsize, GLuint nsize, GLuint csize, const char * tfn)
{
	model_t m = { 0, vsize/(GLuint)(sizeof(GLfloat)*3) };

	// create vbo
	GLuint vvbo = 0;
	glGenBuffers(1, &vvbo);
	glBindBuffer(GL_ARRAY_BUFFER, vvbo);
	glBufferData(GL_ARRAY_BUFFER, vsize, verts, GL_STATIC_DRAW);

	GLuint nvbo = 0;
	glGenBuffers(1, &nvbo);
	glBindBuffer(GL_ARRAY_BUFFER, nvbo);
	glBufferData(GL_ARRAY_BUFFER, nsize, norms, GL_STATIC_DRAW);

	GLuint tvbo = 0;
	glGenBuffers(1, &tvbo);
	glBindBuffer(GL_ARRAY_BUFFER, tvbo);
	glBufferData(GL_ARRAY_BUFFER, csize, uv_coords, GL_STATIC_DRAW);

	// create vao
	glGenVertexArrays(1, &m.vao);
	glBindVertexArray(m.vao);

	glBindBuffer(GL_ARRAY_BUFFER, vvbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, nvbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, tvbo);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	return m;
}

static model_t make_cube(const char * tfn)
{
	#include "cube.h"
	return make_model(verts, norms, uv_coords, sizeof(verts), sizeof(norms), sizeof(uv_coords), tfn);
}

static void read_file(const char * fn, GLchar ** buf, GLint * len)
{
	FILE * f = fopen(fn, "rb");
	assert(f != NULL);

	fseek(f, 0, SEEK_END);
	*len = ftell(f);
	fseek(f, 0, SEEK_SET);

	*buf = (char *) malloc(*len);
	fread(*buf, 1, *len, f);

	fclose(f);
}

static GLuint make_shader(const char * fn, GLenum type)
{
	GLchar * buf;
	GLint len;
	read_file(fn, &buf, &len);

	GLuint s = glCreateShader(type);
	glShaderSource(s, 1, (const GLchar **)&buf, &len);
	glCompileShader(s);

	int status;
	glGetShaderiv(s, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char log[4096];
		glGetShaderInfoLog(s, 4096, NULL, log);
		fprintf(stderr, "error: %s compilation failed\n\n%s", fn, log);
		exit(EXIT_FAILURE);
	}

	free(buf);
	return s;
}

static GLuint make_program(void)
{
	GLuint prog = glCreateProgram();
	glAttachShader(prog, make_shader("vert.glsl", GL_VERTEX_SHADER));
	glAttachShader(prog, make_shader("frag.glsl", GL_FRAGMENT_SHADER));

	glBindAttribLocation(prog, 0, "in_pos");
	glBindAttribLocation(prog, 1, "in_norm");

	glLinkProgram(prog);

	int status;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		char log[4096];
		glGetProgramInfoLog(prog, 4096, NULL, log);
		fprintf(stderr, "error: could not link shader program\n\n%s", log);
		exit(EXIT_FAILURE);
	}

	return prog;
}

static void error_callback(int error, const char * desc)
{
	fprintf(stderr, "error (glfw error %d): %s\n", error, desc);
}

// called when window resizes
static void resize_window_callback(GLFWwindow * win, int w, int h)
{
	glViewport(0, 0, w, h);
	aspect = (double)w / (double)h;
}

// updates keypress states
static void keypress_callback(GLFWwindow * win, int key, int sc, int act, int mods)
{
	if (act != GLFW_REPEAT) {
		if (key == 'Q') exit(0);
	}
}

static void regmat(GLuint prog, glm::mat4 m, const GLchar * str)
{
	GLuint t = glGetUniformLocation(prog, str);
	glUniformMatrix4fv(t, 1, GL_FALSE, &m[0][0]);
}

static void draw_model(GLuint prog, model_t m, glm::vec3 pos, glm::vec3 rotb, glm::vec3 rota, glm::vec3 scl)
{
	glm::mat4 model;
	model = glm::rotate(model, rotb.x, glm::vec3(1., 0., 0.));
	model = glm::rotate(model, rotb.y, glm::vec3(0., 1., 0.));
	model = glm::rotate(model, rotb.z, glm::vec3(0., 0., 1.));
	model = glm::translate(model, pos);
	model = glm::rotate(model, rota.x, glm::vec3(1., 0., 0.));
	model = glm::rotate(model, rota.y, glm::vec3(0., 1., 0.));
	model = glm::rotate(model, rota.z, glm::vec3(0., 0., 1.));
	model = glm::scale(model, scl);
	regmat(prog, model, "model");

	glBindVertexArray(m.vao);
	glDrawArrays(GL_TRIANGLES, 0, m.cnt);
}

int main()
{
	glfwSetErrorCallback(error_callback);

	// initialize glfw
	if (!glfwInit()) {
		fprintf(stderr, "error: could not start glfw.\n");
		return EXIT_FAILURE;
	}
	atexit(glfwTerminate);

	// glfw hints
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	// create glfw window
	GLFWwindow * window = glfwCreateWindow(window_width, window_height, "Project 2", NULL, NULL);
	assert(window != NULL);
	glfwMakeContextCurrent(window);

	// setup callbacks
	glfwSetWindowSizeCallback(window, resize_window_callback);
	glfwSetKeyCallback(window, keypress_callback);

	// initialize glew
	glewExperimental = GL_TRUE;
	assert(glewInit() == GLEW_OK);


	glfwSwapInterval(1);

	// enable depth buffer
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	srandom(time(NULL));

	// make stuff
	model_t cube = make_cube("tex/plyr.png");
	model_t point = make_cube("tex/point.png");
	GLuint prog = make_program();

	glm::vec3 plyr_pos(0., 0.001, 0.);
	glm::vec3 plyr_rot(0., 0., 0.);
	glm::vec3 pnt_rot (M_PI/4., 0., 0.);

	GLfloat cam_angle=0.;

	// main loop
	double ptime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		double time = glfwGetTime();
		if (time - ptime > .01) {
			ptime = time;

			cam_angle += .04;
			// update matricies
			glm::mat4 view = glm::lookAt(
				glm::vec3(cos(cam_angle+M_PI/4.)*10., 5., sin(cam_angle+M_PI/4.)*10.),
				glm::vec3(0., 0., 0.),
				glm::vec3(0., 1., 0.)
			);

			glm::mat4 proj = glm::perspective(FOV, aspect, NEAR, FAR);

			regmat(prog, view, "view");
			regmat(prog, proj, "proj");

			// redraw
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUseProgram(prog);

			draw_model(prog, cube,  plyr_pos, plyr_rot, glm::vec3(0.), glm::vec3(1.));

			glfwPollEvents();
			glfwSwapBuffers(window);
		}
	}

	return 0;
}
