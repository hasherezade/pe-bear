#include "pattern_tree.h"
#include <sstream>
#include <fstream>

#include <iostream>
#include <iomanip>
//--------------------------------------

#include <ctype.h>
namespace pattern_tree {

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
};

//--------------------------------------

using namespace pattern_tree;

#define WILD_CHAR '?'

namespace pattern_tree {
	bool inline is_hex(char c)
	{
		if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) return true;
		return false;
	};

	char inline hex_char_to_val(char c)
	{
		if (c >= '0' && c <= '9') {
			return c - '0';
		}
		if (c >= 'a' && c <= 'f') {
			return c - 'a' + 10;
		}
		if (c >= 'A' && c <= 'F') {
			return c - 'A' + 10;
		}
		return 0;
	};

	bool parseSigNode(const char chunk[3], BYTE& val, BYTE& vmask)
	{
		if (is_hex(chunk[0]) && is_hex(chunk[1])) {
			val = (hex_char_to_val(chunk[0]) << 4) | (hex_char_to_val(chunk[1]));
			vmask = 0xFF;
			return true;
		}
		if (chunk[0] == WILD_CHAR && chunk[1] == WILD_CHAR) {
			val = 0;
			vmask = 0;
			return true;
		}
		if (chunk[0] == WILD_CHAR || chunk[1] == WILD_CHAR) {
			if (chunk[1] == WILD_CHAR && is_hex(chunk[0])) {
				val = hex_char_to_val(chunk[0]) << 4;
				vmask = 0xF0;
				return true;
			}
			else if (chunk[0] == WILD_CHAR && is_hex(chunk[1])) {
				val = hex_char_to_val(chunk[1]);
				vmask = 0x0F;
				return true;
			}
		}
		std::cerr << "Invalid chunk supplied: " << std::hex << chunk[0] << " : " << chunk[1] << std::endl;
		return false;
	}
};

std::string Signature::toByteStr()
{
	std::stringstream ss;
	for (size_t i = 0; i < this->pattern_size; i++) {
		if (mask[i] == MASK_IMM) {
			ss << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(pattern[i]);
		}
		else if (mask[i] == MASK_PARTIAL1) {
			ss << WILD_CHAR;
			ss << std::setw(1) << std::setfill('0') << std::hex << (unsigned int)(pattern[i] & MASK_PARTIAL1);
		}
		else if (mask[i] == MASK_PARTIAL2) {
			ss << std::setw(1) << std::setfill('0') << std::hex << (unsigned int)((pattern[i] & MASK_PARTIAL2) >> 4);
			ss << WILD_CHAR;
		}
		else if (mask[i] == MASK_WILDCARD) {
			ss << WILD_CHAR << WILD_CHAR;
		}
		ss << " ";
	}
	return ss.str();
}
//---

Signature* Signature::loadFromByteStr(const std::string& signName, const std::string& content)
{
	if (!content.length()) return nullptr;

	const size_t buf_max = content.length() / 2;
	BYTE* pattern = (BYTE*)::calloc(buf_max, 1);
	BYTE* mask = (BYTE*)::calloc(buf_max, 1);
	if (!pattern || !mask) return nullptr;

	bool isOk = true;
	std::stringstream input(content);
	size_t indx = 0;
	// parse all the nodes one by one
	while(!input.eof() && indx < buf_max) {
		// parse all chunks from the line
		char chunk[3] = { 0, 0, 0 };
		input >> chunk[0];
		if (input.eof()) {
			break;
		}
		input >> chunk[1];
		if (!parseSigNode(chunk, pattern[indx], mask[indx])) {
			isOk = false;
			break;
		}
		indx++;
	}
	Signature* sign = nullptr;
	if (isOk) {
		sign = new Signature(signName, pattern, indx, mask);
	}
	free(pattern);
	free(mask);
	return sign;
}


size_t Signature::loadFromFile(std::ifstream& input, std::vector<Signature*> &signatures)
{
	if (!input.is_open()) return 0;

	while (!input.eof()) {
		std::string line;

		// read signature name
		if (!std::getline(input, line)) break;

		std::string signName = util::trim(line);

		// read signature size
		if (!std::getline(input, line)) break;
		int signSize = 0;
		std::stringstream iss1;
		iss1 << std::dec << line;
		iss1 >> signSize; //read the expected size
		if (signSize == 0) continue;

		const size_t buf_max = signSize;
		BYTE* pattern = (BYTE*)::calloc(buf_max, 1);
		BYTE* mask = (BYTE*)::calloc(buf_max, 1);
		if (!pattern || !mask) return false;

		bool isOk = false;
		size_t indx = 0;
		// parse all the nodes one by one
		while (!input.eof() && indx < buf_max) {

			// parse all chunks from the line
			char chunk[3] = { 0, 0, 0 };
			input >> chunk[0];
			if (input.eof()) {
				break;
			}
			input >> chunk[1];
			if (!parseSigNode(chunk, pattern[indx], mask[indx])) {
				isOk = false;
				break;
			}
		}
		// check if the signature is valid:
		if (indx != signSize) {
			isOk = false;
		}
		Signature* sign = nullptr;
		if (isOk) {
			sign = new Signature(signName, pattern, indx, mask);
			signatures.push_back(sign);
		}
		free(pattern);
		free(mask);
	}
	return signatures.size();
}
