#pragma once

#include <vector>

struct Point
{
	float x, y;
};

Point operator+(Point lhs, Point rhs);
Point operator-(Point lhs, Point rhs);
Point operator*(Point lhs, Point rhs);
Point operator*(float lhs, Point rhs);
Point operator*(Point lhs, float rhs);
Point operator/(Point lhs, Point rhs);
Point operator/(float lhs, Point rhs);
Point operator/(Point lhs, float rhs);

inline Point& operator+=(Point& lhs, Point rhs)
{
	return lhs = lhs + rhs;
}
inline Point& operator-=(Point& lhs, Point rhs)
{
	return lhs = lhs - rhs;
}
inline Point& operator*=(Point& lhs, Point rhs)
{
	return lhs = lhs * rhs;
}
inline Point& operator*=(Point& lhs, float rhs)
{
	return lhs = lhs * rhs;
}
inline Point& operator/=(Point& lhs, Point rhs)
{
	return lhs = lhs / rhs;
}
inline Point& operator/=(Point& lhs, float rhs)
{
	return lhs = lhs / rhs;
}

struct Dendrite
{
	Point points[32];
	float maxLength;
};

struct Neuron
{
	Point    pos;
	Dendrite dendrites[256];

	size_t longest      = 0;
	size_t furthest     = 0;
	float  longestDist  = 0.0f;
	float  furthestDist = 0.0f;
};

void InitNeuron(Neuron& neuron);
void GrowNeuron(Neuron& neuron);