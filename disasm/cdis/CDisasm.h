#pragma once

#include <vector>
#include <capstone/capstone.h>
#include <bearparser/bearparser.h>
#include "../Disasm.h"
#include "../MnemType.h"

namespace pe_bear {

//----------------------------------------------

class CDisasm : public Disasm
{
public:
	//const  static int MAX_ARG_NUM;

	CDisasm();
	~CDisasm();

	bool init(uint8_t* buf, size_t bufSize, size_t disasmSize, offset_t offset, Executable::exe_bits bitMode);
	bool fillTable();
	bool clearTable();
	offset_t getRawAt(int index) const;
	offset_t getArgVA(int index, int argNum, bool &isOk) const;
	
	size_t getChunkSize(int index) const
	{
		if ((size_t)index >= this->chunksCount()) {
			return 0;
		}
		const cs_insn m_insn = m_table.at(index);
		return m_insn.size;
	}
	
	bool isPushRet(int push_index,/*out*/ int* ret_index = NULL) const;
	
	//---

	size_t chunksCount() const
	{
		return _chunksCount();
	}

	QString mnemStr(size_t index) const
	{
		if (index >= this->_chunksCount()) {
			return "";
		}
		const cs_insn m_insn = m_table.at(index);
		const QString str = QString(m_insn.mnemonic) + " " + QString(m_insn.op_str);
		return str;
	}
	
	QString getHexStr(size_t index) const 
	{
		if (index >= this->_chunksCount()) {
			return "";
		}
		
		const cs_insn m_insn = m_table.at(index);
		return printBytes((uint8_t*) m_insn.bytes, m_insn.size);
	}
	
	bool isImmediate(size_t index) const
	{
		if (index >= this->_chunksCount()) {
			return false;
		}
		const cs_insn m_insn = m_table.at(index);
		const cs_detail *detail = &m_details.at(index);
		
		const size_t argNum = 0;
		const x86_op_type type = detail->x86.operands[argNum].type;
		if (type == X86_OP_IMM) {
			return true;
		}
		return false;
	}
	
	int32_t getImmediateVal(size_t index) const
	{
		if (!isImmediate(index)) {
			return 0;
		}
		if (index >= this->_chunksCount()) {
			return false;
		}
		const cs_insn m_insn = m_table.at(index);
		const cs_detail *detail = &m_details.at(index);
		
		const size_t argNum = 0;
		const x86_op_type type = detail->x86.operands[argNum].type;
		const x86_reg reg = static_cast<x86_reg>(detail->x86.operands[argNum].mem.base);
		int32_t val = static_cast<x86_reg>(detail->x86.operands[argNum].imm);
		return val;
	}
	
	minidis::mnem_type getMnemType(size_t index) const
	{
		using namespace minidis;
		if (index >= this->_chunksCount()) {
			return MT_NONE;
		}
		const cs_insn &m_insn =  m_table.at(index);
		return fetchMnemType(static_cast<x86_insn>(m_insn.id));
	}
	
	virtual bool isAddrOperand(int index) const;
	
	bool isFollowable(const int y) const;
	
	QString translateBranching(const int y) const;
protected:
	size_t _chunksCount() const { return this->m_table.size(); }
	bool isLongOp(const cs_insn &m_insn) const { return (m_insn.id == X86_INS_LCALL || m_insn.id == X86_INS_LJMP); }
	minidis::mnem_type fetchMnemType(const x86_insn cMnem) const;
	
	size_t disasmNext();
	bool init_capstone(Executable::exe_bits bitMode);
	
	//capstone stuff:
	std::vector<cs_insn> m_table;
	std::vector<cs_detail> m_details;
	
	csh handle;
	cs_insn* m_insn;

}; /* class Disasm */

}; /* namespace pe_bear */

