/* 
* Copyright (c) 2013 hasherezade
*/

#include "SigTree.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

//--------------------------------------
/* Util*/
#include <ctype.h>

namespace util {

    std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
    {
        str.erase(0, str.find_first_not_of(chars));
        return str;
    }

    std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
    {
        str.erase(str.find_last_not_of(chars) + 1);
        return str;
    }

    std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
    {
        return ltrim(rtrim(str, chars), chars);
    }
}

//--------------------------------------

using namespace sig_ma;

void SigTree::clear()
{
	nodeToSign.clear();

	std::set<PckrSign*>::iterator sigItr;
	for (sigItr = this->all_signatures.begin(); sigItr != this->all_signatures.end(); ++sigItr ) {
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

	size_t len = sign->length();
	if (this->min_siglen == 0 || this->min_siglen > len) 
		this->min_siglen = len;
	
	if (this->max_siglen == 0 || this->max_siglen < len) 
		this->max_siglen = len;
}


matched SigTree::getMatching(const uint8_t *buf, const size_t buf_len, bool skipNOPs)
{
	matched matchedSet;
	matchedSet.match_offset = 0;

	if (buf == NULL) return matchedSet; /* Empty */

	std::vector<SigNode*> level;
	level.push_back(&root);

	long checked = 0;
	long skipped = 0;
	for (size_t indx = 0; indx < buf_len; indx++) {
		const uint8_t bufChar = buf[indx];

		std::vector<SigNode*> level2;

		for (std::vector<SigNode*>::const_iterator lvlI = level.begin(); lvlI != level.end(); ++lvlI) {
			const SigNode* currNode = (*lvlI);
			if (!currNode) continue;
			
			// allow for alternate sig search paths: with wildcards AND with exact matches
			SigNode *nextC = currNode->getChild(bufChar);
			if (nextC) {
				PckrSign *sig = this->nodeToSign[nextC];
				if (sig) {
					matchedSet.signs.insert(sig);
				}
				level2.push_back(nextC);
			}
			
			nextC = currNode->getWildc();
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

size_t SigTree::loadFromFile(std::ifstream& input)
{
	if (!input.is_open()) return 0;

	sig_type type = ROOT;
	size_t loadedNum = 0;

	while (!input.eof()) {
		std::string line;

		// read signature name
		if (!std::getline(input, line)) break;
		std::string name = util::trim(line);

		// read signature size
		if (!std::getline(input, line)) break;
		int signSize = 0;
		std::stringstream iss1;
		iss1 << std::dec << line;
		iss1 >> signSize; //read the expected size
		if (signSize == 0) continue;

		PckrSign *sign = new PckrSign(name); // <- new signature created

		bool isFin = false;
		// read signature chunks:
		while ( !input.eof() && (sign->length() < signSize) ){

			// parse all chunks from the line
			char chunk[3] = { 0, 0, 0 };
			input >> chunk[0];
			input >> chunk[1];

			unsigned int val = 0;
			if (is_hex(chunk[0]) && is_hex(chunk[1])) {
				type = IMM;
				sscanf(chunk, "%2X", &val);
			}
			else if (chunk[0] == WILD_ONE) {
				type = WILDC;
				val = chunk[0];
			}
			else break;

			sign->addNode(val, type);
		}

		// check if the signature is valid:
		if (sign->length() == signSize) {
			if (this->addPckrSign(sign)) { // <- new signature stored
				loadedNum++;
				continue; // success
			}
		}
		//failure:
		delete sign;
		sign = NULL;
	}

	return loadedNum;
}
