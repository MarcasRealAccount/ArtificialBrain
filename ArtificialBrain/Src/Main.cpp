#include "Brain.h"

#include <string>
#include <vector>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

const char s_VertexShaderSource[]   = R"glsl(#version 460 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 col;

layout(location = 0) out vec3 passCol;

layout(location = 0) uniform vec2 camPos;
layout(location = 1) uniform vec2 camScale;

void main()
{
	gl_Position = vec4((pos - camPos) * camScale, 0.0f, 1.0f);
	passCol     = col;
}
)glsl";
const char s_FragmentShaderSource[] = R"glsl(#version 460 core

layout(location = 0) in vec3 passCol;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(passCol, 1.0f);
}
)glsl";

struct Vertex
{
	Point pos;
	float r, g, b;
};

GLuint CompileProgram(const char* vertexShaderSource, const char* fragmentShaderSource);

int main(int argc, char** argv)
{
	if (!glfwInit())
		return 1;

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

	auto window = glfwCreateWindow(1280, 720, "Artificial Brain", nullptr, nullptr);
	if (!window)
		return 1;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
		return 1;

	GLuint shaderProgram = CompileProgram(s_VertexShaderSource, s_FragmentShaderSource);
	if (!shaderProgram)
		return 1;

	GLuint vaos[1];
	GLuint vbos[1];

	std::vector<Vertex> lineSegments(2 * 256 * 31);

	glCreateVertexArrays(1, vaos);
	glCreateBuffers(1, vbos);

	glBindVertexArray(vaos[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, lineSegments.size() * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, r));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	float camX   = 0.0f;
	float camY   = 0.0f;
	float scaleY = 1.0f / 10.0f;
	float scaleX = scaleY;

	Neuron& neuron = *new Neuron();
	InitNeuron(neuron);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);
		scaleX = scaleY * height / width;

		GrowNeuron(neuron);
		size_t index = 0;
		{
			for (size_t i = 0; i < 256; ++i)
			{
				float r = 0.0f;
				float g = 0.0f;
				float b = 0.0f;

				if (neuron.longest == i)
				{
					r = 1.00f;
					g = 0.05f;
					b = 0.05f;
				}
				else if (neuron.furthest == i)
				{
					r = 0.05f;
					g = 1.00f;
					b = 0.05f;
				}
				else
				{
					r = 0.05f;
					g = 0.05f;
					b = 1.00f;
				}

				for (size_t j = 0; j < 31; ++j)
				{
					lineSegments[index++] = { neuron.dendrites[i].points[j], r, g, b };
					lineSegments[index++] = { neuron.dendrites[i].points[j + 1], r, g, b };
				}
			}
			glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, lineSegments.size() * sizeof(Vertex), lineSegments.data());

			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);
		glUniform2f(0, camX, camY);
		glUniform2f(1, scaleX, scaleY);

		glBindVertexArray(vaos[0]);
		glDrawArrays(GL_LINES, 0, index);

		glBindVertexArray(0);
		glUseProgram(0);

		glfwSwapBuffers(window);
	}
	delete &neuron;

	glDeleteProgram(shaderProgram);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

GLuint CompileProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
{
	GLuint vertexShader   = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	const char* sources[] { vertexShaderSource };
	glShaderSource(vertexShader, 1, sources, nullptr);
	sources[0] = fragmentShaderSource;
	glShaderSource(fragmentShader, 1, sources, nullptr);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	{
		bool failed = false;

		int status = 0;
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			int length = 0;
			glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &length);
			std::string log(length, '\0');
			glGetShaderInfoLog(vertexShader, length, nullptr, log.data());
			std::printf("VertexShader log: %s\n", log.c_str());
			failed = true;
		}

		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			int length = 0;
			glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &length);
			std::string log(length, '\0');
			glGetShaderInfoLog(fragmentShader, length, nullptr, log.data());
			std::printf("FragmentShader log: %s\n", log.c_str());
			failed = true;
		}

		if (failed)
		{
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
			return 0;
		}
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	int status = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status)
	{
		int length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		std::string log(length, '\0');
		glGetProgramInfoLog(program, length, nullptr, log.data());
		std::printf("Linkage log: %s\n", log.data());
		glDeleteProgram(program);
		return 0;
	}

	return program;
}