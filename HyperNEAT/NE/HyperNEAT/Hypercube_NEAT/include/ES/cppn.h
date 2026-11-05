#pragma once
#include "substrate.h"
#include <cstddef>

namespace NEAT
{
	template<class Type> class FastNetwork;
}

namespace ES {

struct LinkOut
{
	float g{0.f};
	float w{0.f};
};

struct NodeOut
{
	float g{0.f};
	float b{0.f};
};

// Dual-head CPPN backed by an evolved NEAT network (one per individual).
// The explorers only call evalLink/evalNode, so they work unchanged.
//
// The NEAT network must have these nodes (see ESExperiment::createInitialPopulation):
//   inputs:  Bias, X1, Y1, X2, Y2, D, Angle
//   outputs: LinkGate, LinkWeight, NodeGate, NodeBias
class CPPN
{
	public:
		explicit CPPN(NEAT::FastNetwork<double>* network) : net(network) {}

		// link head inputs: pre(x,y), post(x,y), distance d, angle, sameComm, degPre, degPost, bias(1)
		LinkOut evalLink(float x1, float y1, float x2, float y2, float d,
			float angle, float sameComm, float degPre, float degPost, float bias) const;

		// node head inputs: (x,y), bias(1)
		NodeOut evalNode(float x, float y, float bias) const;

	private:
		NEAT::FastNetwork<double>* net; // not owned
};

} // namespace ES
