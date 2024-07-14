#include "CDisasm.h"
#include <iostream>
using namespace pe_bear;

CDisasm::CDisasm()
	: Disasm(),
	m_insn(NULL)
{
}

CDisasm::~CDisasm()
{
	is_init = false;
}

cs_mode toCSmode(Executable::exe_bits bitMode)
{
	switch (bitMode) {
	case Executable::BITS_16:
		return CS_MODE_16;
	case Executable::BITS_32:
		return CS_MODE_32;
	case Executable::BITS_64:
		return CS_MODE_64;
	}
	return CS_MODE_32; //Default
}

bool CDisasm::init_capstone(Executable::exe_arch arch, Executable::exe_bits bitMode)
{
	cs_err err;
	if (arch == Executable::ARCH_INTEL) {
		err = cs_open(CS_ARCH_X86, toCSmode(bitMode), &handle);
	} else if (arch == Executable::ARCH_ARM && bitMode == Executable::BITS_64) {
		err = cs_open(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN, &handle);
	} else if (arch == Executable::ARCH_ARM && bitMode == Executable::BITS_32) {
		err = cs_open(CS_ARCH_ARM, CS_MODE_LITTLE_ENDIAN, &handle);
	} else {
		std::cout << "Unknown ARCH: " << std::hex << arch << "\n";
		return false;
	}

	if (err) {
		if (err == CS_ERR_ARCH) {
			std::cerr << "Failed on cs_open(): unsupported architecture supplied!\n";
		} else {
			std::cerr << "Failed on cs_open(), error: " << std::dec << err << std::endl;
		}
		return false;
	}
	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
	cs_option(handle, CS_OPT_SKIPDATA, CS_OPT_ON);
	m_insn = cs_malloc(handle);
	if (!m_insn) {
		cs_close(&handle);
		return false;
	}
	return true;
}

bool CDisasm::init(uint8_t* buf, size_t bufSize, size_t disasmSize, offset_t offset, Executable::exe_arch arch, Executable::exe_bits bitMode)
{
	QMutexLocker locker(&m_disasmMutex);
	is_init = false;
	if (!buf || bufSize == 0) return false;

	m_buf = buf;
	m_bufSize = bufSize;
	m_disasmSize = disasmSize;
	m_iptr = 0;

	this->m_offset = 0;
	this->startOffset = this->convertToVA(offset);
	this->m_RVA = (this->startOffset != INVALID_ADDR) ? this->startOffset : 0;
	m_bitMode = bitMode;
	m_arch = arch;

	is_init = init_capstone(m_arch, m_bitMode);
	return this->is_init;
}

size_t CDisasm::disasmNext()
{
	if (!is_init && m_insn) {
		printf("Cannot disasm next = not initialized!\n");
		return 0;
	}
	//--
	bool isOk = cs_disasm_iter(handle, (const unsigned char**)&m_buf, &m_bufSize, &m_RVA, m_insn);
	if (!isOk || !m_insn) {
		is_init = false;
		return 0;
	}
	//--
	const size_t step = m_insn->size;
	m_iptr += step;
	return step;
}

bool CDisasm::fillTable()
{
	QMutexLocker locker(&m_disasmMutex);

	if (!is_init) {
		return false;
	}
	this->clearTable();
	size_t processedSize = 0;
	while (processedSize < this->m_disasmSize) {
		if (!disasmNext()) {
			break; //could not disasemble more
		}
		if (!m_insn) continue;
		processedSize += m_insn->size;

		const cs_insn next_insn = *m_insn;
		const cs_detail *detail = m_insn->detail;
		m_table.push_back(next_insn);
		m_details.push_back(*detail);
	}
	if (m_table.size() == 0) {
		return false;
	}
	return true;
}

bool CDisasm::clearTable()
{
	m_table.clear();
	m_details.clear();
	return true;
}

offset_t CDisasm::getRawAt(int index) const
{
	if (index >= m_table.size()) {
		return INVALID_ADDR;
	}
	const cs_insn m_insn = m_table.at(index);
	if (startOffset == INVALID_ADDR) {
		return m_insn.address;
	}
	return m_insn.address - startOffset;
}

offset_t CDisasm::getArgVA_Intel(int index, int argNum, bool &isOk, const cs_insn &insn, const cs_detail &detail) const
{
	size_t cnt = static_cast<size_t>(detail.x86.op_count);
	if (argNum >= cnt) return INVALID_ADDR;
	
	offset_t va = INVALID_ADDR;
	const x86_op_type op_type = detail.x86.operands[argNum].type;

	if (op_type == X86_OP_MEM) {
		int64_t lval = detail.x86.operands[argNum].mem.disp;
		const x86_reg reg = static_cast<x86_reg>(detail.x86.operands[argNum].mem.base);
		const bool isEIPrelative = (reg == X86_REG_IP || reg == X86_REG_EIP || reg == X86_REG_RIP);
		if (isEIPrelative) {
			const offset_t currVA = getVaAt(index);
			const size_t instrLen = getChunkSize(index);
			va = Disasm::getJmpDestAddr(currVA, instrLen, lval);
			isOk = true;
		}
		else if (reg <= X86_REG_INVALID) { //simple case, no reg value to add
			va = Disasm::trimToBitMode(lval, this->m_bitMode);
			isOk = true;
		}
	}
	if (op_type == X86_OP_IMM) {
		va = detail.x86.operands[argNum].imm;
		isOk = true;
	}
	return va;
}

int64_t CDisasm::backtraceReg_Arm64(int startIndx, arm64_reg reg, bool& isOk) const
{
	using namespace minidis;
	isOk = false;
	
	const int maxWindow = 10;
	int64_t va = INVALID_ADDR;
	bool _isOk = false;
	int found = startIndx;
	for (int index = (startIndx - 1); index >= 0 && index >= (startIndx - maxWindow); index--) {
		const mnem_type mType = this->getMnemType(index);
		if (mType == MT_CALL || mType == MT_JUMP || mType == MT_COND_JUMP || mType == MT_RET || mType == MT_INVALID) {
			// basic block end
			break;
		}
		const cs_detail detail = m_details.at(index);
		size_t cnt = static_cast<size_t>(detail.arm64.op_count);
		if (cnt != 2) continue;

		const arm64_reg regI = static_cast<arm64_reg>(detail.arm64.operands[0].mem.base);
		if (regI != reg) continue;
		
		const cs_insn insn = m_table.at(index);
		if (insn.id == arm64_insn::ARM64_INS_ADRP) {
			va = detail.arm64.operands[1].imm; // the register is set to the immediate value
			_isOk = true;
			found = index;
			//std::cout << "Found value for the register: " << std::hex << va << "\n";
			break;
		}
	}
	if (!_isOk) return INVALID_ADDR;
	
	for (int index = found + 1; index < startIndx; index++) {
		bool affectsReg = false;
		
		const cs_detail detail = m_details.at(index);
		const size_t cnt = static_cast<size_t>(detail.arm64.op_count);
		
		if (cnt > 0 
			&& detail.arm64.operands[0].type == ARM64_OP_REG
			&& static_cast<arm64_reg>(detail.arm64.operands[0].mem.base) == reg
			)
		{
			affectsReg = true;
			_isOk = false;
		}
		
		if (!affectsReg) continue;
		
		const cs_insn insn = m_table.at(index);
		/*
		std::cout << "Checking: " << this->mnemStr(index).toStdString() << " Cnt: " << cnt << " { ";
		for (size_t c = 0; c < cnt; c++) {
			std::cout << " " <<  detail.arm64.operands[c].type;
		}
		std::cout << " }\n";
		*/
		if (insn.id == arm64_insn::ARM64_INS_LDR
			&& cnt == 2
			&& detail.arm64.operands[1].type == ARM64_OP_MEM
			&& detail.arm64.operands[1].mem.base == reg
			)
		{
			va += detail.arm64.operands[1].mem.disp;
			_isOk = true;
			//std::cout << "Added: " << std::hex << va << "\n";
		}
	}
	
	if (!_isOk) return INVALID_ADDR;
	isOk = true;
	return va;
}

offset_t CDisasm::getArgVA_Arm64(int index, int argNum, bool &isOk, const cs_insn &insn, const cs_detail &detail) const
{
	size_t cnt = static_cast<size_t>(detail.arm64.op_count);
	if (argNum >= cnt) return INVALID_ADDR;
	
	offset_t va = INVALID_ADDR;
	//immediate:
	if (detail.arm64.operands[argNum].type == ARM64_OP_IMM) {
		va = detail.arm64.operands[argNum].imm;
		isOk = true;
	}
	else if (argNum == 0 && detail.arm64.operands[argNum].type == ARM64_OP_REG) {
		const arm64_reg reg = static_cast<arm64_reg>(detail.arm64.operands[argNum].mem.base);
		va = backtraceReg_Arm64(index, reg, isOk);
	}
	return va;
}

offset_t CDisasm::getArgVA(int index, int argNum, bool &isOk) const
{
	isOk = false;
	if (index >= m_table.size()) {
		return INVALID_ADDR;
	}
	const cs_insn &insn = m_table.at(index);
	const cs_detail &detail = m_details.at(index);
	
	offset_t va = INVALID_ADDR;
	// Intel
	if (this->m_arch == Executable::ARCH_INTEL) {
		va = getArgVA_Intel(index, argNum, isOk, insn, detail);
	}
	//ARM:
	else if (this->m_arch == Executable::ARCH_ARM && this->m_bitMode == 64) {
		va = getArgVA_Arm64(index, argNum, isOk, insn, detail);
	}
	// cleanup
	if (isOk) {
		va = Disasm::trimToBitMode(va, this->m_bitMode);
	}
	return va;
}

minidis::mnem_type CDisasm::fetchMnemType_Intel(const cs_insn &insn) const
{
	using namespace minidis;

	const unsigned int cMnem = insn.id;
	if (cMnem == x86_insn::X86_INS_INVALID) {
		return MT_INVALID;
	}
	if (cMnem == x86_insn::X86_INS_JMP || cMnem == x86_insn::X86_INS_LJMP) {
		return MT_JUMP;
	}
	if (cMnem >= x86_insn::X86_INS_JAE && cMnem <= x86_insn::X86_INS_JS) {
		return MT_COND_JUMP;
	}
	if (cMnem >= x86_insn::X86_INS_MOV && cMnem <= x86_insn::X86_INS_MOVZX) {
		return MT_MOV;
	}

	switch (cMnem) {
	case X86_INS_LOOP:
	case X86_INS_LOOPE:
	case X86_INS_LOOPNE:
		return MT_LOOP;

	case X86_INS_CALL:
	case X86_INS_LCALL:
		return MT_CALL;

	case X86_INS_RET:
	case X86_INS_RETF:
	case X86_INS_RETFQ:
		return MT_RET;

	case X86_INS_NOP: return MT_NOP;

	case X86_INS_POP:
	case X86_INS_POPAW:
	case X86_INS_POPAL:
	case X86_INS_POPCNT:
	case X86_INS_POPF:
	case X86_INS_POPFD:
	case X86_INS_POPFQ:
	{
		return MT_POP;
	}
	case X86_INS_PUSH:
	case X86_INS_PUSHAW:
	case X86_INS_PUSHAL:
	case X86_INS_PUSHF:
	case X86_INS_PUSHFD:
	case X86_INS_PUSHFQ:
	{
		return MT_PUSH;
	}
	case X86_INS_INT3:
		return MT_INT3;

	case X86_INS_INT:
		return MT_INTX;
	}
	return MT_OTHER;
}

minidis::mnem_type CDisasm::fetchMnemType_Arm64(const cs_insn &insn, const cs_detail &detail) const
{
	using namespace minidis;

	const unsigned int cMnem = insn.id;
	if (cMnem == arm64_insn::ARM64_INS_UDF) {
		return MT_INT3;
	}
	if (cMnem == arm64_insn::ARM64_INS_INVALID) {
		return MT_INVALID;
	}
	if (cMnem == arm64_insn::ARM64_INS_NOP) {
		return MT_NOP;
	}
	if (cMnem == arm64_insn::ARM64_INS_ADRP 
		|| cMnem == arm64_insn::ARM64_INS_LDR
		|| cMnem == arm64_insn::ARM64_INS_MOV)
	{
		return MT_MOV;
	}
	for (size_t i = 0; i < detail.groups_count; i++) {
		if (detail.groups[i] == ARM64_GRP_CALL) return MT_CALL;
		if (detail.groups[i] == ARM64_GRP_RET) return MT_RET;
		if (detail.groups[i] == ARM64_GRP_INT)  return MT_INTX;
		
		if (detail.groups[i] == ARM64_GRP_JUMP || detail.groups[i] == ARM64_GRP_BRANCH_RELATIVE) {
			switch (cMnem) {
				case arm64_insn::ARM64_INS_CBZ:
				case arm64_insn::ARM64_INS_CBNZ:
				case arm64_insn::ARM64_INS_TBNZ:
				case arm64_insn::ARM64_INS_TBZ:
					return MT_COND_JUMP;
			}
			return MT_JUMP;
		}

	}
	return MT_OTHER;
}

bool CDisasm::isPushRet(int index, /*out*/ int* ret_index) const
{
	if (this->m_arch != Executable::ARCH_INTEL) {
		return false;
	}
	if (index >= this->_chunksCount()) {
		return false;
	}

	const cs_insn m_insn = m_table.at(index);
	const cs_detail detail = m_details.at(index);

	const minidis::mnem_type mnem = fetchMnemType(m_insn, detail);
	
	if (mnem == minidis::MT_PUSH) {
		int y2 = index + 1;
		if (y2 >= m_table.size()) {
			return false;
		}
		const cs_insn m_insn2 = m_table.at(y2);
		const minidis::mnem_type mnem2 = fetchMnemType(m_insn2, detail);
		if (mnem2 == minidis::MT_RET) {
			if (ret_index != NULL) {
				(*ret_index) = y2;
			}
			return true;
		}
	}
	return false;
}

bool CDisasm::isAddrOperand(int index) const
{
	if (index >= m_table.size()) {
		return false;
	}
	using namespace minidis;
	mnem_type mnem = this->getMnemType(index);
	if (mnem == MT_PUSH || mnem == MT_MOV) return true;

	const cs_detail &detail = m_details.at(index);
	
	// Intel
	if (this->m_arch == Executable::ARCH_INTEL) {
		const size_t cnt = static_cast<size_t>(detail.x86.op_count);

		for (int argNum = 0; argNum < cnt; argNum++) {
			const x86_op_type opType = detail.x86.operands[argNum].type;

			if (opType == X86_OP_IMM 
				&& detail.x86.operands[argNum].size > 8)
			{
				return true;
			}

			const x86_reg reg = static_cast<x86_reg>(detail.x86.operands[argNum].mem.base);
			const bool isEIPrelative = (reg == X86_REG_IP || reg == X86_REG_EIP || reg == X86_REG_RIP);

			if (opType == X86_OP_MEM && isEIPrelative) {
				return true;
			}
		}
	}
	// Arm64
	else if (this->m_arch == Executable::ARCH_ARM && this->m_bitMode == 64) {
		const size_t cnt = static_cast<size_t>(detail.arm64.op_count);

		for (int argNum = 0; argNum < cnt; argNum++) {
			if (detail.arm64.operands[argNum].type == ARM64_OP_IMM)
			{
				return true;
			}
		}
	}
	return false;
}

bool CDisasm::isFollowable(const int y) const
{
	if (y >= this->chunksCount()) return false;

	if (getRvaAt(y) == INVALID_ADDR) return false;

	if (!isBranching(y) && !isPushRet(y)) {
		return false;
	}
	const cs_detail *detail = &m_details.at(y);
	if (!detail) return false;

	const size_t argNum = 0;
	// Intel
	if (this->m_arch == Executable::ARCH_INTEL) {
		size_t cnt = static_cast<size_t>(detail->x86.op_count);
		if (!cnt) {
			return false;
		}
		const x86_op_type op_type = detail->x86.operands[argNum].type;
		if (op_type == X86_OP_IMM) {
			return true;
		}
		if (op_type == X86_OP_MEM || op_type == X86_OP_IMM) {
			const x86_reg reg = static_cast<x86_reg>(detail->x86.operands[argNum].mem.base);
			if (reg <= X86_REG_INVALID) { //simple case, no reg value to add
				return true;
			}
			const bool isEIPrelative = (reg == X86_REG_IP || reg == X86_REG_EIP || reg == X86_REG_RIP);
			if (isEIPrelative) {
				return true;
			}
		}
		return false;
	}
	// ARM
	else if (this->m_arch == Executable::ARCH_ARM && this->m_bitMode == 64) {
		size_t cnt = static_cast<size_t>(detail->arm64.op_count);
		if (!cnt) {
			return false;
		}
		if (detail->arm64.operands[argNum].type == ARM64_OP_IMM) {
			return true;
		}
		if (cnt == 1 && detail->arm64.operands[argNum].type == ARM64_OP_REG) {
			return true;
		}
	}
	return false;
}
