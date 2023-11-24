#pragma once

#include <cstddef>
#include <cstdint>

#include <vector>

struct QuadtreePoint
{
	float x, y;
};

enum class EQuadtreeVisit : uint8_t
{
	Continue = 0,
	Skip,
	Exit
};

struct QuadtreeNode
{
	size_t parent : 8 * sizeof(size_t) - 1;
	size_t leaf   : 1;
	size_t children[4];

	QuadtreeNode()
		: parent(~0ULL),
		  leaf(false),
		  children { ~0ULL, ~0ULL, ~0ULL, ~0ULL } {}
	QuadtreeNode(size_t parent)
		: parent(parent),
		  leaf(false),
		  children { ~0ULL, ~0ULL, ~0ULL, ~0ULL } {}
};

struct QuadtreeLeaf
{
	size_t parent : 8 * sizeof(size_t) - 1;
	size_t leaf   : 1;
	size_t data;
	size_t count;

	QuadtreeLeaf()
		: parent(~0ULL),
		  leaf(true),
		  data(~0ULL),
		  count(0) {}
	QuadtreeLeaf(size_t parent, size_t data)
		: parent(parent),
		  leaf(true),
		  data(data),
		  count(0) {}
};

struct QuadtreeData
{
	QuadtreePoint pos;
	size_t        id;
	size_t        value;

	QuadtreeData()
		: pos({ 0.0f, 0.0f }),
		  id(~0ULL),
		  value(~0ULL) {}

	QuadtreeData(QuadtreePoint pos, size_t id, size_t value)
		: pos(pos),
		  id(id),
		  value(value) {}
};

struct QuadtreeLeafData
{
	QuadtreeData datas[32];
};

struct QuadtreeNeighbour
{
	const QuadtreeData* data;
	float               distance;
};

struct Quadtree
{
public:
	Quadtree(QuadtreePoint min = { -1.0f, -1.0f }, QuadtreePoint max = { 1.0f, 1.0f });
	Quadtree(Quadtree&& move) noexcept;
	~Quadtree();

	Quadtree& operator=(Quadtree&& move) noexcept;

	size_t Insert(QuadtreePoint pos, size_t value);
	void   Erase(size_t id);

	size_t FindNeighbours(QuadtreePoint pos, float maxDistance, QuadtreeNeighbour* neighbours, size_t& maxNeighbours);

	bool EnsureRegion(QuadtreePoint min, QuadtreePoint max);
	bool Resize(QuadtreePoint min, QuadtreePoint max);
	void Clear();

	bool VisitNodes(EQuadtreeVisit (*visitor)(void* userdata, size_t node, QuadtreePoint min, QuadtreePoint max), void* userdata) const;
	bool VisitDatas(size_t node, bool (*visitor)(void* userdata, const QuadtreeData& data), void* userdata) const;

	bool VisitNodesTemp(auto&& visitor) const
	{
		struct Userdata
		{
			decltype(visitor) func;
		} userdata { std::move(visitor) };

		auto callback = +[](void* userdata, size_t node, QuadtreePoint min, QuadtreePoint max) -> EQuadtreeVisit {
			return ((Userdata*) userdata)->func(node, min, max);
		};

		return VisitNodes(callback, &userdata);
	}

	bool VisitDatasTemp(size_t node, auto&& visitor) const
	{
		struct Userdata
		{
			decltype(visitor) func;
		} userdata { std::move(visitor) };

		auto callback = +[](void* userdata, const QuadtreeData& data) -> bool {
			return ((Userdata*) userdata)->func(data);
		};

		return VisitDatas(node, callback, &userdata);
	}

	QuadtreeNode* GetNode(size_t node);
	QuadtreeData* GetData(size_t id);

private:
	bool VisitNodesInt(size_t node, QuadtreePoint min, QuadtreePoint max, EQuadtreeVisit (*visitor)(void* userdata, size_t node, QuadtreePoint min, QuadtreePoint max), void* userdata) const;

	size_t ComputePath(QuadtreePoint pos, QuadtreePoint* min = nullptr, QuadtreePoint* max = nullptr) const;
	size_t GetPathNode(size_t path) const;
	bool   ShouldLeafSplit(size_t node) const;
	void   SplitLeafNode(size_t node);
	size_t GetNodeRegion(size_t node, QuadtreePoint& min, QuadtreePoint& max) const;
	size_t GetNodeDepth(size_t node) const;

	size_t GetNewID(size_t node) const;

	bool InsertData(const QuadtreeData& data);

private:
	QuadtreePoint                 m_Min, m_Max;
	std::vector<QuadtreeNode>     m_Nodes;
	std::vector<QuadtreeLeafData> m_LeafDatas;
};