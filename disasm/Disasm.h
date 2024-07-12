#pragma once

#include <QtCore>
#include <vector>
#include <bearparser/bearparser.h>
#include "MnemType.h"

namespace pe_bear {

enum flag_val { FLAG_UNK = 0, FLAG_UNSET = (-1), FLAG_SET = 1, FLAG_REL, FLAG_RELNEG };

struct cond_buf {
	flag_val CF, PF, AF, ZF, SF, IF, DF, OF;
	int8_t cx;
	int8_t affectedCounter;
};

void resetCond(cond_buf &buf);

//----------------------------------------------

class Disasm
{
public:
	const  static int MAX_ARG_NUM;
	
	static QString printBytes(const uint8_t* buf, const size_t size);
	
	Disasm();
	~Disasm();

	virtual bool init(uint8_t* buf, size_t bufSize, size_t disasmSize, offset_t offset, Executable::exe_arch arch, Executable::exe_bits bitMode) = 0;
	virtual bool fillTable() = 0;
	virtual bool clearTable() = 0;
	virtual size_t getChunkSize(int index) const = 0;
	
	virtual bool isPushRet(int push_index,/*out*/ int* ret_index = NULL) const = 0;

	offset_t getJmpDestAddr(offset_t currVA, int instrLen, int lVal) const;
	//---

	virtual size_t chunksCount() const  = 0;

	bool isBranching(size_t index) const
	{
		if (index >= chunksCount()) return false;
		return isBranching(getMnemType(index));
	}
	
	bool isUnconditionalBranching(size_t index) const
	{
		if (index >= chunksCount()) return false;
		return isUnconditionalBranching(getMnemType(index));
	}
	
	virtual QString mnemStr(size_t index) const = 0;
	
	virtual QString getHexStr(size_t index) const = 0;
	virtual bool isImmediate(size_t index) const = 0;
	
	virtual int32_t getImmediateVal(size_t index) const = 0;
	virtual minidis::mnem_type getMnemType(size_t index) const = 0;

	virtual offset_t getRawAt(int index) const = 0;
	
	virtual offset_t getRvaAt(int index) const
	{
		return getRawAt(index);
	}
	
	virtual offset_t getVaAt(int index) const
	{
		return getRvaAt(index);
	}
	
	virtual offset_t convertToRVA(offset_t raw) const = 0;
	
	virtual offset_t convertToVA(offset_t raw) const = 0;

	/* returns target VA or INVALID_ADDR */
	virtual offset_t getTargetVA(int index, bool &isOk) const = 0;
	
	virtual bool isAddrOperand(int index) const = 0;
	
	/* returns target VA or INVALID_ADDR */
	virtual offset_t getArgVA(int index, int argNum, bool &isOk) const = 0;
	
	virtual QString translateBranching(const int index) const = 0;
	
	virtual bool isFollowable(const int y) const = 0;
	
protected:
	static offset_t trimToBitMode(int64_t value, uint8_t bits);
	static int64_t signExtend(int64_t operand, size_t opSize);

	static bool isBranching(minidis::mnem_type mType);
	static bool isUnconditionalBranching(minidis::mnem_type mType);
	
	virtual size_t disasmNext() = 0;

	bool is_init;

	Executable::exe_bits m_bitMode;
	Executable::exe_arch m_arch;

	uint8_t* m_buf;
	size_t m_bufSize;
	
	size_t m_disasmSize; // the part of buffer that will be used as a preview
	offset_t m_offset;
	offset_t startOffset;

	offset_t m_iptr; //instruction pointer
	QMutex m_disasmMutex;

}; /* class Disasm */

}; /* namespace pe_bear */

