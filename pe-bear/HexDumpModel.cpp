#include "HexDumpModel.h"

#define HEX_COL_NUM 16
#define COL_NUM (HEX_COL_NUM)

HexDumpModel::HexDumpModel(PeHandler *peHndl, bool isHexFormat, QObject *parent)
	: PeTableModel(peHndl, parent),
	showHex(isHexFormat),
	startOff(0), endOff(0), pageSize(PREVIEW_SIZE)
{
	addrType = Executable::RAW;
	connectSignals();
	connect(myPeHndl, SIGNAL(marked()), this, SLOT (onNeedReset()));
}

void HexDumpModel::setShownContent(offset_t start, bufsize_t size)
{
	this->startOff = start;
	endOff = start + size;
	reset();
	emit scrollReset();
}

int HexDumpModel::rowCount(const QModelIndex &parent) const
{
	const bufsize_t peSize = this->m_PE->getRawSize();
	if (peSize < this->startOff) return 0;

	size_t viewSize = pageSize;
	size_t diff = peSize - this->startOff;
	if (diff < pageSize) {
		viewSize = diff;
	}
	return pe_util::unitsCount(viewSize, HEX_COL_NUM);
}

int HexDumpModel::columnCount(const QModelIndex &parent) const
{
	return COL_NUM;
}

QVariant HexDumpModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal) {
		if (role == Qt::DisplayRole) {
			if (section == COL_NUM) return QVariant();
			return QString::number(section, 16).toUpper();
		}
		if (role == Qt::SizeHintRole) return QVariant();
	}
	if (orientation == Qt::Vertical) {

		if (role == Qt::SizeHintRole) {
			return settings.getVerticalSize();
		}

		uint32_t offset = this->startOff + (section * HEX_COL_NUM);

		if (role == Qt::DisplayRole) {
			return QString::number(offset, 16).toUpper();
		}
		if ( role == Qt::ToolTipRole) {
			return QString::number(offset, 16).toUpper() +"\nRight click to follow.";
		}
	}

	if (role == Qt::FontRole) {
		QFont hdrFont = settings.myFont;
		hdrFont.setBold(true);
		hdrFont.setItalic(false);
		return hdrFont;
	}
	return QVariant();
}

Qt::ItemFlags HexDumpModel::flags(const QModelIndex &index) const 
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

offset_t HexDumpModel::contentIndexAt(const QModelIndex &index) const
{
	if (!index.isValid()) return (-1);

	offset_t fileSize = m_PE->getRawSize();
	int x = index.column();
	int y = index.row();

	y *= HEX_COL_NUM;
	offset_t indx = (y + x) + startOff;

	if (indx >= fileSize) {
		return (-1); /* out of bounds */
	}
	return indx;
}

QVariant HexDumpModel::getRawContentAt(const QModelIndex &index) const
{
	uint64_t indx = contentIndexAt(index);
	if (indx == (-1)) return QVariant();

	BYTE* contentPtr = m_PE->getContent();
	const QChar c = contentPtr[indx];
	return c;
}

QVariant HexDumpModel::getElement(size_t offset) const
{
	if (!m_PE || offset > m_PE->getContentSize()) {
		return QVariant();
	}
	BYTE* contentPtr = m_PE->getContentAt(offset, 1);
	if (!contentPtr) {
		return QVariant();
	}
	if (showHex) {
#if QT_VERSION >= 0x050000
		return QString().asprintf("%02X", *contentPtr);
#else
		return QString().sprintf("%02X", *contentPtr);
#endif
	}
	
	const QChar c = QChar(*contentPtr);
	if (c.isPrint() && !c.isSpace())
		return c;
	return QChar('.');
}

QVariant HexDumpModel::data(const QModelIndex &index, int role) const
{
	if (index.isValid() == false) return QVariant();
	
	if (role == Qt::SizeHintRole) {
		return QVariant();
	}
	
	if (index.column() >= COL_NUM) {
		return QVariant();
	}

	int x = index.column();
	int y = index.row() * HEX_COL_NUM;
	size_t offset = (y + x) + this->startOff;
	if (offset >= m_PE->getRawSize()) {
		return QVariant(); /* out of bounds */
	}

	if (offset >= myPeHndl->hoveredOffset 
		&& offset < (myPeHndl->hoveredOffset + myPeHndl->hoveredSize))
	{
		if (role == Qt::BackgroundColorRole) return settings.hoveredColor;
	}
	
	if (role == Qt::FontRole) {
		return settings.myFont;
	}
	if (role == Qt::ToolTipRole) {
		return "Double-click to edit";
	}
	if (role == Qt::ForegroundRole) {
		bool isActiveArea = this->myPeHndl->isInActiveArea(offset);
		bool isModifiedArea = this->myPeHndl->isInModifiedArea(offset);

		if (isModifiedArea) {
			if (!isActiveArea) return this->settings.inactiveModifColor;
			return this->settings.modifColor;
		}
		if (!isActiveArea) {
			return this->settings.inactiveColor;
		}
	}
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		return getElement(offset);
	}
	return QVariant();
}

bool HexDumpModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid() == false) return false;
	if (!myPeHndl || !m_PE) return false;

	offset_t offset = contentIndexAt(index);
	if (offset == INVALID_ADDR) return false;
	
	QString text = value.toString();
	const size_t el_size = (showHex) ? 2 : 1;
	if (text.length() < el_size) {
		return false;
	}
	text = text.left(el_size);

	BYTE* contentPtr = m_PE->getContentAt(offset, 1);
	if (!contentPtr) return false;
	
	BYTE prev_val = (*contentPtr);
	BYTE val = 0;

	if (showHex) {
		bool isConv = false;
		BYTE number = text.toUShort(&isConv, 16);
		if (!isConv) return false;
		val = number;
	} else {
#if QT_VERSION >= 0x050000
		val = (BYTE) text.at(0).toLatin1();
#else
		val = (BYTE) text.at(0).toAscii();
#endif
	}
	if (prev_val == val) {
		return false; // nothing has changed
	}
	myPeHndl->backupModification(offset, 1);
	(*contentPtr) = val;
	myPeHndl->setBlockModified(offset, 1);
	return true;
}
