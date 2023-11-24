#include "Brain.h"

#include <random>

constexpr auto PI = 3.1415926535897932384;

std::mt19937                          s_RNG(std::random_device {}());
std::uniform_real_distribution<float> s_ThetaDist(-PI, PI);
std::uniform_real_distribution<float> s_GrowthDist(0.0001f, 0.01f);

Point operator+(Point lhs, Point rhs)
{
	return { lhs.x + rhs.x, lhs.y + rhs.y };
}

Point operator-(Point lhs, Point rhs)
{
	return { lhs.x - rhs.x, lhs.y - rhs.y };
}

Point operator*(Point lhs, Point rhs)
{
	return { lhs.x * rhs.x, lhs.y * rhs.y };
}

Point operator*(float lhs, Point rhs)
{
	return { lhs * rhs.x, lhs * rhs.y };
}

Point operator*(Point lhs, float rhs)
{
	return { lhs.x * rhs, lhs.y * rhs };
}

Point operator/(Point lhs, Point rhs)
{
	return { lhs.x / rhs.x, lhs.y / rhs.y };
}

Point operator/(float lhs, Point rhs)
{
	return { lhs / rhs.x, lhs / rhs.y };
}

Point operator/(Point lhs, float rhs)
{
	return { lhs.x / rhs, lhs.y / rhs };
}

float length(Point p)
{
	return sqrtf(p.x * p.x + p.y * p.y);
}

float angle(Point p)
{
	return atan2f(p.y, p.x);
}

Point fromAngle(float angle)
{
	return { cosf(angle), sinf(angle) };
}

template <size_t N>
void GrowNeurite(Point (&points)[N], float maxLength, float growth, float angleSpread)
{
	float totalLength = 0.0f;
	Point pPos        = points[0];
	for (size_t i = 1; i < N; ++i)
	{
		Point delta  = points[i] - pPos;
		pPos         = points[i];
		totalLength += length(delta);
	}
	growth = std::min<float>(growth, maxLength - totalLength);
	if (growth == 0.0f)
		return;

	float len      = totalLength / (N - 1);
	points[0]      = { 0.0f, 0.0f };
	float curAngle = angle(points[N - 1] - points[N - 2]);
	for (size_t i = N - 1; i > 1; --i)
	{
		points[i - 1] = points[i] - fromAngle(curAngle) * len;
		curAngle      = angle(points[i - 1] - points[i - 2]);
	}
	for (size_t i = 1; i < N; ++i)
	{
		curAngle  = angle(points[i] - points[i - 1]);
		points[i] = points[i - 1] + fromAngle(curAngle) * len;
	}

	curAngle       = angle(points[N - 1] - points[N - 2]);
	float dAngle   = s_ThetaDist(s_RNG) * angleSpread;
	float newAngle = curAngle + dAngle;
	points[N - 1]  = points[N - 2] + fromAngle(newAngle) * (len + growth);
}

void InitNeuron(Neuron& neuron)
{
	neuron.pos = { 0.0f, 0.0f };
	for (size_t i = 0; i < 256; ++i)
	{
		for (size_t j = 0; j < 32; ++j)
		{
			neuron.dendrites[i].points[j] = { 0.0f, 0.0f };
			neuron.dendrites[i].maxLength = s_GrowthDist(s_RNG) * 500;
			if (neuron.dendrites[i].maxLength > neuron.longestDist)
			{
				neuron.longestDist = neuron.dendrites[i].maxLength;
				neuron.longest     = i;
			}
		}
	}

	for (size_t i = 0; i < 256; ++i)
		neuron.dendrites[i].points[31] = fromAngle(s_ThetaDist(s_RNG)) * neuron.dendrites[i].maxLength / 500;
}

float GrowthSpeed()
{
	return (100.0f - (1.0f / s_GrowthDist(s_RNG))) * 0.0001f;
}

void GrowNeuron(Neuron& neuron)
{
	neuron.furthest     = 0;
	neuron.furthestDist = 0.0f;
	for (size_t i = 0; i < 256; ++i)
	{
		float speed = s_GrowthDist(s_RNG);
		GrowNeurite(neuron.dendrites[i].points, neuron.dendrites[i].maxLength, speed, 2.0f * speed);

		float dist = length(neuron.dendrites[i].points[31]);
		if (dist > neuron.furthestDist)
		{
			neuron.furthestDist = dist;
			neuron.furthest     = i;
		}
	}

	neuron.dendrites[neuron.furthest].maxLength += s_GrowthDist(s_RNG);
	neuron.dendrites[neuron.furthest].maxLength  = std::min<float>(neuron.dendrites[neuron.furthest].maxLength, 10.0f);
	if (neuron.dendrites[neuron.furthest].maxLength > neuron.longestDist)
	{
		neuron.longestDist = neuron.dendrites[neuron.furthest].maxLength;
		neuron.longest     = neuron.furthest;
	}
}