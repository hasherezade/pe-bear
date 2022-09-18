/*
 * Copyright (c) 2013 hasherezade
*/

#pragma once

#include <stdio.h>
#include <set>
#include <vector>
#include <string>
#include <string.h>
#include <stdlib.h>

#include "SigNode.h"

namespace sig_ma {
//------------------

class PckrSign
{
public:
	PckrSign(std::string name1) { name = name1; }
	size_t length() const { return nodes.size() > 0 ? nodes.size() : name.size(); }
	std::string get_name() const { return name; }

protected:
	std::string name;
	std::vector<SigNode> nodes;

friend class SigTree;
};

}; /* namespace sig_ma */
