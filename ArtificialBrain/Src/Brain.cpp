#include "Brain.h"

Brain::Brain()
	: m_RNG(std::random_device {}())
{
}

void Brain::Populate(size_t count, float minX, float maxX, float minY, float maxY)
{
	m_Quadtree.EnsureRegion(QuadtreePoint { minX, minY }, QuadtreePoint { maxX, maxY });
	m_Neurons.reserve(m_Neurons.size() + count);
	m_Connections.reserve(m_Connections.size() + 8 * count);
	for (size_t i = 0; i < count; ++i)
	{
		auto& neuron = m_Neurons.emplace_back(m_Neurons.size(), Point { RandFloat(minX, maxX), RandFloat(minY, maxY) }, (uint16_t) RandInt(5, 25));
		neuron.Id    = m_Quadtree.Insert(QuadtreePoint { neuron.Pos.x, neuron.Pos.y }, neuron.Index);
		neuron.Connections.reserve(8);
	}
}

void Brain::Write(size_t neuron, float value)
{
	if (neuron >= m_Neurons.size())
		return;
	m_Neurons[neuron].NextValue = value;
}

float Brain::Read(size_t neuron)
{
	if (neuron >= m_Neurons.size())
		return 0.0f;
	return m_Neurons[neuron].Value;
}

size_t Brain::Tick()
{
	float  maxDistance = 3.5f;
	size_t newCons     = 0;

	for (auto& neuron : m_Neurons)
	{
		neuron.Value     = std::min(neuron.NextValue, 1.0f); // / (neuron.Connections.empty() ? 1.0f : neuron.MaxStrength);
		neuron.NextValue = 0.0f;
		if (neuron.CanConnect && neuron.Value != 0.0f)
			++neuron.ConnectTick;
	}
	for (auto& con : m_Connections)
	{
		auto& a = m_Neurons[con.NeuronA];
		auto& b = m_Neurons[con.NeuronB];

		float deltaStrengthA = 0.0f;
		float deltaStrengthB = 0.0f;
		float deltaMinA      = 0.0f;
		float deltaMinB      = 0.0f;
		float deltaMaxA      = 0.0f;
		float deltaMaxB      = 0.0f;
		if (a.Value >= con.BiasMinA && a.Value <= con.BiasMaxA)
		{
			b.NextValue   += a.Value * con.StrengthA;
			deltaStrengthA = (0.8f - con.StrengthA) * RandFloat(0.0f, 0.001f);
			float force    = RandFloat(0.0f, 0.0001f);
			deltaMinA      = (a.Value - con.BiasMinA) * force;
			deltaMaxA      = (a.Value - con.BiasMaxA) * force;
		}
		else
		{
			deltaStrengthA = (0.05f - con.StrengthA) * RandFloat(0.0f, 0.0005f);
			float force    = RandFloat(0.0f, 0.0005f);
			deltaMinA      = (-0.1f - con.BiasMinA) * force;
			deltaMaxA      = (1.1f - con.BiasMaxA) * force;
		}
		if (b.Value >= con.BiasMinB && b.Value <= con.BiasMaxB)
		{
			a.NextValue   += b.Value * con.StrengthB;
			deltaStrengthB = (0.8f - con.StrengthB) * RandFloat(0.0f, 0.001f);
			float force    = RandFloat(0.0f, 0.0001f);
			deltaMinB      = (b.Value - con.BiasMinB) * force;
			deltaMaxB      = (b.Value - con.BiasMaxB) * force;
		}
		else
		{
			deltaStrengthB = (0.05f - con.StrengthB) * RandFloat(0.0f, 0.0005f);
			float force    = RandFloat(0.0f, 0.0005f);
			deltaMinB      = (-0.1f - con.BiasMinB) * force;
			deltaMaxB      = (1.1f - con.BiasMaxB) * force;
		}
		con.StrengthA += deltaStrengthA;
		con.StrengthB += deltaStrengthB;
		con.BiasMinA  += deltaMinA;
		con.BiasMinB  += deltaMinB;
		con.BiasMaxA  += deltaMaxA;
		con.BiasMaxB  += deltaMaxB;
		a.MaxStrength += deltaStrengthA;
		b.MaxStrength += deltaStrengthB;
	}
	for (auto& neuron : m_Neurons)
	{
		if (neuron.ConnectTick < neuron.ConnectTicks)
			continue;

		QuadtreeNeighbour neighbours[64];
		size_t            maxNeighbours = 64;

		neuron.ConnectTick = 0;
		size_t count       = m_Quadtree.FindNeighbours(QuadtreePoint { neuron.Pos.x, neuron.Pos.y }, maxDistance, neighbours, maxNeighbours);
		if (maxNeighbours > count)
		{
			// TODO(MarcasRealAccount): add logging maybe?
		}

		for (size_t i = 0; i < count; ++i)
		{
			auto& neighbour = neighbours[i];
			if (neighbour.data->value >= m_Neurons.size())
				break;
			if (neighbour.data->value == neuron.Index)
				continue;

			Neuron& otherNeuron = m_Neurons[neighbour.data->value];
			float   dx          = otherNeuron.Pos.x - neuron.Pos.x;
			float   dy          = otherNeuron.Pos.y - neuron.Pos.y;
			bool    isOccluded  = false;
			for (auto& j : neuron.Connections)
			{
				auto& con         = m_Connections[j];
				auto& thirdNeuron = m_Neurons[con.NeuronA == neuron.Index ? con.NeuronB : con.NeuronA];
				if (thirdNeuron.Index == otherNeuron.Index)
				{
					isOccluded = true;
					break;
				}
				float dx2   = thirdNeuron.Pos.x - neuron.Pos.x;
				float dy2   = thirdNeuron.Pos.y - neuron.Pos.y;
				float len   = sqrtf(dx2 * dx2 + dy2 * dy2);
				float angle = std::acosf((dx * dx2 + dy * dy2) / (neighbour.distance * len));
				if (angle < 3.1415926535897932384f / 8)
				{
					isOccluded = true;
					break;
				}
			}
			if (isOccluded)
				continue;
			for (auto& j : otherNeuron.Connections)
			{
				auto& con         = m_Connections[j];
				auto& thirdNeuron = m_Neurons[con.NeuronA == otherNeuron.Index ? con.NeuronB : con.NeuronA];
				if (thirdNeuron.Index == otherNeuron.Index)
				{
					isOccluded = true;
					break;
				}
				float dx2   = thirdNeuron.Pos.x - otherNeuron.Pos.x;
				float dy2   = thirdNeuron.Pos.y - otherNeuron.Pos.y;
				float len   = sqrtf(dx2 * dx2 + dy2 * dy2);
				float angle = std::acosf((dx * dx2 + dy * dy2) / (neighbour.distance * len));
				if (angle < 3.1415926535897932384f / 8)
				{
					isOccluded = true;
					break;
				}
			}
			if (isOccluded)
				continue;

			AddConnection(neuron, otherNeuron);
			newCons += 2;
		}
	}
	return newCons;
}

float Brain::RandFloat(float min, float max)
{
	return std::uniform_real_distribution(min, max)(m_RNG);
}

int Brain::RandInt(int min, int max)
{
	return std::uniform_int_distribution(min, max)(m_RNG);
}

void Brain::AddConnection(Neuron& a, Neuron& b)
{
	a.Connections.emplace_back(m_Connections.size());
	b.Connections.emplace_back(m_Connections.size());
	auto& con      = m_Connections.emplace_back(a.Index, b.Index);
	con.BiasMinA   = RandFloat(-0.1f, 1.1f);
	con.BiasMinB   = RandFloat(-0.1f, 1.1f);
	con.BiasMaxA   = RandFloat(-0.1f, 1.1f);
	con.BiasMaxB   = RandFloat(-0.1f, 1.1f);
	con.StrengthA  = RandFloat(0.05f, 0.8f);
	con.StrengthB  = RandFloat(0.05f, 0.8f);
	a.MaxStrength += con.StrengthB;
	b.MaxStrength += con.StrengthA;
}