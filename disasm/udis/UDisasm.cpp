#include "UDisasm.h"

using namespace pe_bear;

const int UDisasm::MAX_ARG_NUM = 2;

UDisasm::UDisasm() 
	: Disasm()
{
}

UDisasm::~UDisasm()
{
	is_init = false;
}

bool UDisasm::init(uint8_t* buf, size_t bufSize, size_t disasmSize, offset_t offset, Executable::exe_bits bitMode)
{
	QMutexLocker locker(&m_disasmMutex);
	is_init = false;
	if (!buf || bufSize == 0) return false;

	m_buf = buf;
	m_bufSize = bufSize;
	m_iptr = 0;

	m_disasmSize = disasmSize;
	m_offset = 0;
	this->startOffset = this->convertToVA(offset);
	m_bitMode = bitMode;
    
    //udis:
	ud_init(&this->ud_obj);
	ud_set_input_buffer(&this->ud_obj, m_buf, m_bufSize);
	ud_set_mode(&this->ud_obj, (uint8_t)bitMode);
	ud_set_syntax(&ud_obj, UD_SYN_INTEL);

	is_init = true;
	return true;
}

size_t UDisasm::disasmNext()
{
	if (!is_init)
		return 0;
        
	if (!ud_disassemble(&this->ud_obj)) {
		is_init = false;
		return 0;
	}

	size_t step =  ud_insn_len(&this->ud_obj);
	m_iptr += step;
	return step;
}

bool UDisasm::fillTable()
{
	QMutexLocker locker(&m_disasmMutex);
	if (!is_init){
		return false;
	}
	this->clearTable();
	size_t processedSize = 0;
	while (processedSize < this->m_disasmSize) {
		size_t step = disasmNext();
		if (!step) {
			break; //could not disasemble more
		}
		processedSize += step;
		m_table.push_back(ud_obj);
	}
	if (m_table.size() == 0) {
		return false;
	}
	return true;
}

bool UDisasm::clearTable()
{
	m_table.clear();
	return true;
}

size_t UDisasm::getChunkSize(int index) const
{
	if (index >= m_table.size()) return 0;
	ud_t obj =  m_table.at(index);
	size_t chunkSize = ud_insn_len(&obj);
	return chunkSize;
}

uint64_t UDisasm::getRawAt(int index) const
{
	if (index >= m_table.size()) return INVALID_ADDR;
	ud_t ud_obj =  m_table.at(index);
	return ud_insn_off(&ud_obj);
}

bool UDisasm::isAddrOperand(int index) const
{
	ud_t obj =  m_table.at(index);
	if (obj.mnemonic == UD_Ipush || obj.mnemonic == UD_Imov) return true;

	for (int i = 0 ; i <Disasm::MAX_ARG_NUM; i++) {
		if (obj.operand[i].type == UD_OP_IMM 
			&& obj.operand[i].size > 8)
		{
			return true;
		}
	}
	return false;
}

uint64_t UDisasm::getArgVA(int index, int argNum, bool &isOk) const
{
	if (index >= m_table.size()) return 0;
	isOk = false;
	if (argNum > MAX_ARG_NUM) return INVALID_ADDR;

	if (index >= m_table.size()) {
		printf("Out of bounds\n");
		return INVALID_ADDR;
	}

	int instrLen = getChunkSize(index);
	uint64_t currVA = getVaAt(index);// + myPe->getImageBase();

	bool got = false;
	uint64_t lval = getSignedLVal(index, argNum, got);
	if (!got) return INVALID_ADDR;
	
	ud_t inpObj = m_table.at(index);
	ud_type opType = inpObj.operand[argNum].type;
	
	uint64_t va = INVALID_ADDR;
	if (opType == UD_OP_JIMM) {
		va = this->getJmpDestAddr(currVA, instrLen, lval);
	}
	//TODO: implement it
	
	if (opType == UD_OP_MEM || opType == UD_OP_IMM) {
		ud_type regType = inpObj.operand[argNum].base;

		if (regType == UD_R_RIP) {
			va = this->getJmpDestAddr(currVA, instrLen, lval);
		} 
		else if (regType == UD_NONE) { // may be VA
			va = Disasm::trimToBitMode(lval, this->m_bitMode);
			/*if (myPe->isValidVA(lval)) {
				rva = lval - myPe->getImageBase();
			}*/
		}
	}
	if (va != INVALID_ADDR) {
		isOk = true;
		va = Disasm::trimToBitMode(va, this->m_bitMode);
	}
	return va;
}

QString UDisasm::translateBranching(int index) const
{
	if (index >= m_table.size()) return "";
	
	ud_t ud_obj =  m_table.at(index);
	/* substitute branching istructions with immediate operands */
	ud_mnemonic_code_t mnem = ud_obj.mnemonic;

	QString mnemDesc = "";
	switch (mnem) {
		case UD_Iloop: mnemDesc = "LOOP"; break;
	    case UD_Ijo : mnemDesc = "JO"; break;
	    case UD_Ijno : mnemDesc = "JNO"; break;
	    case UD_Ijb : mnemDesc = "JB"; break;
	    case UD_Ijae : mnemDesc = "JLAE"; break;
	    case UD_Ijz : mnemDesc = "JZ"; break;
	    case UD_Ijnz : mnemDesc = "JNZ"; break;
	    case UD_Ijbe : mnemDesc = "JBE"; break;
	    case UD_Ija : mnemDesc = "JA"; break;
	    case UD_Ijs : mnemDesc = "JS"; break;
	    case UD_Ijns : mnemDesc = "JNS"; break;
	    case UD_Ijp : mnemDesc = "JP"; break;
	    case UD_Ijnp : mnemDesc = "JNP"; break;
	    case UD_Ijl : mnemDesc = "JL"; break;
	    case UD_Ijge : mnemDesc = "JGE"; break;
	    case UD_Ijle : mnemDesc = "JLE"; break;
	    case UD_Ijg : mnemDesc = "JG"; break;
	    case UD_Ijcxz : mnemDesc = "JCXZ"; break;
	    case UD_Ijecxz : mnemDesc = "JECXZ"; break;
	    case UD_Ijrcxz : mnemDesc = "JRCXZ"; break;
	    case UD_Ijmp : mnemDesc = "JMP"; break;
	    case UD_Icall : mnemDesc = "CALL"; break;
	    default:
		return ud_insn_asm(&ud_obj);
	}

	if (ud_obj.operand[0].type != UD_OP_JIMM) {
		return ud_insn_asm(&ud_obj);
	}

	if (ud_obj.operand[0].size <= 8 && mnem != UD_Icall) {
		mnemDesc = mnemDesc + " SHORT";
	}

	bool isOk = false;
	uint64_t targetVA = getTargetVA(index, isOk);
	if (targetVA == INVALID_ADDR || !isOk) {
			mnemDesc += " <INVALID>";
			return mnemDesc;
	}
	mnemDesc += " 0x" + QString::number(targetVA, 16);
	return mnemDesc;
}

minidis::mnem_type UDisasm::getMnemType(size_t index) const
{
	using namespace minidis;
	if (index >= this->chunksCount()) {
		return MT_NONE;
	}
	const ud_t &ud_obj =  m_table.at(index);
	switch(ud_obj.mnemonic)
	{
		case UD_Ijo : case UD_Ijno : case UD_Ijb : case UD_Ijae :
		case UD_Ijz : case UD_Ijnz : case UD_Ijbe : case UD_Ija :
		case UD_Ijs : case UD_Ijns : case UD_Ijp : case UD_Ijnp :
		case UD_Ijl : case UD_Ijge : case UD_Ijle : case UD_Ijg :
		case UD_Ijcxz : case UD_Ijecxz : case UD_Ijrcxz :
			return MT_COND_JUMP;

		case UD_Ijmp :
			return MT_JUMP;

		case UD_Iloop: return MT_LOOP;
		case UD_Icall : return MT_CALL;

		case UD_Iret:
		case UD_Iretf :
			return MT_RET;

		case UD_Inop : return MT_NOP;
		case UD_Iinvalid :
			return MT_INVALID;

		case UD_Ipush :
		case UD_Ipusha:
		case UD_Ipushad:
		case UD_Ipushfd:
		case UD_Ipushfq:
		case UD_Ipushfw:
			return MT_PUSH;

		case UD_Ipop :
		case UD_Ipopa:
		case UD_Ipopad:
		//case UD_Ipopcnt:
		case UD_Ipopfd:
		case UD_Ipopfq:
		case UD_Ipopfw:
			return MT_POP;

		case UD_Iint3 :
			return MT_INT3;

		case UD_Iint:
			return MT_INTX;
	}
	return MT_OTHER;
}

bool UDisasm::isBranching(ud_t ud_obj)
{
	ud_mnemonic_code_t mnem = ud_obj.mnemonic;
	switch (mnem) {
		case UD_Iloop:
		case UD_Ijo : case UD_Ijno : case UD_Ijb : case UD_Ijae :
		case UD_Ijz : case UD_Ijnz : case UD_Ijbe : case UD_Ija :
		case UD_Ijs : case UD_Ijns : case UD_Ijp : case UD_Ijnp :
		case UD_Ijl : case UD_Ijge : case UD_Ijle : case UD_Ijg :
		case UD_Ijcxz : case UD_Ijecxz : case UD_Ijrcxz :
		case UD_Ijmp :
		case UD_Icall :
			return true;
	}
	return false;
}

bool UDisasm::isFollowable(const int y) const
{
	if (y >= this->chunksCount()) return false;
	
	if (getRvaAt(y) == INVALID_ADDR) return false;

	if (isBranching(y) == false && isPushRet(y) == false) {
		return false;
	}
	const ud_t &obj =  m_table.at(y);
	if (obj.operand[0].type == UD_OP_JIMM) {
		return true;
	}

	if (obj.operand[0].type == UD_OP_MEM || obj.operand[0].type == UD_OP_IMM) {
		ud_type reg_type = obj.operand[0].base;
		if (reg_type == UD_NONE) {
			return true;
		}

		if (reg_type == UD_R_RIP) {
			return true;
		}
		return false;
	}
	return false;
}

uint64_t UDisasm::getJmpDestAddr(uint64_t currVA, int instrLen, int lVal) const
{
	int delta = instrLen + lVal;
	uint64_t addr = currVA + delta;
	return addr;
}

/* TODO: test it!!! */
int64_t UDisasm::getSignedLVal(ud_t &inpObj, size_t argNum, bool &isOk) const
{
	if (argNum > MAX_ARG_NUM) return INVALID_ADDR;
	isOk = false;
	int64_t lValue = 0;
	uint8_t size = inpObj.operand[argNum].size;

	if (size == 0) return 0;
	uint8_t maxSize = sizeof(uint64_t) * 8; // in bits
	uint8_t dif = maxSize - size;

	lValue = inpObj.operand[argNum].lval.uqword;
	int64_t mlValue = (lValue << dif) >> dif; // gives signed!!!
	isOk = true;
	return mlValue;
}

bool UDisasm::isUnconditionalBranching(ud_t ud_obj)
{
	ud_mnemonic_code_t mnem = ud_obj.mnemonic;
	switch (mnem) {
		case UD_Ijmp :
		case UD_Icall :
			return true;
	}
	return false;
}

bool UDisasm::isPushRet(int tab_index, /*out*/ int* ret_index) const
{
	ud_t ud_obj =  m_table.at(tab_index);
	ud_mnemonic_code_t mnem = ud_obj.mnemonic;

	/* push ... ret = CALL */
	if (mnem == UD_Ipush) {
		int y2 = tab_index + 1;
		if (y2 >= m_table.size()) {
			return false;
		}
		ud_t ud_obj2 =  m_table.at(y2);
		if (ud_obj2.mnemonic == UD_Iret) {
			if (ret_index != NULL) {
				(*ret_index) = y2;
			}
			return true;
		}
	}
	//TODO: implement more complex cases
	return false;
}
