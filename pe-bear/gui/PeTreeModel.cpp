#include "PeTreeModel.h"

#define COL_OFFSET 0

PeTreeItem::PeTreeItem(PeHandler *peHndl, int _fID, int _sID, PeTreeItem *parent)
	:  TreeItem(parent), PeViewItem(peHndl),
	fID(_fID), sID(_sID),
	errColor(ERR_COLOR)
{
	errColor.setAlpha(170);

	offsetFont.setCapitalization(QFont::AllUppercase);
	offsetFontColor.setRgb(100, 100, 100);
}

bool PeTreeItem::canFollow(int column, int row) const
{
	if (column == COL_OFFSET) return true;
	return false;
}

bool PeTreeItem::containsRVA(int column, int row) const
{
	if (column == COL_OFFSET) return false;
	return true;
}

QVariant PeTreeItem::toolTip(int column) const
{
	if (!m_PE) return QVariant();
	if (canFollow(column, 0)) return "Right click to follow";
	return data(column);
}

bool PeTreeItem::setData(int column, const QVariant &value, int role)
{
	if ((this->flags(column) & Qt::ItemIsEditable) == 0) {
		return false;
	}
	QVariant prevVal = this->data(column);
	if (prevVal == value) {
		return false;
	}
	const offset_t offset = this->getContentOffset();
	const offset_t size = this->getContentSize();
	if (!size){
		return false;
	}
	
	this->myPeHndl->backupModification(offset, size);
	if (setDataValue(column, value)) {
		this->myPeHndl->setBlockModified(offset, size);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}

//--------------------------------------------------------------------------

size_t PeTreeModel::counter = 0;

PeTreeModel::PeTreeModel(PeHandler *peHndl, QObject *parent, bool isExpandable )
	: TreeModel(parent), PeViewItem(peHndl),
	m_isExpandable(isExpandable)
{
	PeTreeModel::counter++;
	
	offsetFont.setCapitalization(QFont::AllUppercase);
	offsetFontColor.setRgb(100, 100,100);
	errColor.setRgb(255, 0, 0);
	errColor.setAlpha(addrColors.flagsAlpha());
}

void PeTreeModel::connectSignals()
{
	if (!this->myPeHndl) {
		return;
	}
	connect(this->myPeHndl, SIGNAL(modified()), this, SLOT(onNeedReset()));
}

Executable::addr_type PeTreeModel::addrTypeAt(QModelIndex index) const
{
	if (!index.isValid() || !m_PE) return Executable::NOT_ADDR;
	if (index.column() == COL_OFFSET) return Executable::RAW;
	return Executable::NOT_ADDR;
}

QVariant PeTreeModel::addrColor(const QModelIndex &index) const
{
	Executable::addr_type aT = addrTypeAt(index);
	QColor col = addrColors.addrTypeToColor(aT);
	if (col != QColor()) return col;
	return QVariant();
}

QVariant PeTreeModel::toolTip(QModelIndex index) const
{
	if (!index.isValid()) return "";

	QString desc = "Right click to follow ";
	
	switch (addrTypeAt(index)) {
		case Executable::RAW: return desc + "[raw]";
		case Executable::RVA: return desc + "[RVA]";
		case Executable::VA: return desc + "[VA]";
	}
	return "";
}

offset_t PeTreeModel::getFieldOffset(QModelIndex index) const
{
	if (!index.isValid()) return 0;
	TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
	PeTreeItem *it = dynamic_cast<PeTreeItem*>(item);
	if (!it) {
		return 0;
	}
	return it->getContentOffset();
}

bufsize_t PeTreeModel::getFieldSize(QModelIndex index) const
{
	if (!index.isValid()) return 0;
	TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
	PeTreeItem *it = dynamic_cast<PeTreeItem*>(item);
	if (!it) {
		return 0;
	}
	return it->getContentSize();
}

