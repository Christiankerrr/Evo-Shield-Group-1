#pragma once
#include "substrate.h"
#include "cppn.h"

namespace ES {

struct AABB
{
	Vec2 min;
	Vec2 max;
};

struct ESNodeConfig
{
	int maxDepth = 3;
	float tauExist = 0.15f; // keep node if gate exceeds somewhere in cell
	float alphaVar = 0.10f; // variation threshold (structure present)
};

class ESNodeExplorer
{
	public:
		explicit ESNodeExplorer(const ESNodeConfig& cfg) : cfg(cfg) {};

		// Discovers hidden nodes over a continuous 2D field [-1,1]^2
		void discoverHiddenNodes(const CPPN& cppn, Substrate& sub);

	private:
		ESNodeConfig cfg;

		void refineCell(const CPPN& cppn, Substrate& sub, const AABB& box, int depth);
};

} // namespace ES
