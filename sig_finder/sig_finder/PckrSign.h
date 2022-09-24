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
	PckrSign(std::string name1)
		: name(name1)
	{
	}

	size_t length() const
	{
		return nodes.size();
	}

	std::string getName() const { return name; }

	bool addNode(unsigned char val, sig_type type);

	std::string getContent()
	{
		return signContent;
	}

protected:
	std::string name;
	std::vector<SigNode> nodes;
	std::string signContent;

friend class SigTree;
};

}; /* namespace sig_ma */
