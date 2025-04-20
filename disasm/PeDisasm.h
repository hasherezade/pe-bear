#pragma once

#include "Disasm.h"

#ifdef BUILD_WITH_UDIS86
#include "udis/UDisasm.h"
typedef pe_bear::UDisasm __disasm_super; 
#else
#include "cdis/CDisasm.h"
typedef pe_bear::CDisasm __disasm_super; 
#endif

#define DISASM_PREVIEW_SIZE 0x200

namespace pe_bear {

class PeDisasm : public __disasm_super
{
	
public:
	PeDisasm(PEFile *pe, size_t previewSize = DISASM_PREVIEW_SIZE);
	bool init(const offset_t offset, Executable::exe_arch arch, Executable::exe_bits bitMode);
	bool fillTable();

	bool isRvaContnuous(int index) const;

	offset_t getRawAt(int index) const;
	offset_t getRvaAt(int index) const;
	
	offset_t getVaAt(int index) const
	{
		offset_t myRva = getRvaAt(index);
		if (myRva == INVALID_ADDR || !m_PE){
			return INVALID_ADDR;
		}
		return m_PE->rvaToVa(myRva);
	}

	/* returns target VA or INVALID_ADDR */
	offset_t getTargetVA(int index, bool &isOk) const;
	
	/* wrapper over getTargetVA,
	returns target RVA (if the target address belongs to the current module) or INVALID_ADDR 
	* */
	offset_t getTargetRVA(int index, bool &isOk) const;
	
	/* wrapper over getArgVA,
	returns target RVA (if the target address belongs to the current module) or INVALID_ADDR 
	* */
	offset_t getArgRVA(int index, int argNum, bool &isOk) const;

	/* returns target Raw or INVALID_ADDR */
	offset_t getTargetRaw(int index, bool &isOk) const;

	/* distance between current RVA and Target RVA */
	int32_t getTargetDelta(int index) const;

	bool isCallToRet(int index) const;

	QString getStringAt(offset_t rva) const;
	
	offset_t convertToRVA(offset_t raw) const
	{
		offset_t myRVA = INVALID_ADDR;
		try {
			myRVA = m_PE->convertAddr(raw, Executable::RAW, Executable::RVA);
		} catch (const CustomException&) {
			myRVA = raw;
		}
		return myRVA;
	}
	
	virtual offset_t convertToVA(offset_t raw) const
	{
		offset_t myVA = INVALID_ADDR;
		try {
			myVA = m_PE->convertAddr(raw, Executable::RAW, Executable::VA);
		} catch (const CustomException&) {
			myVA = raw;
		}
		return myVA;
	}
	
protected:
	bool fillOffsetTable();

	std::vector<offset_t> offsetTable;
	PEFile *m_PE;
	offset_t firstOffset;
	size_t previewSize;
	
	bool isBitModeAuto;
	bool isInit;

}; /* class PeDisasm */

}; /* namespace pe_bear */

