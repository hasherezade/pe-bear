#include "HexDumpModel.h"

#define HEX_COL_NUM 16
#define COL_NUM (HEX_COL_NUM)

HexDumpModel::HexDumpModel(PeHandler *peHndl, bool isHexFormat, QObject *parent)
	: PeTableModel(peHndl, parent),
	showHex(isHexFormat),
	startOff(0), endOff(0), pageSize(PREVIEW_SIZE),
	addrType(Executable::RAW)
{
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
	if (startOff == INVALID_ADDR) return 0;

	const offset_t peSize = this->m_PE->getRawSize();
	if (peSize < this->startOff) return 0;

	bufsize_t viewSize = pageSize;
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

		offset_t offset = this->startOff + (section * HEX_COL_NUM);

		if (role == Qt::DisplayRole) {
			return QString::number(offset, 16).toUpper();
		}
		if ( role == Qt::ToolTipRole) {
			return QString::number(offset, 16).toUpper() +"\n"+ tr("Right click to follow.");
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

offset_t HexDumpModel::contentOffsetAt(const QModelIndex &index) const
{
	if (!index.isValid()) return INVALID_ADDR;

	offset_t fileSize = m_PE->getRawSize();
	int x = index.column();
	int y = index.row();

	y *= HEX_COL_NUM;
	offset_t offset = (y + x) + startOff;

	if (offset >= fileSize) {
		return INVALID_ADDR; /* out of bounds */
	}
	return offset;
}

QVariant HexDumpModel::getRawContentAt(const QModelIndex &index) const
{
	offset_t offset = contentOffsetAt(index);
	if (offset == INVALID_ADDR) return QVariant();

	BYTE* contentPtr = m_PE->getContentAt(offset, 1);
	if (!contentPtr) {
		return QVariant();
	}
	const QChar c = contentPtr[0];
	return c;
}

QVariant HexDumpModel::getElement(offset_t offset) const
{
	if (!m_PE || offset == INVALID_ADDR || offset > m_PE->getContentSize()) {
		return QVariant();
	}
	BYTE* contentPtr = m_PE->getContentAt(offset, 1);
	if (!contentPtr) {
		return QVariant();
	}
	const BYTE val = contentPtr[0];
	if (showHex) {
#if QT_VERSION >= 0x050000
		return QString().asprintf("%02X", val);
#else
		return QString().sprintf("%02X", val);
#endif
	}
	
	const QChar c = val;
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
	offset_t offset = (y + x) + this->startOff;
	if (offset >= m_PE->getRawSize()) {
		return QVariant(); /* out of bounds */
	}

	if (offset >= myPeHndl->hoveredOffset 
		&& offset < (myPeHndl->hoveredOffset + myPeHndl->hoveredSize))
	{
		if (role == Qt::BackgroundRole) return settings.hoveredColor;
	}
	
	if (role == Qt::FontRole) {
		return settings.myFont;
	}
	if (role == Qt::ToolTipRole) {
		return tr("Double-click to edit");
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

	offset_t offset = contentOffsetAt(index);
	if (offset == INVALID_ADDR) return false;
	
	QString text = value.toString();
	const size_t el_size = (showHex) ? 2 : 1;
	if (text.length() < el_size) {
		return false;
	}
	text = text.left(el_size);
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
	return myPeHndl->setByte(offset, val);
}
