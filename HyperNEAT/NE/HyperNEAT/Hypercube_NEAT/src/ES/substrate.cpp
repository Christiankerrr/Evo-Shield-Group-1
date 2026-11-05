#include "substrate.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <cctype>

namespace ES {

std::vector<Vec2> loadCoordsCSV(const std::string& path)
{
	std::vector<Vec2> out;

	std::ifstream in(path);
	if (!in) return out;

	std::string line;
	bool first = true;

	while (std::getline(in, line))
	{
		bool hasDigit = false;
		for (char c : line) if (std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '.') { hasDigit = true; break; }
		if (!hasDigit) continue;

		if (first)
		{
			first = false;
			std::stringstream ts(line);
			std::string a, b;
			if (std::getline(ts, a, ',') && std::getline(ts, b, ','))
			{
				try 
				{
					(void)std::stof(a);
					(void)std::stof(b);
				}
				catch (...)
				{
					continue;
				} 
			}
			else { continue; }
		}

		std::stringstream ss(line);
		std::string sx, sy;
		if (!std::getline(ss, sx, ',')) continue;
		if (!std::getline(ss, sy, ',')) continue;

		try
		{
			Vec2 v;
			v.x = std::stof(sx);
			v.y = std::stof(sy);
			out.push_back(v);
		}
		catch (...) { /* skip bad row */ }
	}

	return out;
}

void Substrate::normalizeToUnitSquare()
{
	if (inputs.empty() && outputs.empty() && hiddens.empty()) return;

	float minx = std::numeric_limits<float>::infinity();
	float miny = std::numeric_limits<float>::infinity();
	float maxx = -std::numeric_limits<float>::infinity();
	float maxy = -std::numeric_limits<float>::infinity();

	// scan all nodes to get bounds
	for (const auto& n : inputs)
	{
		minx = std::min(minx, n.pos.x);
		miny = std::min(miny, n.pos.y);
		maxx = std::max(maxx, n.pos.x);
		maxy = std::max(maxy, n.pos.y);
	}
	for (const auto& n : outputs)
	{
		minx = std::min(minx, n.pos.x);
		miny = std::min(miny, n.pos.y);
		maxx = std::max(maxx, n.pos.x);
		maxy = std::max(maxy, n.pos.y);
	}
	for (const auto& n : hiddens)
	{
		minx = std::min(minx, n.pos.x);
		miny = std::min(miny, n.pos.y);
		maxx = std::max(maxx, n.pos.x);
		maxy = std::max(maxy, n.pos.y);
	}

	// avoid divide by zero
	if (maxx == minx) maxx = minx + 1.f;
	if (maxy == miny) maxy = miny + 1.f;

	const float cx = 0.5f * (minx + maxx);
	const float cy = 0.5f * (miny + maxy);
	const float sx = 2.f / (maxx - minx);
	const float sy = 2.f / (maxy - miny);
	const float s = std::min(sx, sy);

	// center to (0,0) and scale so everything fits in [-1,1]
	for (auto& n : inputs) { n.pos.x = (n.pos.x - cx) * s; n.pos.y = (n.pos.y - cy) * s; }
	for (auto& n : outputs) { n.pos.x = (n.pos.x - cx) * s; n.pos.y = (n.pos.y - cy) * s; }
	for (auto& n : hiddens) { n.pos.x = (n.pos.x - cx) * s; n.pos.y = (n.pos.y - cy) * s; }
}

} // namespace ES
