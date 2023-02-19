/* 
* Copyright (c) 2013 hasherezade
*/

#include "SigNode.h"
using namespace sig_ma;

//--------------------------------------

bool SigNode::sig_compare::operator() (const SigNode* el1, const SigNode* el2 ) const
{
	return (*el1) < (*el2); 
}

SigNode::SigNode(uint8_t val, sig_type type)
{
	this->v =val;
	this->type = type;
}

SigNode::~SigNode()
{
	std::set<SigNode*, sig_compare>::iterator itr;
	
	for (itr = childs.begin(); itr != childs.end();) {
		SigNode* node = *itr;
		++itr;
		delete node;
	}

	for (itr = wildcards.begin(); itr != wildcards.end(); ) {
		SigNode* node = *itr;
		++itr;
		delete node;
	}
}

SigNode* SigNode::getWildc() const
{
	/* TODO: value masking */
	SigNode srchd(WILD_ONE, WILDC);
	std::set<SigNode*, sig_compare>::iterator found = wildcards.find(&srchd);
	if (found == wildcards.end()) return NULL;
	return (*found);
}

SigNode* SigNode::getChild(uint8_t val) const
{
	SigNode srchd(val, IMM);
	std::set<SigNode*, sig_compare>::iterator found = childs.find(&srchd);
	if (found == childs.end()) return NULL;
	return (*found);
}

//-----------------------------------------

SigNode* SigNode::putChild(uint8_t val)
{
	SigNode* f_node = NULL;
	SigNode* srchd = new SigNode(val, IMM);
	
	std::set<SigNode*, sig_compare>::iterator found = childs.find(srchd);
	if (found == childs.end()) {
		f_node = srchd;
		childs.insert(f_node);
		//if (DBG_LVL > 1) printf("[+] %02X %c\n", val, val);

	} else {
		f_node = *found;
		delete srchd; //already exists, no need for the new one
		//if (DBG_LVL > 1) printf("[#] %02X %c\n", val, val);
	}
	return f_node;
}

SigNode* SigNode::putWildcard(uint8_t val)
{
	SigNode* f_node = NULL;
	SigNode *srchd = new SigNode(val, WILDC);

	std::set<SigNode*, sig_compare>::iterator found = wildcards.find(srchd);
	if (found == wildcards.end()) {
		f_node = srchd;
		wildcards.insert(f_node);
		//if (DBG_LVL > 1) printf("[+W] %02X %c\n", val, val);
		
	} else {
		f_node = *found;
		delete srchd; //already exists, no need for the new one
		//if (DBG_LVL > 1) printf("[#W] %02X %c\n", val, val);

	}
	return f_node;
}
