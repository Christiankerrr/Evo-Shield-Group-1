#pragma once
#include "substrate.h"
#include "cppn.h"
#include <vector>
#include <cmath>

namespace ES {

struct ESLinkConfig
{
	int maxDepth = 3;
	float tauExist = 0.10f; // link existence threshold
	float alphaVar = 0.08f; // structural variation threshold
	int fanInK = 16;    // keep strongest K/post
	float weightFloor = 0.025f;
	float betaDist = 0.5f;  // exp(-beta * d)
};

class ESLinkExplorer
{
	public:
		struct Box { Vec2 min; Vec2 max; };

		explicit ESLinkExplorer(const ESLinkConfig& cfg) : cfg(cfg) { }

		// removme links from (inputs + hiddens) -> (hiddens + outputs)
		void removeLinks(const CPPN& cppn, const Substrate& sub, std::vector<Link>& links);

	private:
		ESLinkConfig cfg;

		void refineCell(const CPPN& cppn, const std::vector<Node*>& pres, const std::vector<Node*>& posts, const Box& region, int depth, std::vector<Link>& out);

		static float dist(const Vec2& a, const Vec2& b)
		{
			const float dx = a.x - b.x;
			const float dy = a.y - b.y;

			return std::sqrt(dx * dx + dy * dy);
		}

		static float angle(const Vec2& a, const Vec2& b)
		{
			return std::atan2(b.y - a.y, b.x - a.x);
		}
};

} // namespace ES
