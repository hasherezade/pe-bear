#pragma once

#include <set>
#include "SigTree.h"

namespace sig_ma {
//------------------

class FoundPacker {

public:
	FoundPacker(uint64_t offs, sig_ma::PckrSign* sig) : offset(offs), signaturePtr(sig) { }

	uint64_t offset;
	sig_ma::PckrSign* signaturePtr; // do not delete it - belongs to SignFinder

	bool operator== (const FoundPacker& f2) { return (this->offset == f2.offset && this->signaturePtr == f2.signaturePtr);}
};

enum match_direction {
	FIXED,
	FRONT_TO_BACK,
	BACK_TO_FRONT
};


class SigFinder
{
public:
	SigFinder(void) {}
	~SigFinder(void){}

	matched getMatching(const uint8_t *buf, long buf_size, long start_offset, match_direction md = FIXED);

	size_t loadSignatures(const std::string &fname);

	std::set<PckrSign*>& signatures() { return tree.all_signatures; };
	std::vector<PckrSign*>& signaturesVec() { return tree.signaturesVec; }

protected:
	SigTree tree;
};
//---
}; //namespace sig_ma
//------------------