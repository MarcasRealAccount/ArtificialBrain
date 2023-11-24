#include "Quadtree.h"

bool RectCircleOverlap(QuadtreePoint min, QuadtreePoint max, QuadtreePoint pos, float radius)
{
	float dx = pos.x - std::max<float>(min.x, std::min<float>(pos.x, max.x));
	float dy = pos.y - std::max<float>(min.y, std::min<float>(pos.y, max.y));
	return (dx * dx + dy * dy) <= (radius * radius);
}

Quadtree::Quadtree(QuadtreePoint min, QuadtreePoint max)
	: m_Min(min),
	  m_Max(max)
{
	QuadtreeNode  root {};
	QuadtreeLeaf* leaf = (QuadtreeLeaf*) &root;
	*leaf              = QuadtreeLeaf(0, 0);
	m_Nodes.emplace_back(root);
	m_LeafDatas.emplace_back();
}

Quadtree::Quadtree(Quadtree&& move) noexcept
	: m_Min(move.m_Min),
	  m_Max(move.m_Max),
	  m_Nodes(std::move(move.m_Nodes)),
	  m_LeafDatas(std::move(move.m_LeafDatas))
{
}

Quadtree::~Quadtree()
{
}

Quadtree& Quadtree::operator=(Quadtree&& move) noexcept
{
	m_Min       = move.m_Min;
	m_Max       = move.m_Max;
	m_Nodes     = std::move(move.m_Nodes);
	m_LeafDatas = std::move(move.m_LeafDatas);
	return *this;
}

size_t Quadtree::Insert(QuadtreePoint pos, size_t value)
{
	if (pos.x < m_Min.x || pos.y < m_Min.y ||
		pos.x > m_Max.x || pos.y > m_Max.y)
	{
		QuadtreePoint min {
			std::min<float>(m_Min.x, pos.x),
			std::min<float>(m_Min.y, pos.y),
		};
		QuadtreePoint max {
			std::max<float>(m_Max.x, pos.x),
			std::max<float>(m_Max.y, pos.y),
		};
		Resize(min, max);
	}

	size_t path = ComputePath(pos);
	size_t node = GetPathNode(path);
	if (ShouldLeafSplit(node))
		SplitLeafNode(node);
	node = GetPathNode(path);

	QuadtreeLeaf* pLeaf = (QuadtreeLeaf*) &m_Nodes[node];
	size_t        id    = path << 32 | GetNewID(node);
	if (pLeaf->count == sizeof(QuadtreeLeafData) / sizeof(QuadtreeLeaf))
		return ~0ULL;
	m_LeafDatas[pLeaf->data].datas[pLeaf->count++] = QuadtreeData(pos, id, value);
	return id;
}

void Quadtree::Erase(size_t id)
{
	size_t        node  = GetPathNode(id >> 32);
	QuadtreeLeaf* pLeaf = (QuadtreeLeaf*) &m_Nodes[node];
	if (!pLeaf->leaf)
		return;

	auto& datas = m_LeafDatas[pLeaf->data];
	for (size_t i = 0; i < pLeaf->count; ++i)
	{
		if (datas.datas[i].id == id)
		{
			if (i == pLeaf->count - 1)
			{
				datas.datas[i] = QuadtreeData();
			}
			else
			{
				memmove(&datas.datas[i], &datas.datas[i + 1], (pLeaf->count - i) * sizeof(QuadtreeLeaf));
			}
			--pLeaf->count;
			return;
		}
	}
}

size_t Quadtree::FindNeighbours(QuadtreePoint pos, float maxDistance, QuadtreeNeighbour* neighbours, size_t& maxNeighbours)
{
	size_t totalFound = 0;
	size_t count      = 0;
	VisitNodesTemp([&](size_t node, QuadtreePoint min, QuadtreePoint max) -> EQuadtreeVisit {
		if (!RectCircleOverlap(min, max, pos, maxDistance))
			return EQuadtreeVisit::Skip;
		if (!VisitDatasTemp(node, [&](const QuadtreeData& data) -> bool {
				float dx   = data.pos.x - pos.x;
				float dy   = data.pos.y - pos.y;
				float dist = sqrtf(dx * dx + dy * dy);
				if (dist > maxDistance)
					return true;

				++totalFound;
				auto it = std::lower_bound(neighbours, neighbours + count, dist, [](const QuadtreeNeighbour& a, float dist) {
					return a.distance < dist;
				});

				if (it == neighbours + count)
				{
					if (count >= maxNeighbours)
						return true;
					++count;
				}
				else if (it < neighbours + count - 1)
				{
					if (count >= maxNeighbours)
					{
						memmove(it + 1, it, ((neighbours + count - 1) - it) * sizeof(QuadtreeNeighbour));
					}
					else
					{
						memmove(it + 1, it, ((neighbours + count) - it) * sizeof(QuadtreeNeighbour));
						++count;
					}
				}
				*it = QuadtreeNeighbour {
					.data     = &data,
					.distance = dist
				};
				return true;
			}))
			return EQuadtreeVisit::Exit;
		return EQuadtreeVisit::Continue;
	});
	maxNeighbours = totalFound;
	return count;
}

bool Quadtree::EnsureRegion(QuadtreePoint min, QuadtreePoint max)
{
	if (min.x < m_Min.x || min.y < m_Min.y ||
		max.x > m_Max.x || max.y > m_Max.y)
	{
		min = {
			std::min<float>(min.x, m_Min.x),
			std::min<float>(min.y, m_Min.y),
		};
		max = {
			std::max<float>(max.x, m_Max.x),
			std::max<float>(max.y, m_Max.y),
		};
		return Resize(min, max);
	}
	return true;
}

bool Quadtree::Resize(QuadtreePoint min, QuadtreePoint max)
{
	Quadtree resizedQuadtree(min, max);
	if (!VisitNodesTemp([this, &resizedQuadtree](size_t node, [[maybe_unused]] QuadtreePoint min, [[maybe_unused]] QuadtreePoint max) -> EQuadtreeVisit {
			if (!VisitDatasTemp(node, [&resizedQuadtree](const QuadtreeData& data) -> bool {
					return resizedQuadtree.InsertData(data);
				}))
				return EQuadtreeVisit::Exit;
			return EQuadtreeVisit::Continue;
		}))
		return false;
	*this = std::move(resizedQuadtree);
	return true;
}

void Quadtree::Clear()
{
	m_Nodes.clear();
	m_LeafDatas.clear();

	QuadtreeNode  root {};
	QuadtreeLeaf* leaf = (QuadtreeLeaf*) &root;
	*leaf              = QuadtreeLeaf(0, 0);
	m_Nodes.emplace_back(root);
	m_LeafDatas.emplace_back();
}

bool Quadtree::VisitNodes(EQuadtreeVisit (*visitor)(void* userdata, size_t node, QuadtreePoint min, QuadtreePoint max), void* userdata) const
{
	return VisitNodesInt(0, m_Min, m_Max, visitor, userdata);
}

bool Quadtree::VisitDatas(size_t node, bool (*visitor)(void* userdata, const QuadtreeData& data), void* userdata) const
{
	if (node >= m_Nodes.size())
		return true;
	const QuadtreeNode* pNode = &m_Nodes[node];
	if (!pNode->leaf)
		return true;

	QuadtreeLeaf* leaf = (QuadtreeLeaf*) pNode;
	if (leaf->data >= m_LeafDatas.size())
		return true;
	const QuadtreeLeafData& leafData = m_LeafDatas[leaf->data];
	for (size_t i = 0; i < leaf->count; ++i)
	{
		if (!visitor(userdata, leafData.datas[i]))
			return false;
	}
	return true;
}

QuadtreeNode* Quadtree::GetNode(size_t node)
{
	if (node >= m_Nodes.size())
		return nullptr;
	return &m_Nodes[node];
}

QuadtreeData* Quadtree::GetData(size_t id)
{
	size_t        node  = GetPathNode(id >> 32);
	QuadtreeLeaf* pLeaf = (QuadtreeLeaf*) &m_Nodes[node];
	if (!pLeaf->leaf)
		return nullptr;

	for (size_t i = 0; i < pLeaf->count; ++i)
	{
		if (m_LeafDatas[pLeaf->data].datas[i].id == id)
			return &m_LeafDatas[pLeaf->data].datas[i];
	}
	return nullptr;
}

bool Quadtree::VisitNodesInt(size_t node, QuadtreePoint min, QuadtreePoint max, EQuadtreeVisit (*visitor)(void* userdata, size_t node, QuadtreePoint min, QuadtreePoint max), void* userdata) const
{
	if (node >= m_Nodes.size())
		return true;
	EQuadtreeVisit visit = visitor(userdata, node, min, max);
	switch (visit)
	{
	case EQuadtreeVisit::Continue:
	{
		QuadtreePoint curMid = {
			min.x + (max.x - min.x) / 2,
			min.y + (max.y - min.y) / 2,
		};

		const QuadtreeNode* pNode = &m_Nodes[node];
		if (!pNode->leaf)
		{
			if (!VisitNodesInt(pNode->children[0], min, curMid, visitor, userdata))
				return false;
			if (!VisitNodesInt(pNode->children[1], { curMid.x, min.y }, { max.x, curMid.y }, visitor, userdata))
				return false;
			if (!VisitNodesInt(pNode->children[2], { min.x, curMid.y }, { curMid.x, max.y }, visitor, userdata))
				return false;
			if (!VisitNodesInt(pNode->children[3], curMid, max, visitor, userdata))
				return false;
		}
		return true;
	}
	case EQuadtreeVisit::Skip:
		return true;
	case EQuadtreeVisit::Exit:
		return false;
	}
	return true;
}

size_t Quadtree::ComputePath(QuadtreePoint pos, QuadtreePoint* min, QuadtreePoint* max) const
{
	QuadtreePoint curMin = m_Min;
	QuadtreePoint curMax = m_Max;
	QuadtreePoint curMid = {
		curMin.x + (curMax.x - curMin.x) / 2,
		curMin.y + (curMax.y - curMin.y) / 2,
	};
	size_t path = 0;
	for (size_t i = 0; i < 16; ++i)
	{
		size_t childPath = (pos.x >= curMid.x) |
						   ((pos.y >= curMid.y) << 1);
		path |= childPath << (2 * i);
		switch (childPath)
		{
		case 0b00:
			curMax = curMid;
			break;
		case 0b01:
			curMin.x = curMid.x;
			curMax.y = curMid.y;
			break;
		case 0b10:
			curMax.x = curMid.x;
			curMin.y = curMid.y;
			break;
		case 0b11:
			curMin = curMid;
			break;
		}
		if (i < 15)
		{
			curMid = {
				curMin.x + (curMax.x - curMin.x) / 2,
				curMin.y + (curMax.y - curMin.y) / 2,
			};
		}
	}
	if (min)
		*min = curMin;
	if (max)
		*max = curMax;
	return path;
}

size_t Quadtree::GetPathNode(size_t path) const
{
	size_t              node    = 0;
	const QuadtreeNode* current = &m_Nodes[0];
	for (size_t i = 0; i < 16 && !current->leaf; ++i)
	{
		size_t childPath = (path >> (2 * i)) & 0b11;
		node             = current->children[childPath];
		current          = &m_Nodes[node];
	}
	return node;
}

bool Quadtree::ShouldLeafSplit(size_t node) const
{
	if (node >= m_Nodes.size())
		return false;
	const QuadtreeNode* pNode = &m_Nodes[node];
	if (!pNode->leaf ||
		GetNodeDepth(node) == 15)
		return false;

	return ((QuadtreeLeaf*) pNode)->count == sizeof(QuadtreeLeafData) / sizeof(QuadtreeData);
}

void Quadtree::SplitLeafNode(size_t node)
{
	if (node >= m_Nodes.size())
		return;
	QuadtreeNode* pNode = &m_Nodes[node];
	if (!pNode->leaf)
		return;

	QuadtreePoint min, max;
	size_t        depth = GetNodeRegion(node, min, max);
	if (depth == 16)
		return;
	QuadtreePoint mid = {
		min.x + (max.x - min.x) / 2,
		min.y + (max.y - min.y) / 2,
	};

	QuadtreeLeafData oldLeafData;
	QuadtreeLeaf*    leaf  = (QuadtreeLeaf*) pNode;
	size_t           count = leaf->count;
	std::swap(oldLeafData, m_LeafDatas[leaf->data]);

	{
		size_t        firstChild    = m_Nodes.size();
		size_t        firstLeafData = m_LeafDatas.size();
		QuadtreeNode  child;
		QuadtreeLeaf* childLeaf = (QuadtreeLeaf*) &child;
		*childLeaf              = QuadtreeLeaf(node, leaf->data);
		m_Nodes.emplace_back(child);
		childLeaf->data = firstLeafData;
		m_Nodes.emplace_back(child);
		m_LeafDatas.emplace_back();
		++childLeaf->data;
		m_Nodes.emplace_back(child);
		m_LeafDatas.emplace_back();
		++childLeaf->data;
		m_Nodes.emplace_back(child);
		m_LeafDatas.emplace_back();
		pNode->leaf        = false;
		pNode->children[0] = firstChild;
		pNode->children[1] = firstChild + 1;
		pNode->children[2] = firstChild + 2;
		pNode->children[3] = firstChild + 3;
	}
	for (size_t i = 0; i < count; ++i)
	{
		auto&  data = oldLeafData.datas[i];
		size_t path = (data.pos.x >= mid.x) |
					  ((data.pos.y >= mid.y) << 1);

		QuadtreeLeaf* childLeaf = (QuadtreeLeaf*) &m_Nodes[pNode->children[path]];

		m_LeafDatas[childLeaf->data].datas[childLeaf->count++] = data;
	}
}

size_t Quadtree::GetNodeDepth(size_t node) const
{
	if (node >= m_Nodes.size())
		return ~0ULL;

	size_t              depth   = 0;
	const QuadtreeNode* current = &m_Nodes[node];
	while (current->parent != node)
	{
		node    = current->parent;
		current = &m_Nodes[node];
		++depth;
	}
	return depth;
}

size_t Quadtree::GetNodeRegion(size_t node, QuadtreePoint& min, QuadtreePoint& max) const
{
	if (node >= m_Nodes.size())
		return ~0ULL;

	size_t depth                = 0;
	min                         = { -1.0f, -1.0f };
	max                         = { 1.0f, 1.0f };
	QuadtreePoint       mid     = { 0.0f, 0.0f };
	const QuadtreeNode* current = &m_Nodes[node];
	while (current->parent != node)
	{
		const QuadtreeNode* parent = &m_Nodes[current->parent];
		if (parent->children[0] == node)
		{
			min.x /= 2;
			min.y /= 2;
			max    = { mid.x / 2, mid.y / 2 };
		}
		else if (parent->children[1] == node)
		{
			min.x  = mid.x / 2;
			min.y /= 2;
			max.x /= 2;
			max.y  = mid.y / 2;
		}
		else if (parent->children[2] == node)
		{
			min.x /= 2;
			min.y  = mid.y / 2;
			max.x  = mid.x / 2;
			max.y /= 2;
		}
		else if (parent->children[3] == node)
		{
			min    = { mid.x / 2, mid.y / 2 };
			max.x /= 2;
			max.y /= 2;
		}
		mid = {
			min.x + (max.x - min.x) / 2,
			min.y + (max.y - min.y) / 2,
		};
		++depth;
		node = current->parent;
	}
	float width  = m_Max.x - m_Min.x;
	float height = m_Max.y - m_Min.y;
	min.x       *= width;
	min.y       *= height;
	max.x       *= width;
	max.y       *= height;
	min.x       += m_Min.x;
	min.y       += m_Min.y;
	max.x       += m_Min.x;
	max.y       += m_Min.y;
	return depth;
}

size_t Quadtree::GetNewID(size_t node) const
{
	if (node >= m_Nodes.size())
		return ~0ULL;
	QuadtreeLeaf* pLeaf = (QuadtreeLeaf*) &m_Nodes[node];
	if (!pLeaf->leaf)
		return ~0ULL;

	size_t id = 0;
	for (size_t i = 0; i < pLeaf->count; ++i)
	{
		if ((m_LeafDatas[pLeaf->data].datas[i].id & 0xFFFFFFFF) == id)
			++id;
	}
	return id;
}

bool Quadtree::InsertData(const QuadtreeData& data)
{
	size_t node = GetPathNode(data.id >> 32);
	if (ShouldLeafSplit(node))
		SplitLeafNode(node);
	node = GetPathNode(data.id >> 32);

	QuadtreeLeaf* pLeaf = (QuadtreeLeaf*) &m_Nodes[node];
	if (pLeaf->count == sizeof(QuadtreeLeafData) / sizeof(QuadtreeLeaf))
		return false;
	m_LeafDatas[pLeaf->data].datas[pLeaf->count++] = data;
	return true;
}