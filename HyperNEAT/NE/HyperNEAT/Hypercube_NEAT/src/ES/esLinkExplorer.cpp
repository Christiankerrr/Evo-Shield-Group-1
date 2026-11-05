#include "esLinkExplorer.h"
#include <algorithm>
#include <cmath>

namespace ES {

static Vec2 center(const ESLinkExplorer::Box& b)
{
	Vec2 centerReturn = { (b.min.x + b.max.x) * 0.5f, (b.min.y + b.max.y) * 0.5f };
	return centerReturn;
}

static float span(const std::vector<float>& v)
{
	if (v.empty())
	{
		return 0.f;
	};

	auto mm = std::minmax_element(v.begin(), v.end());
	auto mmDiff = *mm.second - *mm.first;
	return mmDiff;
}

static bool contains(const ESLinkExplorer::Box& b, const Vec2& p)
{
	bool inX = (p.x >= b.min.x && p.x <= b.max.x);
	bool inY = (p.y >= b.min.y && p.y <= b.max.y);
	bool inside = inX && inY;
	return inside;
}

void ESLinkExplorer::removeLinks(const CPPN& cppn, const Substrate& sub, std::vector<Link>& links)
{
	links.clear();

	// pres = inputs + hiddens
	std::vector<Node*> pres;
	pres.reserve(sub.inputs.size() + sub.hiddens.size());
	for (auto& n : sub.inputs)  pres.push_back(const_cast<Node*>(&n));
	for (auto& n : sub.hiddens) pres.push_back(const_cast<Node*>(&n));

	// posts = hiddens + outputs
	std::vector<Node*> posts;
	posts.reserve(sub.hiddens.size() + sub.outputs.size());

	for (auto& n : sub.hiddens) posts.push_back(const_cast<Node*>(&n));
	for (auto& n : sub.outputs) posts.push_back(const_cast<Node*>(&n));

	bool noPres = pres.empty();
	bool noPosts = posts.empty();
	bool nothingToProcess = (noPres || noPosts);
	if (nothingToProcess) return;

	// Compute bounding box over posts
	Box box{ posts[0]->pos, posts[0]->pos };

	for (auto* p : posts)
	{
		bool smallerX = (p->pos.x < box.min.x);
		bool smallerY = (p->pos.y < box.min.y);
		bool largerX = (p->pos.x > box.max.x);
		bool largerY = (p->pos.y > box.max.y);

		if (smallerX) box.min.x = p->pos.x;
		if (smallerY) box.min.y = p->pos.y;
		if (largerX) box.max.x = p->pos.x;
		if (largerY) box.max.y = p->pos.y;
	}

	const float pad = 0.02f;

	box.min.x -= pad;
	box.min.y -= pad;
	box.max.x += pad;
	box.max.y += pad;

	refineCell(cppn, pres, posts, box, 0, links);
}


void ESLinkExplorer::refineCell(const CPPN& cppn, const std::vector<Node*>& pres, const std::vector<Node*>& posts, const Box& region, int depth, std::vector<Link>& out)
{
	std::vector<Node*> localPosts;
	localPosts.reserve(posts.size());

	for (auto* p : posts)
	{
		if (contains(region, p->pos))
		{
			localPosts.push_back(p);
		}
	}

	if (localPosts.empty()) return;

	std::vector<float> gates; gates.reserve(localPosts.size() * 8);
	for (auto* post : localPosts)
	{
		const int sampleCount = static_cast<int>(std::min<std::size_t>(8, pres.size()));
		for (int i = 0; i < sampleCount; ++i)
		{
			auto* pre = pres[i];
			const float d = dist(pre->pos, post->pos);
			const float ang = angle(pre->pos, post->pos);
			auto outL = cppn.evalLink(pre->pos.x, pre->pos.y, post->pos.x, post->pos.y, d, ang, /*sameComm placeholder*/0.f, /*degPre placeholder*/0.f, /*degPost placeholder*/0.f, 1.f);
			gates.push_back(outL.g);
		}
	}

	const float E = gates.empty() ? 0.f : *std::max_element(gates.begin(), gates.end());
	const float V = span(gates);

	if (E > cfg.tauExist && V > cfg.alphaVar && depth < cfg.maxDepth)
	{
		const Vec2 c = center(region);
		Box q1{ region.min, c };
		Box q2{ {c.x, region.min.y}, {region.max.x, c.y} };
		Box q3{ c, region.max };
		Box q4{ {region.min.x, c.y}, {c.x, region.max.y} };

		refineCell(cppn, pres, posts, q1, depth + 1, out);
		refineCell(cppn, pres, posts, q2, depth + 1, out);
		refineCell(cppn, pres, posts, q3, depth + 1, out);
		refineCell(cppn, pres, posts, q4, depth + 1, out);

		return;
	}

	// Leaf: gate + top-K fan-in + weight floor + distance decay
	for (auto* post : localPosts)
	{
		struct Cand
		{
			int preID;
			float w;
		};

		std::vector<Cand> cands; cands.reserve(pres.size());

		for (auto* pre : pres)
		{
			const float d = dist(pre->pos, post->pos);
			const float ang = angle(pre->pos, post->pos);
			auto outL = cppn.evalLink(pre->pos.x, pre->pos.y, post->pos.x, post->pos.y, d, ang, /*sameComm placeholder*/0.f, /*degPre placeholder*/0.f, /*degPost placeholder*/0.f, 1.f);
			if (outL.g > cfg.tauExist)
			{
				float w = outL.w * std::exp(-cfg.betaDist * d);
				if (std::fabs(w) >= cfg.weightFloor)
				{
					cands.push_back({ pre->ID, w });
				}
			}
		}

		std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b)
			{
				return std::fabs(a.w) > std::fabs(b.w);
			});

		if (static_cast<int>(cands.size()) > cfg.fanInK)
		{
			cands.resize(cfg.fanInK);
		}

		for (auto& c : cands)
		{
			Link L{}; L.preID = c.preID; L.postID = post->ID; L.w = c.w;
			out.push_back(L);
		}
	}
}

} // namespace ES
