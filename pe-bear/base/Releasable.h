#pragma once

#include <stdio.h>

//-----------------

class Releasable
{
public:
	Releasable() : refCntr(0) {}
	
	void release() { if (refCntr > 0) refCntr--; else delete this; }
	void incRefCntr() { this->refCntr++; }
	size_t getRefCntr() { return refCntr; } 
protected:
	virtual ~Releasable() {}
	size_t refCntr; // reference Counter
};


