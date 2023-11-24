#pragma once

#include "Quadtree.h"

#include <cstddef>
#include <cstdint>

#include <random>
#include <variant>
#include <vector>

struct Point
{
	float x, y;
};

struct Connection
{
public:
	size_t NeuronA, NeuronB;
	float  BiasMinA, BiasMinB;
	float  BiasMaxA, BiasMaxB;
	float  StrengthA, StrengthB;

	Connection()
		: NeuronA(0),
		  NeuronB(0),
		  BiasMinA(0.0f),
		  BiasMinB(0.0f),
		  BiasMaxA(0.0f),
		  BiasMaxB(0.0f),
		  StrengthA(0.0f),
		  StrengthB(0.0f) {}

	Connection(size_t a, size_t b)
		: NeuronA(a),
		  NeuronB(b),
		  BiasMinA(0.0f),
		  BiasMinB(0.0f),
		  BiasMaxA(0.0f),
		  BiasMaxB(0.0f),
		  StrengthA(0.0f),
		  StrengthB(0.0f) {}
};

struct Neuron
{
public:
	size_t   Index;
	size_t   Id;
	Point    Pos;
	float    Value, NextValue;
	float    MaxStrength;
	uint16_t ConnectTick, ConnectTicks;
	bool     CanConnect;

	std::vector<size_t> Connections;

	Neuron()
		: Index(~0ULL),
		  Id(~0ULL),
		  Pos { 0.0f, 0.0f },
		  Value(0.0f),
		  NextValue(0.0f),
		  MaxStrength(0.0f),
		  ConnectTick(0),
		  ConnectTicks(0),
		  CanConnect(true) {}

	Neuron(size_t index, Point pos, uint16_t connectTicks)
		: Index(index),
		  Id(~0ULL),
		  Pos(pos),
		  Value(0.0f),
		  NextValue(0.0f),
		  MaxStrength(0.0f),
		  ConnectTick(0),
		  ConnectTicks(connectTicks),
		  CanConnect(true) {}
};

class Brain
{
public:
	Brain();

	void Populate(size_t count, float minX, float maxX, float minY, float maxY);

	void  Write(size_t neuron, float value);
	float Read(size_t neuron);

	size_t Tick();

	float RandFloat(float min = 0.0f, float max = 1.0f);
	int   RandInt(int min, int max);

	auto& GetQuadtree() const { return m_Quadtree; }
	auto& GetNeurons() const { return m_Neurons; }
	auto& GetConnections() const { return m_Connections; }

private:
	void AddConnection(Neuron& a, Neuron& b);

private:
	Quadtree                m_Quadtree;
	std::vector<Neuron>     m_Neurons;
	std::vector<Connection> m_Connections;
	std::mt19937            m_RNG;
};