#pragma once
#include <vector>
#include <string>

namespace ES {

struct Vec2
{
	float x;
	float y;
};

struct Node
{
	int ID;
	float bias = 0.f;
	bool isInput = false;
	bool isOutput = false;
	bool isHidden = false;
	Vec2 pos;
};

struct Link
{
	int preID;
	int postID;
	float w;
};

struct Substrate
{
	std::vector<Node> inputs, outputs, hiddens;

	// Maps to [-1,1]
	void normalizeToUnitSquare();
};

// UMAP CSV -> Vec2
std::vector<Vec2> loadCoordsCSV(const std::string& path);

} // namespace ES
