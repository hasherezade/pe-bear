/* 
* Copyright (c) 2013 hasherezade
*/

#include "SigTree.h"

//--------------------------------------
/* Util*/
#include <ctype.h>

void inline trim(char *buf) {
	size_t len = 0;
	while ((len = strlen(buf)) > 0) {
		if (isspace(buf[len-1])) buf[len-1] = '\0';
		else break;
	}
}

//--------------------------------------

using namespace sig_ma;

void SigTree::clear()
{
	nodeToSign.clear();

	std::set<PckrSign*>::iterator sigItr;
	for (sigItr = this->all_signatures.begin(); sigItr != this->all_signatures.end(); sigItr++ ) {
		PckrSign* sign = (*sigItr);
		delete sign;
	}
	this->all_signatures.clear();
	signaturesVec.clear();
}


bool SigTree::addPckrSign(PckrSign *sign)
{
	if (!sign) return false;

	SigNode* currNode = &root;

	for (int indx = 0; indx < sign->nodes.size(); indx++) {
		char value = sign->nodes[indx].v;
		SigNode* nextNode = NULL;

		if (sign->nodes[indx].type == WILDC) {
			nextNode = currNode->putWildcard(value);
		} else {
			nextNode = currNode->putChild(value);
		}
		if (nextNode == NULL) {
			printf("Error in signature adding!\n");
			return false;
		}
		currNode = nextNode;
	}

	if (nodeToSign[currNode] == NULL) {
		nodeToSign[currNode] = sign;
		insertPckrSign(sign);
		return true;
	}
	return false;
}

void SigTree::insertPckrSign(PckrSign* sign)
{
	if (!sign) return;
	size_t signCount = all_signatures.size();
	all_signatures.insert(sign);

	if (all_signatures.size() > signCount) {
		signaturesVec.push_back(sign);
	}

	long len = sign->length();
	if (this->min_siglen == 0 || this->min_siglen > len) 
		this->min_siglen = len;
	
	if (this->max_siglen == 0 || this->max_siglen < len) 
		this->max_siglen = len;
}


matched SigTree::getMatching(char *buf, size_t buf_len, bool skipNOPs)
{
	matched matchedSet;
	matchedSet.match_offset = 0;

	if (buf == NULL) return matchedSet; /* Empty */

	std::vector<SigNode*> level;
	level.push_back(&root);

	long checked = 0;
	long skipped = 0;
	for (int indx = 0; indx < buf_len; indx++) {
		char b = buf[indx];

		std::vector<SigNode*> level2;
		std::vector<SigNode*>::iterator lvlI;
		
		for (lvlI = level.begin(); lvlI != level.end();lvlI++) {
			std::vector<SigNode*>::iterator curr = lvlI;

			SigNode *nextC = (*curr)->getChild(b);
			if (nextC == NULL) nextC = (*curr)->getWildc(b);

			if (nextC) {
				PckrSign *sig = this->nodeToSign[nextC];
				if (sig) {
					matchedSet.signs.insert(sig);
				}
				level2.push_back(nextC);
			}
		}
		//-----
		if (level2.size() == 0) {
			if (buf[indx] == OP_NOP && skipNOPs) { //skip NOPs
				if (checked == 0) {
					matchedSet.match_offset++;
				}
				skipped++;
				continue;
			}
			break;
		}
		checked++;
		level = level2;
	}
	return matchedSet;
}

long SigTree::loadFromFile(FILE* fp)
{
	if (!fp) return 0;

	//init buffers
	const size_t LINE_SIZE = 101;
	char line[LINE_SIZE];
	memset(line, 0, sizeof(line));

	const size_t NAME_SIZE = 101;
	char name[NAME_SIZE];

	size_t sigSize = 0, maskSize = 0;
	char *sign = NULL, *mask = NULL;
	sig_type type = ROOT;

	int signSize = 0;
	int loadedNum = 0;

	while (!feof(fp)) {
		memset(name, 0, NAME_SIZE);
		memset(line, 0, LINE_SIZE);
		
		if (!fgets(name, NAME_SIZE - 1, fp)) break;
		trim(name);

		signSize = 0;
		fscanf(fp, "%d", &signSize); //read the expected size
		if (signSize == 0) continue;

		PckrSign *sign = new PckrSign(name); // <- new signature created

		for (int i = 0; i < signSize; i++) {
			char chunk[3] = {0, 0, 0};
			if (feof(fp)) break;

			fscanf(fp, "%2s", chunk);
			unsigned int val = 0;

			if ( is_hex(chunk[0]) && is_hex(chunk[1]) ) {
				type = IMM;
				sscanf(chunk, "%X", &val);
			} else if (chunk[0] == WILD_ONE) {
				type = WILDC;
				val = chunk[0];
			} else break;

			sign->nodes.push_back(SigNode(val, type));
		}

		if (sign->nodes.size() == signSize) {
			if (this->addPckrSign(sign)) // <- new signature stored
				loadedNum++;
				continue; // success
		}

		//failure:
		delete sign;
		sign = NULL;
	}
	return loadedNum;
}
