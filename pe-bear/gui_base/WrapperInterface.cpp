#include "WrapperInterface.h"

#define COL_OFFSET 0

//---------------------------------------
WrapperInterface::WrapperInterface()
{
}

bool WrapperInterface::containsOffset(QModelIndex index) const
{
	if (index.column() == COL_OFFSET) {
		return true;
	}
	return false;
}

QVariant WrapperInterface::dataValue(const QModelIndex &index) const
{
	ExeElementWrapper *wrapper = this->wrapperAt(index);
	if (!wrapper) return false;

	if (isComplexValue(index)) return complexValue(index);

	bool isOk = false;
	uint64_t val = wrapper->getNumValue(getFID(index), getSID(index), &isOk);
	if (isOk) return QString::number(val, 16);

	return complexValue(index);
}

QVariant WrapperInterface::complexValue(const QModelIndex &index) const
{
	if (!index.isValid()) return QVariant();
	ExeElementWrapper *wrapper = this->wrapperAt(index);
	if (!wrapper) return false;

	int fId = getFID(index);
	int sId = getSID(index);

	uint32_t fieldSize = wrapper->getFieldSize(fId, sId);
	WORD* ptr = (WORD*) wrapper->getFieldPtr(fId, sId);
	if (ptr == NULL || fieldSize == 0) return "INVALID";

	int cntr = fieldSize / sizeof(WORD);
	const int CNTR_MAX = 50;
	QStringList strL;
	for (int i = 0; i < cntr && i < CNTR_MAX; i++) {
		strL.append(QString::number(ptr[i], 16));
	}
	if (cntr > CNTR_MAX) strL.append("...");
	return strL.join(", ");
}

Executable::addr_type WrapperInterface::addrTypeAt(QModelIndex index) const
{
	if (containsOffset(index)) {
		return offsetAddrType();
	}
	if (!containsValue(index)) return Executable::NOT_ADDR;

	ExeElementWrapper *wrapper = this->wrapperAt(index);
	if (!wrapper) return Executable::NOT_ADDR;

	Executable::addr_type aType = Executable::NOT_ADDR;
	aType = wrapper->containsAddrType(getFID(index), getSID(index));
	return aType;
}

offset_t WrapperInterface::getFieldOffset(QModelIndex index) const
{ 
	if (!index.isValid()) return 0;
	int fieldId = getFID(index);
	return wrapperAt(index) ? wrapperAt(index)->getFieldOffset(fieldId) : 0;
}

bufsize_t WrapperInterface::getFieldSize(QModelIndex index) const 
{
	if (!index.isValid()) return 0;
	int fieldId = getFID(index);
	return wrapperAt(index) ? wrapperAt(index)->getFieldSize(fieldId) : 0;
}

