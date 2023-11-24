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

		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		float mouseXInput = (float) (mouseX / width);
		float mouseYInput = (float) (mouseY / height);

		if (tickCount < 1000)
		{
			for (size_t i = 20; i < 30; ++i)
				brain.Write(i, 1.0f);
			for (size_t i = 30; i < 40; ++i)
				brain.Write(i, 1.0f);
		}
		for (size_t i = 0; i < 10; ++i)
			brain.Write(i, mouseXInput);
		for (size_t i = 10; i < 20; ++i)
			brain.Write(i, mouseYInput);
		float mouseXOutput = 0.0f, mouseYOutput = 0.0f;
		for (size_t i = 20; i < 30; ++i)
			mouseXOutput += brain.Read(i);
		for (size_t i = 30; i < 40; ++i)
			mouseYOutput += brain.Read(i);
		mouseXOutput /= 10.0f;
		mouseYOutput /= 10.0f;
		for (size_t i = 40; i < 50; ++i)
			brain.Write(i, mouseXOutput);
		for (size_t i = 50; i < 60; ++i)
			brain.Write(i, mouseYOutput);
		size_t newConnections = brain.Tick();
		std::printf("Tick %10zu : %5zu new connections (%f, %f => %f, %f)\n", tickCount, newConnections, mouseXInput, mouseYInput, mouseXOutput, mouseYOutput);
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
					.a = std::max<float>(con.BiasMinA, 0.03f),
					.b = std::max<float>(con.BiasMaxA, 0.03f),
					.c = std::max<float>(con.StrengthA * a.Value / a.MaxStrength, 0.03f)
				};
				cpuConnections[i * 4 + 1] = Vertex {
					.x = midX,
					.y = midY,
					.a = std::max<float>(con.BiasMinA, 0.03f),
					.b = std::max<float>(con.BiasMaxA, 0.03f),
					.c = std::max<float>(con.StrengthA * a.Value / a.MaxStrength, 0.03f)
				};
				cpuConnections[i * 4 + 2] = Vertex {
					.x = midX,
					.y = midY,
					.a = std::max<float>(con.BiasMinB, 0.03f),
					.b = std::max<float>(con.BiasMaxB, 0.03f),
					.c = std::max<float>(con.StrengthB * b.Value / b.MaxStrength, 0.03f)
				};
				cpuConnections[i * 4 + 3] = Vertex {
					.x = b.Pos.x,
					.y = b.Pos.y,
					.a = std::max<float>(con.BiasMinB, 0.03f),
					.b = std::max<float>(con.BiasMaxB, 0.03f),
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

// struct Connection
//{
//	size_t target;
//	float  min, max;
//	float  strength;
// };
//
// struct Neuron
//{
//	float    x, y;
//	float    value, maxStrength;
//	uint32_t connectTick, connectTicks;
//	bool     canConnect;
//
//	std::vector<Connection> connections;
// };
//
// struct Brain
//{
//	std::mt19937        rng { std::random_device {}() };
//	std::vector<Neuron> neurons;
// };
//
// int BrainRandInt(Brain& brain, int min, int max)
//{
//	return std::uniform_int_distribution(min, max)(brain.rng);
// }
//
// float BrainRandFloat(Brain& brain, float min, float max)
//{
//	return std::uniform_real_distribution(min, max)(brain.rng);
// }
//
// void PopulateBrain(Brain& brain, size_t n)
//{
//	brain.neurons.resize(n);
//
//	for (size_t i = 0; i < n; ++i)
//	{
//		brain.neurons[i] = Neuron {
//			.x            = BrainRandFloat(brain, -113.0f, 113.0f),
//			.y            = BrainRandFloat(brain, -64.0f, 64.0f),
//			.value        = 0.0f,
//			.maxStrength  = 0.0f,
//			.connectTick  = 0,
//			.connectTicks = (uint32_t) BrainRandInt(brain, 25, 125),
//			.canConnect   = true
//		};
//	}
// }
//
// std::vector<size_t> FindNearestNeurons(Brain& brain, size_t neuron, float maxDist)
//{
//	auto& a = brain.neurons[neuron];
//
//	std::vector<size_t> neighbours;
//	for (size_t i = 0; i < brain.neurons.size(); ++i)
//	{
//		if (i == neuron)
//			continue;
//
//		auto& b = brain.neurons[i];
//
//		float dx   = b.x - a.x;
//		float dy   = b.y - a.y;
//		float dist = sqrtf(dx * dx + dy * dy);
//		if (dist <= maxDist)
//			neighbours.push_back(i);
//	}
//	return neighbours;
// }
//
// size_t Tick(Brain& brain)
//{
//	float  maxDist        = 2.5f;
//	size_t newConnections = 0;
//
//	std::vector<float> nextValues(brain.neurons.size());
//	for (size_t i = 0; i < brain.neurons.size(); ++i)
//	{
//		auto& a = brain.neurons[i];
//		if (a.canConnect && a.value > 0.25f)
//			++a.connectTick;
//		for (auto& con : a.connections)
//		{
//			float deltaStrength = 0.0f;
//			float deltaMin      = 0.0f;
//			float deltaMax      = 0.0f;
//			if (a.value >= con.min && a.value <= con.max)
//			{
//				nextValues[con.target] += con.strength;
//				deltaStrength           = (1.0f - con.strength) * BrainRandFloat(brain, 0.0f, 0.01f);
//				float force             = BrainRandFloat(brain, 0.0f, 0.1f);
//				deltaMax                = (a.value - con.max) * force;
//				deltaMin                = (a.value - con.min) * force;
//			}
//			else
//			{
//				deltaStrength = (0.05f - con.strength) * BrainRandFloat(brain, 0.0f, 0.05f);
//				float force   = BrainRandFloat(brain, 0.0f, 0.05f);
//				deltaMax      = (1.1f - con.max) * force;
//				deltaMin      = (-0.1f - con.min) * force;
//			}
//			con.strength  += deltaStrength;
//			con.min       += deltaMin;
//			con.max       += deltaMax;
//			a.maxStrength += deltaStrength;
//		}
//	}
//	for (size_t i = 0; i < brain.neurons.size(); ++i)
//	{
//		auto& a = brain.neurons[i];
//		a.value = nextValues[i] / std::max<float>(a.maxStrength, 1.0f);
//		if (a.connectTick >= a.connectTicks)
//		{
//			a.connectTick        = 0;
//			auto   neighbours    = FindNearestNeurons(brain, i, maxDist);
//			size_t bestNeighbour = ~0ULL;
//			float  closestDist   = maxDist + 0.1f;
//			for (auto neighbour : neighbours)
//			{
//				auto& b    = brain.neurons[neighbour];
//				float dx   = b.x - a.x;
//				float dy   = b.y - a.y;
//				float dist = sqrtf(dx * dx + dy * dy);
//
//				bool good = true;
//				for (auto& con : a.connections)
//				{
//					if (con.target == neighbour)
//					{
//						good = false;
//						break;
//					}
//
//					auto& c   = brain.neurons[con.target];
//					float dx2 = c.x - a.x;
//					float dy2 = c.y - a.y;
//
//					float dot = (dx * dx2 + dy * dy2) / (dist * sqrtf(dx2 * dx2 + dy2 * dy2));
//					if (!(dot > 0.3f || dot < -0.3f))
//					{
//						good = false;
//						break;
//					}
//				}
//				if (!good)
//					continue;
//
//				if (dist < closestDist)
//				{
//					closestDist   = dist;
//					bestNeighbour = neighbour;
//				}
//			}
//
//			if (bestNeighbour == ~0ULL)
//			{
//				a.canConnect = false;
//				continue;
//			}
//
//			float biasA    = BrainRandFloat(brain, -0.1f, 1.1f);
//			float biasB    = BrainRandFloat(brain, -0.1f, 1.1f);
//			float strength = BrainRandFloat(brain, 0.0f, 1.0f);
//			a.connections.emplace_back(Connection {
//				.target   = bestNeighbour,
//				.min      = std::min<float>(biasA, biasB),
//				.max      = std::max<float>(biasA, biasB),
//				.strength = strength,
//			});
//			a.maxStrength += strength;
//
//			auto& b  = brain.neurons[bestNeighbour];
//			biasA    = BrainRandFloat(brain, -0.1f, 1.1f);
//			biasB    = BrainRandFloat(brain, -0.1f, 1.1f);
//			strength = BrainRandFloat(brain, 0.0f, 1.0f);
//			b.connections.emplace_back(Connection {
//				.target   = i,
//				.min      = std::min<float>(biasA, biasB),
//				.max      = std::max<float>(biasA, biasB),
//				.strength = strength,
//			});
//			b.maxStrength += strength;
//
//			newConnections += 2;
//		}
//	}
//
//	return newConnections;
// }
//
// const char vertexShaderSource[] = R"glsl(#version 460 core
//
// layout (location = 0) in vec2 pos;
// layout (location = 1) in float value;
//
// layout (location = 0) out float passValue;
//
// void main()
//{
//	gl_Position = vec4(pos, 0.0, 1.0);
//	passValue = value;
// }
//)glsl";
//
// const char fragmentShaderSource[] = R"glsl(#version 460 core
//
// layout (location = 0) in float passValue;
//
// layout (location = 0) out vec4 outColor;
//
// void main()
//{
//	outColor = vec4(passValue, passValue, passValue, 1.0);
// }
//)glsl";
//
// int main()
//{
//	if (!glfwInit())
//		return 1;
//
//	glfwDefaultWindowHints();
//	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
//
//	auto window = glfwCreateWindow(1280, 720, "Artificial Brain", nullptr, nullptr);
//	if (!window)
//		return 1;
//
//	glfwMakeContextCurrent(window);
//	glfwSwapInterval(0);
//
//	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
//		return 1;
//
//	GLuint shaderProgram;
//	{
//		const char* const vertexShaderSources[]   = { vertexShaderSource };
//		const char* const fragmentShaderSources[] = { fragmentShaderSource };
//
//		auto vertexShader   = glCreateShader(GL_VERTEX_SHADER);
//		auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
//
//		glShaderSource(vertexShader, 1, vertexShaderSources, nullptr);
//		glShaderSource(fragmentShader, 1, fragmentShaderSources, nullptr);
//
//		glCompileShader(vertexShader);
//		glCompileShader(fragmentShader);
//
//		std::string errors;
//		int         success;
//		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
//		if (!success)
//		{
//			int length;
//			glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &length);
//			std::string infoLog(length, '\0');
//			glGetShaderInfoLog(vertexShader, length, nullptr, infoLog.data());
//			errors += "Vertex shader error: " + infoLog;
//		}
//		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
//		if (!success)
//		{
//			int length;
//			glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &length);
//			std::string infoLog(length, '\0');
//			glGetShaderInfoLog(fragmentShader, length, nullptr, infoLog.data());
//			errors += "Fragment shader error: " + infoLog;
//		}
//
//		if (!errors.empty())
//		{
//			std::printf("%s\n", errors.c_str());
//			glDeleteShader(vertexShader);
//			glDeleteShader(fragmentShader);
//			glfwDestroyWindow(window);
//			glfwTerminate();
//			return 1;
//		}
//
//		shaderProgram = glCreateProgram();
//		glAttachShader(shaderProgram, vertexShader);
//		glAttachShader(shaderProgram, fragmentShader);
//		glLinkProgram(shaderProgram);
//		glDeleteShader(vertexShader);
//		glDeleteShader(fragmentShader);
//
//		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
//		if (!success)
//		{
//			int length;
//			glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &length);
//			std::string infoLog(length, '\0');
//			glGetProgramInfoLog(shaderProgram, length, nullptr, infoLog.data());
//			std::printf("%s\n", infoLog.c_str());
//			glDeleteProgram(shaderProgram);
//			glfwDestroyWindow(window);
//			glfwTerminate();
//			return 1;
//		}
//	}
//
//	Brain brain;
//	PopulateBrain(brain, 32768);
//
//	struct Vertex
//	{
//		float x, y;
//		float r;
//	};
//
//	GLuint vaos[2];
//	GLuint vbos[2];
//	glCreateVertexArrays(2, vaos);
//	glCreateBuffers(2, vbos);
//
//	glBindVertexArray(vaos[0]);
//	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
//	glBufferData(GL_ARRAY_BUFFER, brain.neurons.size() * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
//	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
//	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (2 * sizeof(float)));
//	glEnableVertexAttribArray(0);
//	glEnableVertexAttribArray(1);
//
//	glBindVertexArray(vaos[1]);
//	glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
//	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
//	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (2 * sizeof(float)));
//	glEnableVertexAttribArray(0);
//	glEnableVertexAttribArray(1);
//
//	glBindVertexArray(0);
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//	size_t lineCount = 0;
//	size_t tickCount = 0;
//	while (!glfwWindowShouldClose(window))
//	{
//		glfwPollEvents();
//
//		int width, height;
//		glfwGetFramebufferSize(window, &width, &height);
//
//		glViewport(0, 0, width, height);
//
//		if (tickCount < 10000)
//		{
//			brain.neurons[0].value = 1.0f;
//			brain.neurons[1].value = 1.0f;
//			brain.neurons[2].value = 1.0f;
//		}
//		size_t newConnections = Tick(brain);
//		++tickCount;
//		std::printf("%zu new connections\n", newConnections);
//
//		{
//			float yscale = 0.98f / 64;
//			float xscale = yscale * height / width;
//
//			std::vector<Vertex> vertices(brain.neurons.size());
//			std::vector<Vertex> lineVertices;
//			for (size_t i = 0; i < brain.neurons.size(); ++i)
//			{
//				auto& a = brain.neurons[i];
//
//				vertices[i] = Vertex {
//					.x = a.x * xscale,
//					.y = a.y * yscale,
//					.r = a.value
//				};
//
//				for (auto& con : a.connections)
//				{
//					auto& b = brain.neurons[con.target];
//					lineVertices.emplace_back(Vertex {
//						.x = a.x * xscale,
//						.y = a.y * yscale,
//						.r = con.strength * a.value,
//					});
//					lineVertices.emplace_back(Vertex {
//						.x = b.x * xscale,
//						.y = b.y * yscale,
//						.r = con.strength * a.value,
//					});
//				}
//			}
//			glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
//			glBufferSubData(GL_ARRAY_BUFFER, 0, brain.neurons.size() * sizeof(Vertex), vertices.data());
//			glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//			glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
//			if (lineVertices.size() > lineCount)
//			{
//				glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(Vertex), lineVertices.data(), GL_DYNAMIC_DRAW);
//				lineCount = lineVertices.size();
//			}
//			else
//			{
//				glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(Vertex), lineVertices.data());
//			}
//			glBindBuffer(GL_ARRAY_BUFFER, 0);
//		}
//
//		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//		glUseProgram(shaderProgram);
//
//		glEnable(GL_POINT_SIZE);
//		glEnable(GL_LINE_WIDTH);
//		glBindVertexArray(vaos[1]);
//		glLineWidth(1.0f);
//		glDrawArrays(GL_LINES, 0, lineCount);
//
//		glBindVertexArray(vaos[0]);
//		glPointSize(2.0f);
//		glDrawArrays(GL_POINTS, 0, brain.neurons.size());
//
//		glBindBuffer(GL_ARRAY_BUFFER, 0);
//		glUseProgram(0);
//
//		glfwSwapBuffers(window);
//
//		/*using namespace std::chrono_literals;
//		std::this_thread::sleep_for(10ms);*/
//	}
//
//	glDeleteBuffers(2, vbos);
//	glDeleteProgram(shaderProgram);
//
//	glfwDestroyWindow(window);
//	glfwTerminate();
//	return 0;
// }