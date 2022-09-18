#include "ExceptionTreeModel.h"

//-----------------------------------------------------------------------------

#define NOT_FILLED  "-"

int ExceptionTreeModel::columnCount(const QModelIndex &parent) const
{
	ExceptionDirWrapper* wrap = dynamic_cast<ExceptionDirWrapper*>(wrapper());
	if (!wrap) return 0;
	ExeNodeWrapper* entry = wrap->getEntryAt(0);
	if (!entry) return 0;
	uint32_t cntr = entry->getFieldsCount() + ADDED_COLS_NUM;
	return cntr;
}

QVariant ExceptionTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case OFFSET: return "Offset";
	}
	ExceptionDirWrapper* impWrap = dynamic_cast<ExceptionDirWrapper*>(wrapper());
	if (!impWrap) return QVariant();
	int32_t fID = this->columnToFID(section);
	return impWrap->getFieldName(0, fID);
}

ExeElementWrapper* ExceptionTreeModel::wrapperAt(QModelIndex index) const
{
	ExceptionDirWrapper* wrap = dynamic_cast<ExceptionDirWrapper*>(wrapper());
	if (!wrap) return NULL;
	return wrap->getEntryAt(index.row());
}

QVariant ExceptionTreeModel::data(const QModelIndex &index, int role) const
{
	ExceptionDirWrapper* impWrap = dynamic_cast<ExceptionDirWrapper*>(wrapper());
	if (!impWrap) return QVariant();

	int column = index.column();
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	if (role == Qt::FontRole) return offsetFont;
	if (role == Qt::ToolTipRole) return toolTip(index);

	ExceptionEntryWrapper* entry =  dynamic_cast<ExceptionEntryWrapper*>(wrapperAt(index));
	if (!entry) return QVariant();

	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
	}
	bool isOk = false;
	uint64_t val = entry->getNumValue(getFID(index), &isOk);
	if (!isOk) return "UNK"; /* other type? */

	return QString::number(val, 16);
}

bool ExceptionTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;

	size_t fID = getFID(index);

	if (!wrapper()) return false;
	ExceptionEntryWrapper* entry =  dynamic_cast<ExceptionEntryWrapper*>(wrapperAt(index));
	if (!entry) return false;

	QString text = value.toString();

	bool isModified = false;
	uint32_t offset = 0;
	uint32_t fieldSize = 0;

	bool isOk = false;
	ULONGLONG number = text.toLongLong(&isOk, 16);
	if (!isOk) return false;

	offset = entry->getFieldOffset(fID);
	fieldSize = entry->getFieldSize(fID);

	this->myPeHndl->backupModification(offset, fieldSize);
	isModified = entry->setNumValue(fID, index.column(), number);

	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}

//----------------------------------------------------------------------------

void  ExceptionTreeView::selectionChanged(const QItemSelection &newSel, const QItemSelection &prevSel)
{
	FollowablePeTreeView::selectionChanged(newSel, prevSel);

	ExceptionTreeModel *impModel = dynamic_cast<ExceptionTreeModel*> (this->model());
	if (!impModel) return;
	if (!impModel->wrapper()) return;
	if (newSel.indexes().size() == 0) return;
	QModelIndex index = newSel.indexes().at(0);
	uint32_t libId = index.row();
	emit librarySelected(libId);
}