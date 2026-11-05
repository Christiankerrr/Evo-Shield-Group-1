#include "esNodeExplorer.h"
#include <array>
#include <numeric>
#include <algorithm>
#include <cmath>

namespace ES {

static Vec2 center(const AABB& b)
{
	return { (b.min.x + b.max.x) * 0.5f, (b.min.y + b.max.y) * 0.5f };
}

static float span(const std::array<float, 5>& v)
{
	float mn = v[0];
	float mx = v[0];

	for (auto f : v)
	{
		mn = std::min(mn, f);
		mx = std::max(mx, f);
	};

	return mx - mn;
}

void ESNodeExplorer::discoverHiddenNodes(const CPPN& cppn, Substrate& sub)
{
	AABB root{ {-1.f,-1.f}, {1.f,1.f} };
	refineCell(cppn, sub, root, 0);
}

void ESNodeExplorer::refineCell(const CPPN& cppn, Substrate& sub, const AABB& box, int depth)
{
	const Vec2 c = center(box);
	std::array<Vec2, 5> pts{ Vec2{box.min.x,box.min.y}, Vec2{box.max.x,box.min.y}, Vec2{box.max.x,box.max.y}, Vec2{box.min.x,box.max.y}, c };
	std::array<float, 5> gates{}, biases{};

	for (int i = 0; i < 5; ++i)
	{
		auto out = cppn.evalNode(pts[i].x, pts[i].y, 1.f);
		gates[i] = out.g;
		biases[i] = out.b;
	}

	const float E = *std::max_element(gates.begin(), gates.end());
	const float V = span(gates);

	if (E > cfg.tauExist && V > cfg.alphaVar && depth < cfg.maxDepth)
	{
		const Vec2 m = c;
		AABB q1{ {box.min.x, box.min.y}, {m.x, m.y} };
		AABB q2{ {m.x,      box.min.y}, {box.max.x, m.y} };
		AABB q3{ {m.x,      m.y},       {box.max.x, box.max.y} };
		AABB q4{ {box.min.x, m.y},      {m.x,      box.max.y} };

		refineCell(cppn, sub, q1, depth + 1);
		refineCell(cppn, sub, q2, depth + 1);
		refineCell(cppn, sub, q3, depth + 1);
		refineCell(cppn, sub, q4, depth + 1);

		return;
	}

	// Leaf -> spawn a hidden node at cell center with average bias
	Node h{};
	h.ID = static_cast<int>(sub.inputs.size() + sub.outputs.size() + sub.hiddens.size());
	h.pos = c;
	h.bias = std::accumulate(biases.begin(), biases.end(), 0.f) / static_cast<float>(biases.size());
	h.isHidden = true;
	sub.hiddens.push_back(h);
}

} // namespace ES
