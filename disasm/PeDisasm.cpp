#include "PeDisasm.h"
#include <QtCore>


using namespace pe_bear;

PeDisasm::PeDisasm(PEFile *pe, size_t _previewSize)
	: __disasm_super(),
	m_PE(pe), previewSize(_previewSize), 
	firstOffset(0), isInit(false), isBitModeAuto(true)
{
	if (pe == NULL) throw CustomException("PE not initialized!");
}

bool PeDisasm::init(const offset_t offset, Executable::exe_arch arch, Executable::exe_bits bitMode)
{
	this->isInit = false;
	clearTable();
	if (!m_PE) {
		return false;
	}
	this->firstOffset = offset;
	if (bitMode == Executable::UNKNOWN) {
		this->isBitModeAuto = true;
		this->m_bitMode = m_PE->getBitMode();
	} else {
		this->m_bitMode = bitMode;
	}
	if (arch == Executable::ARCH_UNKNOWN) {
		//this->isBitModeAuto = true;
		this->m_arch = m_PE->getArch();
	} else {
		this->m_arch = arch;
	}
	if (m_PE->getContentSize() < offset) {
		return false;
	}
	size_t maxSize = m_PE->getContentSize() - offset;
	size_t disasmSize = (maxSize > previewSize) ? previewSize : maxSize;
	
	uint8_t *buf = m_PE->getContentAt(offset, disasmSize);
	if (!buf) {
		return false;
	}
	const bool isOk = __disasm_super::init(buf, maxSize, disasmSize, offset, m_arch, m_bitMode);
	this->isInit = isOk;
	return isOk;
}

bool PeDisasm::fillTable()
{
	clearTable();
	if (!this->isInit) {
		return false;
	}
	bool isOk = __disasm_super::fillTable();
	if (isOk) {
		isOk = fillOffsetTable();
	}
	return isOk;
}

bool PeDisasm::fillOffsetTable() {

	this->offsetTable.clear();
	size_t count = this->m_table.size();
	if (count == 0) return false;

	for (size_t i = 0; i < count; i++) {
		offset_t offset = getRawAt(i);
		if (offset == INVALID_ADDR) {
			this->offsetTable.push_back(INVALID_ADDR);
			continue;
		}

		try {
			offset_t rva = m_PE->rawToRva(offset);
			this->offsetTable.push_back(rva);
		} catch (CustomException e) {
			this->offsetTable.push_back(INVALID_ADDR);
		}
	}
	return true;
}

bool PeDisasm::isRvaContnuous(int index) const
{
	if (index == 0) return true;
	if ((size_t)index >= m_table.size()) return true;

	offset_t prevRva = this->getRvaAt(index-1);
	size_t chunkSize = getChunkSize(index-1);
	offset_t calcRva = prevRva + chunkSize;

	offset_t currRva = this->getRvaAt(index);

	if (currRva != calcRva) {
		return false;
	}
	return true;
}

offset_t PeDisasm::getRawAt(int index) const
{
	const offset_t instrOffset = __disasm_super::getRawAt(index);
	if (instrOffset == INVALID_ADDR) {
		return INVALID_ADDR;
	}
	offset_t offset = firstOffset + instrOffset;
	return offset;
}

offset_t PeDisasm::getRvaAt(int index) const
{
	if ((size_t)index >= this->offsetTable.size()) return INVALID_ADDR;
	return this->offsetTable[index];
}

int32_t PeDisasm::getTargetDelta(int index) const
{
	if (!m_PE) return 0;

	bool isOk = false;
	offset_t rva = getTargetRVA(index, isOk);
	if (!isOk) return 0;

	offset_t currRVA = getRvaAt(index);
	return (int32_t)(rva - currRVA);
}

offset_t PeDisasm::getArgRVA(int index, int argNum, bool &isOk) const
{
	offset_t targetVA = PeDisasm::getArgVA(index,  argNum, isOk);
	if (!isOk) {
		return INVALID_ADDR;
	}
	// only if the target address belongs to the current executable, convert it to RVA:
	if (m_PE->isValidVA(targetVA)) {
		return m_PE->VaToRva(targetVA);
	}
	return INVALID_ADDR;
}

offset_t PeDisasm::getTargetVA(int index, bool &isOk) const
{
	offset_t targetAddr = INVALID_ADDR;
	for (int i = 0; i <= MAX_ARG_NUM; i++ ) {
		targetAddr = getArgVA(index, i, isOk);
		if (targetAddr != INVALID_ADDR) break;
	}
	return targetAddr;
}

offset_t PeDisasm::getTargetRVA(int index, bool &isOk) const
{
	offset_t targetAddr = getTargetVA(index, isOk);
	if (!isOk || !m_PE) {
		return INVALID_ADDR;
	}
	// only if the target address belongs to the current executable, convert it to RVA:
	if (!m_PE->isValidVA(targetAddr)) {
		return INVALID_ADDR;
	}
	return m_PE->VaToRva(targetAddr);
}

offset_t PeDisasm::getTargetRaw(int index, bool &isOk) const
{
	isOk = false;

	if (!m_PE) return INVALID_ADDR;

	bool ok = false;
	offset_t rva = getTargetRVA(index, ok);
	if (!ok) return INVALID_ADDR; //failed to get RVA

	offset_t raw = m_PE->toRaw(rva, Executable::RVA);

	if (raw != INVALID_ADDR) {
		isOk = true;
	}
	return raw;
}

QString PeDisasm::getStringAt(offset_t rva) const
{
	if (!m_PE) return "";

	offset_t raw_target = INVALID_ADDR;
	bool isValid = true;
	try {
		raw_target = m_PE->rvaToRaw(rva);
	} catch (CustomException e) {
		isValid = false;
	}
	if (!isValid) return "";

	QString str = m_PE->getStringValue(raw_target, 150);
	if (str.size() == 1) {
		str = m_PE->getWAsciiStringValue(raw_target, 150);
		if (str.trimmed().length() > 1) {
			return "L\'" + str + "\'";
		}
	}
	if (str.trimmed().length() == 0) return "";
	return "\'" + str + "\'";
}

bool PeDisasm::isCallToRet(int index) const
{
	using namespace minidis;
	
	if (!m_PE) return false;

	const mnem_type mnem = this->getMnemType(index);
	if (mnem != MT_CALL) return false;
	//is pointer to RET?
	static const BYTE OP_RET = 0xc3;

	bool isOk = false;
	uint64_t raw = this->getTargetRaw(index, isOk);
	if (raw == INVALID_ADDR || !isOk) return false;

	BYTE *cntnt = m_PE->getContent();
	if (cntnt[raw] == OP_RET) {
		return true;
	}
	return false;
}

//-------------------------
