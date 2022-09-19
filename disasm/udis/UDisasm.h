#pragma once

#include <vector>
#include <udis86.h>
#include <bearparser.h>
#include "../Disasm.h"
#include "../MnemType.h"

namespace pe_bear {

//----------------------------------------------

class UDisasm : public Disasm
{
public:
	const  static int MAX_ARG_NUM;

	UDisasm();
	~UDisasm();

	bool init(uint8_t* buf, size_t bufSize, size_t disasmSize, offset_t offset, Executable::exe_bits bitMode);
	bool fillTable();
	bool clearTable();
	size_t getChunkSize(int index) const;
	
	//get Raw offset of the instruction
	offset_t getRawAt(int index) const;
	
	offset_t getArgVA(int index, int argNum, bool &isOk) const;
	
	QString translateBranching(int index) const;
	
	bool isAddrOperand(int index) const;
	
	bool isPushRet(int push_index,/*out*/ int* ret_index = NULL) const;
	
	uint64_t getJmpDestAddr(uint64_t currVA, int instrLen, int lVal) const;

	int64_t getSignedLVal(size_t index, size_t operandNum, bool &isOk) const
	{
		if (index >= chunksCount()) return 0;
		ud_t udObj = m_table.at(index);
		return getSignedLVal(udObj, operandNum, isOk);
	}
	//---
    
	size_t chunksCount() const { return this->m_table.size(); }
    
	bool isBranching(size_t index) const
	{
		if (index >= chunksCount()) return false;
		return isBranching(m_table.at(index));
	}
	
	bool isUnconditionalBranching(size_t index) const
	{
		if (index >= chunksCount()) return false;
		return isUnconditionalBranching(m_table.at(index));
	}
	
	QString mnemStr(size_t index) const
	{
		if (index >= this->chunksCount()) {
			return "";
		}
		ud_t udObj = m_table.at(index);
		const char* str = ud_insn_asm(&udObj);
		if (!str) return "";
		return str;
	}
	
	QString getHexStr(size_t index) const 
	{
		if (index >= this->chunksCount()) {
			return "";
		}
		ud_t ud_obj =  m_table.at(index);
		char *hex_str = ud_insn_hex(&ud_obj);
		if (!hex_str) return "";
		return hex_str;
	}
	
	bool isImmediate(size_t index) const
	{
		if (index >= this->chunksCount()) {
			return false;
		}
		const ud_t &ud_obj =  m_table.at(index);
		ud_type type = ud_obj.operand[0].type;
		if (type != UD_OP_IMM) {
			return false;
		}
		return true;
	}
	
	int32_t getImmediateVal(size_t index) const
	{
		if (!isImmediate(index)) {
			return 0;
		}
		const ud_t &ud_obj =  m_table.at(index);
		int32_t val = ud_obj.operand[0].lval.sdword;
		return val;
	}
	
	minidis::mnem_type getMnemType(size_t index) const;
	
	bool isFollowable(const int y) const;
	
protected:
	static bool isBranching(ud_t ud_obj);
	static bool isUnconditionalBranching(ud_t ud_obj);
	
	int64_t getSignedLVal(ud_t &obj, size_t operandNum, bool &isOk) const; /* TODO: test it!!! */

	size_t disasmNext();
	
	ud_t ud_obj;
	std::vector<ud_t> m_table;

}; /* class Disasm */

}; /* namespace pe_bear */

