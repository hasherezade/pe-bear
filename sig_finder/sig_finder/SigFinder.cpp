#include "SigFinder.h"

#include <iostream>
#include <fstream>

using namespace sig_ma;

//----------------------------------------------------

/* read file with signatures */
size_t SigFinder::loadSignatures(const std::string &fname)
{
	std::ifstream input;
	input.open(fname);
	if (!input.is_open()) {
		//std::cout << "File not found: " << fname << std::endl;
		return 0;
	}
	const size_t num = tree.loadFromFile(input);
	input.close();
	return num;
}


matched SigFinder::getMatching(const uint8_t *buf, long buf_size, long start_offset, match_direction md)
{
	long srch_size = buf_size - start_offset;
	const uint8_t* srch_bgn = buf + start_offset;
	size_t min_sig_len = tree.getMinLen();
	
	matched matched;
	matched.match_offset = 0;

	bool skipNOPs = (md == FIXED)? true : false;

	if (md == FIXED) {
		matched = tree.getMatching(srch_bgn, srch_size, skipNOPs);
		matched.match_offset += srch_bgn - (buf + start_offset);
		return matched; 
	}

	if (md == FRONT_TO_BACK) {
		while (srch_size > min_sig_len) {
			matched = tree.getMatching(srch_bgn, srch_size, skipNOPs);

			if (matched.signs.size() > 0) {
				matched.match_offset += srch_bgn - (buf + start_offset);
				return matched;
			}
			srch_size--;
			srch_bgn++;
		}

	} else if (md == BACK_TO_FRONT) {
		while (srch_size > min_sig_len && srch_size <= buf_size) {
			matched = tree.getMatching(srch_bgn, srch_size, skipNOPs);
			if (matched.signs.size() > 0) {
				matched.match_offset += (buf + start_offset) - srch_bgn;
				return matched;
			}
			srch_size++;
			srch_bgn--;
		}
	}
	return matched; /* empty set */
}
