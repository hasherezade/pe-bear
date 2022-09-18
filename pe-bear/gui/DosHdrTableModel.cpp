#include "DosHdrTableModel.h"

enum COLS {
	COL_OFFSET = 0,
	COL_NAME,
	COL_VALUE,
	MAX_COL
};


DosHdrTableModel::DosHdrTableModel(PeHandler *peHndl, QObject *parent)
    : WrapperTableModel(peHndl, parent)
{
	connectSignals();
}

QVariant DosHdrTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case COL_OFFSET: return "Offset";
		case COL_NAME: return "Name";
		case COL_VALUE:  return "Value";
	}
	return QVariant();
}

Qt::ItemFlags DosHdrTableModel::flags(const QModelIndex &index) const
{ 
	if (!index.isValid()) return Qt::NoItemFlags;

	if (index.column() == COL_VALUE) return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool DosHdrTableModel::isComplexValue(const QModelIndex &index) const
{
	int fId = getFID(index);

	if (fId == DosHdrWrapper::RES || fId == DosHdrWrapper::RES2) return true;
	return false;
}

QVariant DosHdrTableModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::ForegroundRole) return this->addrColor(index);
	if (role == Qt::FontRole) {
		if (index.column() == COL_OFFSET || index.column() == COL_VALUE) {
			return offsetFont;
		}
	}
	if (role == Qt::ToolTipRole) return toolTip(index);
	if (role == Qt::DisplayRole || role == Qt::EditRole) {

		if (index.column() == COL_OFFSET) return QString::number(getFieldOffset(index), 16);
		if (index.column() == COL_NAME) {
			int fId = getFID(index);
			return wrapper()->getFieldName(fId);
		}
		return this->dataValue(index);
	}	
	return QVariant();
}

bool DosHdrTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;

	int fId = getFID(index);
	uint32_t offset = wrapper()->getFieldOffset(fId);
	uint32_t fieldSize = wrapper()->getFieldSize(fId);

	bool isModified = false;

	if (this->isComplexValue(index)) {
		this->myPeHndl->backupModification(offset, fieldSize);
		isModified = setComplexValue(index, value, role);
	} else {
		QString text = value.toString();
		bool isOk = false;
		ULONGLONG number = text.toLongLong(&isOk, 16);
		if (!isOk) return false;

		this->myPeHndl->backupModification(offset, fieldSize);
		isModified = wrapper()->setNumValue(fId, index.column(), number);
	}

	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}

