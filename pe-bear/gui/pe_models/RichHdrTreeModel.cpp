#include "RichHdrTreeModel.h"

#include "../../DateDisplay.h"

//#define MAX_COL 3
///------------------------
enum COLS {
	COL_OFFSET = 0,
	COL_NAME,
	COL_VALUE,
	COL_CLEAN_VALUE,
	COL_MEANING,
	COL_CID_PROD,
	COL_CID_BUILD,
	COL_CID_COUNT,
	COL_CID_VSVER,
	MAX_COL
};

RichHdrTreeItem::RichHdrTreeItem(PeHandler *peHndl, RichHdrWrapper::FieldID role, RichHdrTreeItem *parent)
	: PeTreeItem(peHndl, role, FIELD_NONE, parent),
	approvedColor(APPROVED_COLOR)
{
	approvedColor.setAlpha(160);
	this->role = role;
	if (peHndl == NULL) {
		this->role = RichHdrWrapper::NONE;
		return;
	}
}

int RichHdrTreeItem::columnCount() const
{
	return MAX_COL;
}

QVariant RichHdrTreeItem::background(int column) const
{
	RichHdrWrapper *richHdr = this->getRichHdr();
	if (!richHdr) return QVariant();
	
	//mark mismatching checksum red
	const size_t cnt = richHdr->compIdCount() - 1;
	if (role == (RichHdrWrapper::CHECKSUM + (cnt))) {
		if (column == COL_VALUE) {
			bool isOk = false;
			DWORD val = richHdr->getNumValue(role, &isOk);
			if (richHdr->calcChecksum() != val) return QColor(ERR_COLOR);
		} else if (column == COL_MEANING) {
			return approvedColor;
		}
	}
	return QVariant();
}

QVariant RichHdrTreeItem::foreground(int column) const
{
	if (column == COL_OFFSET) return this->offsetFontColor;
	return QVariant();
}

Qt::ItemFlags RichHdrTreeItem::flags(int column) const
{
	Qt::ItemFlags fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if (column != COL_VALUE) return fl;
	return fl| Qt::ItemIsEditable;
}

QVariant RichHdrTreeItem::data(int column) const
{
	RichHdrWrapper *richHdr = getRichHdr();
	if (!richHdr) return QVariant();

	switch (column) {
		case COL_OFFSET: return QString::number(getContentOffset(),16).toUpper();
		case COL_NAME: return (richHdr->getFieldName(role));
		case COL_VALUE:
		{
			bool isOk = false;
			uint64_t val = richHdr->getNumValue(role, &isOk);
			if (!isOk) return QVariant();
			return QString::number(val, 16);
		}
		case COL_CLEAN_VALUE:
		{
			const pe::RICH_COMP_ID compId = richHdr->getCompId(role);
			bool isOk = false;
			uint64_t val = richHdr->getNumValue(role, &isOk);
			if (!isOk) return QVariant();
			const size_t cnt = richHdr->compIdCount() - 1;
			uint64_t xor_val = richHdr->getNumValue(RichHdrWrapper::CHECKSUM + (cnt), &isOk);
			if (!isOk) return QVariant();
			if (role >= RichHdrWrapper::COMP_ID_1 && role <= RichHdrWrapper::COMP_ID_1 + cnt) {
				xor_val = xor_val << 32 | xor_val;
			} else if (role > RichHdrWrapper::COMP_ID_1 + cnt) {
				return QVariant();
			}
			return QString::number(val ^ xor_val, 16);
		}
		case COL_MEANING:
		{
			return richHdr->translateFieldContent(role);
		}
		case COL_CID_PROD:
		{
			const pe::RICH_COMP_ID compId = richHdr->getCompId(role);
			if (compId.count > 0) {
				return RichHdr_translateProdId(compId.prodId);
			}
		}
		case COL_CID_BUILD:
		{
			const pe::RICH_COMP_ID compId = richHdr->getCompId(role);
			if (compId.count > 0) {
				return  QString::number(compId.CV);
			}
		}
		case COL_CID_COUNT:
		{
			const pe::RICH_COMP_ID compId = richHdr->getCompId(role);
			if (compId.count > 0) {
				return QString::number(compId.count);
			}
		}
		case COL_CID_VSVER:
		{
			const pe::RICH_COMP_ID compId = richHdr->getCompId(role);
			if (compId.count > 0) {
				return RichHdr_ProdIdToVSversion(compId.prodId);
			}
		}
	}
	return QVariant();
}

bool RichHdrTreeItem::setDataValue(int column, const QVariant &value)
{
	RichHdrWrapper *richHdr = getRichHdr();
	if (!richHdr) return false;
	
	if (column != COL_VALUE) return false;

	QString text = value.toString();
	bool isOk = false;
	ULONGLONG number = text.toULongLong(&isOk, 16);
	if (!isOk) return false;

	isOk = richHdr->setNumValue(role, number);
	return isOk;
}

offset_t RichHdrTreeItem::getContentOffset() const
{
	RichHdrWrapper *richHdr = getRichHdr();
	if (!richHdr) return 0;
	return richHdr->getFieldOffset(role);
}

bufsize_t RichHdrTreeItem::getContentSize() const
{
	RichHdrWrapper *richHdr = getRichHdr();
	if (!richHdr) return 0;
	return richHdr->getFieldSize(role);
}

//-----------------------------------------------------------------------------

RichHdrTreeModel::RichHdrTreeModel(PeHandler* peHndl, QObject *parent)
	: WrapperTableModel(peHndl, parent)
{
}

void RichHdrTreeModel::onNeedReset()
{
	if (myPeHndl) {
		this->myPeHndl->richHdrWrapper.wrap();
	}
	reset();
	emit modelUpdated();
}

bool RichHdrTreeModel::containsValue(QModelIndex index) const
{
	return (index.column() == COL_VALUE);
}

int RichHdrTreeModel::columnCount(const QModelIndex &parent) const
{
	RichHdrWrapper* entry =  dynamic_cast<RichHdrWrapper*>(wrapper());
	if (!entry) return 0;
	
	return MAX_COL;
}

QVariant RichHdrTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) return QVariant();
	
	RichHdrTreeItem item(myPeHndl, RichHdrWrapper::FieldID(index.row()));
	
	if (role == Qt::ForegroundRole) return this->addrColor(index);
	if (role == Qt::ToolTipRole) {
		RichHdrWrapper *richHdr = item.getRichHdr();
		if (!richHdr) return QVariant();
		size_t cnt = richHdr->compIdCount() - 1;
		if (index.row() == (RichHdrWrapper::CHECKSUM + cnt)) {
			if (index.column() == COL_MEANING) {
				return tr("Calculated checksum");
			}
		}
		return this->toolTip(index);
	}

	if (role == Qt::BackgroundRole)
		return item.background(index.column());

	if ( role == Qt::SizeHintRole) {
		if (index.column() == COL_OFFSET) return QSize(50, 16);
		if (index.column() == COL_NAME) return QSize(150, 16);
		return QVariant(); //get default
	}

	if (role == Qt::DisplayRole || role == Qt::EditRole)
		return item.data(index.column());

	return QVariant();
}

bool RichHdrTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	RichHdrTreeItem item(myPeHndl, RichHdrWrapper::FieldID(index.row()));
	return item.setData(index.column(), value, item.role);
}


QVariant RichHdrTreeModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
{
	if(role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case COL_OFFSET : return tr("Offset");
		case COL_NAME : return tr("Name");
		case COL_VALUE : return tr("Value");
		case COL_CLEAN_VALUE: return tr("Unmasked Value");
		case COL_MEANING : return tr("Meaning");
		case COL_CID_PROD : return tr("ProductId");
		case COL_CID_BUILD : return tr("BuildId");
		case COL_CID_COUNT : return tr("Count");
		case COL_CID_VSVER: return tr("VS version");
	}
	return QVariant();
}

Qt::ItemFlags RichHdrTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
		
	RichHdrTreeItem item(myPeHndl, RichHdrWrapper::FieldID(index.row()));
	return item.flags(index.column());
}

offset_t RichHdrTreeModel::getContentOffset() const
{
	if (!m_PE) return 0;
	return m_PE->peNtHdrOffset();
}

bufsize_t RichHdrTreeModel::getContentSize() const
{
	if (!m_PE) return 0;
	return (DWORD) m_PE->peNtHeadersSize();
}
