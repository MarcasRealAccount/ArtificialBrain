#include <cstdint>

#include <random>
#include <thread>
#include <vector>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Brain.h"

const char s_VertexShaderSource[]   = R"glsl(#version 460 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 stats;

layout(location = 0) out vec3 passStats;

layout(location = 0) uniform vec2 camPos;
layout(location = 1) uniform vec2 camScale;

void main()
{
	gl_Position = vec4((pos - camPos) * camScale, 0.0f, 1.0f);
	passStats   = stats;
}
)glsl";
const char s_FragmentShaderSource[] = R"glsl(#version 460 core

layout(location = 0) in vec3 passStats;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(passStats, 1.0f);
}
)glsl";

struct Vertex
{
	float x, y;
	float a, b, c;
};

GLuint CompileProgram(const char* vertexShaderSource, const char* fragmentShaderSource);

int main()
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
	glfwSwapInterval(0);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
		return 1;

	GLuint shaderProgram = CompileProgram(s_VertexShaderSource, s_FragmentShaderSource);
	if (!shaderProgram)
		return 1;

	GLuint vaos[2];
	GLuint vbos[2];

	std::vector<Vertex> cpuNeurons;
	std::vector<Vertex> cpuConnections;

	glCreateVertexArrays(2, vaos);
	glCreateBuffers(2, vbos);

	glBindVertexArray(vaos[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, a));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(vaos[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, a));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Brain brain;
	float w = 96.0f;
	brain.Populate(32768, -w * 16 / 9, w * 16 / 9, -w, w);
	cpuNeurons.reserve(brain.GetNeurons().capacity());
	cpuConnections.reserve(4 * brain.GetConnections().capacity());
	size_t tickCount = 0;

	float camX   = 0.0f;
	float camY   = 0.0f;
	float scaleY = 0.98f / w;
	float scaleX = scaleY;
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);
		scaleX = scaleY * height / width;

		if (tickCount < 10000)
		{
			brain.Write(0, 1.0f);
			brain.Write(1, 1.0f);
			brain.Write(2, 1.0f);
		}
		size_t newConnections = brain.Tick();
		std::printf("Tick %10zu : %5zu new connections\n", tickCount, newConnections);
		++tickCount;
		{
			auto& neurons = brain.GetNeurons();
			bool  resized = false;
			if (cpuNeurons.size() < neurons.size())
			{
				cpuNeurons.resize(neurons.size());
				resized = true;
			}

			for (size_t i = 0; i < neurons.size(); ++i)
			{
				auto& neuron  = neurons[i];
				cpuNeurons[i] = Vertex {
					.x = neuron.Pos.x,
					.y = neuron.Pos.y,
					.a = std::max<float>(neuron.Value / neuron.MaxStrength, 0.03f),
					.b = std::max<float>(neuron.Value / neuron.MaxStrength, 0.03f),
					.c = std::max<float>(neuron.Value / neuron.MaxStrength, 0.03f)
				};
			}

			glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
			if (resized)
				glBufferData(GL_ARRAY_BUFFER, cpuNeurons.size() * sizeof(Vertex), cpuNeurons.data(), GL_DYNAMIC_DRAW);
			else
				glBufferSubData(GL_ARRAY_BUFFER, 0, cpuNeurons.size() * sizeof(Vertex), cpuNeurons.data());

			auto& connections = brain.GetConnections();
			resized           = false;
			if (cpuConnections.size() < 4 * connections.size())
			{
				cpuConnections.resize(4 * connections.size());
				resized = true;
			}

			for (size_t i = 0; i < connections.size(); ++i)
			{
				auto& con             = connections[i];
				auto& a               = neurons[con.NeuronA];
				auto& b               = neurons[con.NeuronB];
				float midX            = a.Pos.x + (b.Pos.x - a.Pos.x) / 2;
				float midY            = a.Pos.y + (b.Pos.y - a.Pos.y) / 2;
				cpuConnections[i * 4] = Vertex {
					.x = a.Pos.x,
					.y = a.Pos.y,
					.a = std::max<float>(con.BiasMinA * a.Value / a.MaxStrength, 0.03f),
					.b = std::max<float>(con.BiasMaxA * a.Value / a.MaxStrength, 0.03f),
					.c = std::max<float>(con.StrengthA * a.Value / a.MaxStrength, 0.03f)
				};
				cpuConnections[i * 4 + 1] = Vertex {
					.x = midX,
					.y = midY,
					.a = std::max<float>(con.BiasMinA * a.Value / a.MaxStrength, 0.03f),
					.b = std::max<float>(con.BiasMaxA * a.Value / a.MaxStrength, 0.03f),
					.c = std::max<float>(con.StrengthA * a.Value / a.MaxStrength, 0.03f)
				};
				cpuConnections[i * 4 + 2] = Vertex {
					.x = midX,
					.y = midY,
					.a = std::max<float>(con.BiasMinB * a.Value / a.MaxStrength, 0.03f),
					.b = std::max<float>(con.BiasMaxB * a.Value / a.MaxStrength, 0.03f),
					.c = std::max<float>(con.StrengthB * b.Value / b.MaxStrength, 0.03f)
				};
				cpuConnections[i * 4 + 3] = Vertex {
					.x = b.Pos.x,
					.y = b.Pos.y,
					.a = std::max<float>(con.BiasMinB * a.Value / a.MaxStrength, 0.03f),
					.b = std::max<float>(con.BiasMaxB * a.Value / a.MaxStrength, 0.03f),
					.c = std::max<float>(con.StrengthB * b.Value / b.MaxStrength, 0.03f)
				};
			}
			glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
			if (resized)
				glBufferData(GL_ARRAY_BUFFER, cpuConnections.size() * sizeof(Vertex), cpuConnections.data(), GL_DYNAMIC_DRAW);
			else
				glBufferSubData(GL_ARRAY_BUFFER, 0, cpuConnections.size() * sizeof(Vertex), cpuConnections.data());

			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		glUniform2f(0, camX, camY);
		glUniform2f(1, scaleX, scaleY);

		glBindVertexArray(vaos[1]);
		glDrawArrays(GL_LINES, 0, cpuConnections.size());

		glBindVertexArray(vaos[0]);
		glEnable(GL_POINT_SIZE);
		glPointSize(2.0f);
		glDrawArrays(GL_POINTS, 0, cpuNeurons.size());

		glBindVertexArray(0);
		glUseProgram(0);

		glfwSwapBuffers(window);
	}

	glDeleteBuffers(2, vbos);
	glDeleteVertexArrays(2, vaos);

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