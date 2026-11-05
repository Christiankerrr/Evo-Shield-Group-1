#include "HCUBE_Defines.h"
#include "cppn.h"
#include <cmath>

namespace ES {

// HyperNEAT runs with SignedActivation, so the network's outputs are in [-1,1].
// The explorers compare gates against thresholds in [0,1], so shift gates there.
static inline float toUnit(double v)
{
	return static_cast<float>(0.5 * (v + 1.0));
}

LinkOut CPPN::evalLink(float x1, float y1, float x2, float y2, float d, float angle, float sameComm, float degPre, float degPost, float bias) const
{
	// sameComm, degPre and degPost are still always 0 in the explorers, so they are
	// not CPPN inputs yet. Add genes for them once they carry graph information.
	(void)sameComm; (void)degPre; (void)degPost;

	net->reinitialize();
	net->setValue("X1", x1);
	net->setValue("Y1", y1);
	net->setValue("X2", x2);
	net->setValue("Y2", y2);
	net->setValue("D", d);
	net->setValue("Angle", angle / 3.14159265f); // atan2 range -> [-1,1]
	net->setValue("Bias", bias);
	net->update();

	LinkOut o;
	o.g = toUnit(net->getValue("LinkGate"));
	o.w = static_cast<float>(net->getValue("LinkWeight"));

	return o;
}

NodeOut CPPN::evalNode(float x, float y, float bias) const
{
	// Both heads share one network: a node query is a zero-length link from (x,y)
	// to itself, and the node head's outputs are read.
	net->reinitialize();
	net->setValue("X1", x);
	net->setValue("Y1", y);
	net->setValue("X2", x);
	net->setValue("Y2", y);
	net->setValue("D", 0.0);
	net->setValue("Angle", 0.0);
	net->setValue("Bias", bias);
	net->update();

	NodeOut o;
	o.g = toUnit(net->getValue("NodeGate"));
	o.b = static_cast<float>(net->getValue("NodeBias"));

	return o;
}

} // namespace ES
