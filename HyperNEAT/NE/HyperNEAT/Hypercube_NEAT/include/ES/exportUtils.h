#pragma once
#include "substrate.h"
#include <string>
#include <vector>

namespace ES {

void writeExpressedLinksCSV(const Substrate& sub, const std::vector<Link>& links, const std::string& path);
bool saveNodesCSV(const Substrate& sub, const std::string& path);

} // namespace ES
