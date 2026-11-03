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

int ExceptionTreeModel::totalCount() const
{
	ExceptionDirWrapper* wrap = dynamic_cast<ExceptionDirWrapper*>(wrapper());
	if (!wrap) return 0;
	return static_cast<int>(wrap->getEntriesCount());
}

int ExceptionTreeModel::pagesCount() const
{
	const int total = totalCount();
	return (total / limitPerPage) + (((total % limitPerPage) != 0) ? 1 : 0);
}

int ExceptionTreeModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) return 0; // a flat table
	const qint64 start = pageStartIndex();
	const qint64 total = totalCount();
	if (start >= total) return 0;
	const qint64 remaining = total - start;
	return static_cast<int>(remaining < limitPerPage ? remaining : limitPerPage);
}

void ExceptionTreeModel::setPage(int page)
{
	const int last = qMax(0, pagesCount() - 1);
	page = qBound(0, page, last);
	if (page == pageNum) return;
	beginResetModel();
	pageNum = page;
	endResetModel();
	emit pagingChanged();
}

void ExceptionTreeModel::setMaxPerPage(int maxPerPage)
{
	maxPerPage = qMax(1, maxPerPage);
	if (maxPerPage == limitPerPage) return;
	beginResetModel();
	/* keep the first record that was on the page in view */
	const qint64 firstShown = pageStartIndex();
	limitPerPage = maxPerPage;
	pageNum = static_cast<int>(firstShown / limitPerPage);
	endResetModel();
	emit pagingChanged();
}

QVariant ExceptionTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case OFFSET: return tr("Offset");
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
	/* the row is relative to the page */
	const qint64 entryIndex = pageStartIndex() + index.row();
	if (index.row() < 0 || entryIndex >= totalCount()) return NULL;
	return wrap->getEntryAt(static_cast<size_t>(entryIndex));
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

	bool isOk = false;
	ULONGLONG number = value.toString().toLongLong(&isOk, 16);
	if (!isOk) return false;

	const offset_t offset = entry->getFieldOffset(fID);
	const bufsize_t fieldSize = entry->getFieldSize(fID);

	this->myPeHndl->backupModification(offset, fieldSize);
	const bool isModified = entry->setNumValue(fID, index.column(), number);
	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}
