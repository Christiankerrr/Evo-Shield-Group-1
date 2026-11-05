#include "exportUtils.h"
#include <unordered_map>
#include <fstream>

namespace ES {

static void addMap(const std::vector<Node>& v, std::unordered_map<int, const Node*>& m)
{
	for (auto& n : v) m[n.ID] = &n;
}

void writeExpressedLinksCSV(const Substrate& sub, const std::vector<Link>& links, const std::string& path)
{
	std::unordered_map<int, const Node*> byId;
	byId.reserve(sub.inputs.size() + sub.hiddens.size() + sub.outputs.size());
	addMap(sub.inputs, byId);
	addMap(sub.hiddens, byId);
	addMap(sub.outputs, byId);

	std::ofstream fout(path);
	if (!fout.is_open()) return;

	fout << "preX,preY,postX,postY,weight\n";
	for (auto& L : links)
	{
		auto itPre = byId.find(L.preID);
		auto itPost = byId.find(L.postID);
		if (itPre == byId.end() || itPost == byId.end()) continue;

		const Vec2& p = itPre->second->pos;
		const Vec2& q = itPost->second->pos;
		fout << p.x << "," << p.y << "," << q.x << "," << q.y << "," << L.w << "\n";
	}
}

bool saveNodesCSV(const Substrate& sub, const std::string& path)
{
	std::ofstream out(path);
	if (!out) return false;

	out << "id,x,y,type\n";
	for (const auto& n : sub.inputs)  out << n.ID << "," << n.pos.x << "," << n.pos.y << ",input\n";
	for (const auto& n : sub.hiddens) out << n.ID << "," << n.pos.x << "," << n.pos.y << ",hidden\n";
	for (const auto& n : sub.outputs) out << n.ID << "," << n.pos.x << "," << n.pos.y << ",output\n";
	return true;
}

} // namespace ES
