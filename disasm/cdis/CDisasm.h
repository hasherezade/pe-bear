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

	CDisasm();
	~CDisasm();

	bool init(uint8_t* buf, size_t bufSize, size_t disasmSize, offset_t offset, Executable::exe_arch arch, Executable::exe_bits bitMode);
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
		if (this->m_arch == Executable::ARCH_INTEL) {
			if (detail->x86.operands[argNum].type == X86_OP_IMM) {
				return true;
			}
		}
		if (this->m_arch == Executable::ARCH_ARM && this->m_bitMode == 64) {
			if (detail->arm64.operands[argNum].type == ARM64_OP_IMM) {
				return true;
			}
		}
		return false;
	}
	
	int64_t getImmediateVal(size_t index) const
	{
		if (!isImmediate(index)) {
			return 0;
		}
		if (index >= this->_chunksCount()) {
			return 0;
		}
		const cs_insn m_insn = m_table.at(index);
		const cs_detail *detail = &m_details.at(index);

		const size_t argNum = 0;

		int64_t val =  0;
		if (this->m_arch == Executable::ARCH_INTEL) {
			val = static_cast<x86_reg>(detail->x86.operands[argNum].imm);
		}
		if (this->m_arch == Executable::ARCH_ARM && this->m_bitMode == 64) {
			val = static_cast<x86_reg>(detail->arm64.operands[argNum].imm);
		}
		return val;
	}
	
	minidis::mnem_type getMnemType(size_t index) const
	{
		using namespace minidis;
		if (index >= this->_chunksCount()) {
			return MT_NONE;
		}
		const cs_insn &insn =  m_table.at(index);
		const cs_detail &detail =  m_details.at(index);
		return fetchMnemType(insn, detail);
	}
	
	virtual bool isAddrOperand(int index) const;
	
	bool isFollowable(const int y) const;
	
	int getMaxArgNum() const
	{
		if (this->m_arch == Executable::ARCH_INTEL) {
			return 2;
		}
		if (this->m_arch == Executable::ARCH_ARM) {
			return 3;
		}
		return 2;
	}

protected:
	size_t _chunksCount() const { return this->m_table.size(); }

	offset_t getArgVA_Intel(int index, int argNum, bool &isOk, const cs_insn &insn, const cs_detail &detail) const;
	offset_t getArgVA_Arm64(int index, int argNum, bool &isOk, const cs_insn &insn, const cs_detail &detail) const;
	
	bool isLongOp(const cs_insn &m_insn) const 
	{
		if (this->m_arch == Executable::ARCH_INTEL) {
			return isLongOp_Intel(m_insn);
		}
		return false;
	}
	
	bool isLongOp_Intel(const cs_insn &m_insn) const 
	{
		return (m_insn.id == X86_INS_LCALL || m_insn.id == X86_INS_LJMP);
	}
	
	minidis::mnem_type fetchMnemType(const cs_insn &insn, const cs_detail &detail) const {
		if (this->m_arch == Executable::ARCH_INTEL) {
			return fetchMnemType_Intel(insn);
		}
		if (this->m_arch == Executable::ARCH_ARM && this->m_bitMode == 64) {
			return fetchMnemType_Arm64(insn, detail);
		}
		return minidis::MT_OTHER;
	}
	
	minidis::mnem_type fetchMnemType_Intel(const cs_insn &insn) const;
	minidis::mnem_type fetchMnemType_Arm64(const cs_insn &insn, const cs_detail &detail) const;

	size_t disasmNext();
	bool init_capstone(Executable::exe_arch arch, Executable::exe_bits bitMode);
	
	//capstone stuff:
	std::vector<cs_insn> m_table;
	std::vector<cs_detail> m_details;
	
	csh handle;
	cs_insn* m_insn;

}; /* class Disasm */

}; /* namespace pe_bear */

