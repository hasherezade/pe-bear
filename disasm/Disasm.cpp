#include "Disasm.h"
#include <QtGlobal>

using namespace pe_bear;
using namespace minidis;

const int Disasm::MAX_ARG_NUM = 2;

void pe_bear::resetCond(cond_buf &buf)
{
	buf.CF = FLAG_UNK;
	buf.PF = FLAG_UNK;
	buf.AF = FLAG_UNK;
	buf.ZF = FLAG_UNK;
	buf.SF = FLAG_UNK;
	buf.IF = FLAG_UNK;
	buf.DF = FLAG_UNK;
	buf.OF = FLAG_UNK;
	buf.cx = FLAG_UNK;
	buf.affectedCounter = 0;
}

Disasm::Disasm() 
	: is_init(false), m_buf(NULL), m_bufSize(0),
	startOffset(0), m_offset(0), m_disasmSize(0),
	m_iptr(0)
{
}

Disasm::~Disasm()
{
	is_init = false;
}

uint64_t Disasm::trimToBitMode(int64_t value, uint8_t bits) 
{
	offset_t lval = value;
	const size_t max_bits = sizeof(lval) * 8;
	const size_t dif = max_bits - bits;
	lval = (lval << dif) >> dif;
	return lval;
}


int64_t Disasm::signExtend(int64_t operand, size_t opSize)
{
	size_t opBits = opSize * 8;
	int64_t lval = operand;
	size_t dif = sizeof(lval) * 8 - opBits;
	lval = (operand << dif) >> dif;
	return lval;
}

QString Disasm::printBytes(const uint8_t* buf, const size_t size)
{
	QString str;
	for (size_t i = 0; i < size; i++) {
#if QT_VERSION >= 0x050000
		str += QString().asprintf("%02X", buf[i]);
#else
		str += QString().sprintf("%02X", buf[i]);
#endif
	}
	return str;
}

offset_t Disasm::getJmpDestAddr(offset_t currVA, int instrLen, int lVal) const
{
	int delta = instrLen + lVal;
	const offset_t addr = (offset_t)((int64_t)currVA + (int64_t)delta);
	return addr;
}

bool Disasm::isBranching(minidis::mnem_type m_mnemType)
{
	switch (m_mnemType) {
		case MT_COND_JUMP:
		case MT_LOOP:
			return true;
	}
	return isUnconditionalBranching(m_mnemType);
}

bool Disasm::isUnconditionalBranching(minidis::mnem_type m_mnemType)
{
	switch (m_mnemType) {
		case MT_JUMP:
		case MT_CALL:
			return true;
	}
	return false;
}
