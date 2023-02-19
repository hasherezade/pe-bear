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
#include "../win_types.h"

#define OP_NOP 0x90


namespace sig_ma {
//------------------
enum sig_wildc {
	WILD_NONE = 0,
	WILD_ONE = '?'
};

enum sig_type {
    ROOT = 0,
    IMM,
    WILDC,
    MATCH
};

bool inline is_hex(char c) {
	if ((c >= '0' && c <='9') || (c >='a' && c <='f') || (c >='A' && c <='F')) return true;
	return false;
};

class SigNode
{
public:
	struct sig_compare {
		bool operator()(const SigNode* el1, const SigNode* el2 ) const;
	};

	SigNode(uint8_t val, sig_type type = IMM);
	~SigNode();

	SigNode* getWildc() const;
	SigNode* getChild(uint8_t val) const;

	SigNode* putChild(uint8_t val);
	SigNode* putWildcard(uint8_t val);

	bool operator==(const SigNode &el) const { return el.v == this->v; }
	bool operator!=(const SigNode &el) const { return el.v != this->v; }
	bool operator<(const SigNode &el) const { return el.v < this->v; }

private:
	uint8_t v;
	sig_type type;
	std::set<SigNode*, sig_compare> childs;
	std::set<SigNode*, sig_compare> wildcards;

friend class SigTree;
};

}; /* namespace sig_ma */
