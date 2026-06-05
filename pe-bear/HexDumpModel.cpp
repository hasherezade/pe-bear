#include "HexDumpModel.h"

#define HEX_COL_NUM 16
#define COL_NUM (HEX_COL_NUM)

HexDumpModel::HexDumpModel(PeHandler *peHndl, bool isHexFormat, QObject *parent)
	: PeTableModel(peHndl, parent),
	showHex(isHexFormat),
	startOff(0), endOff(0), pageSize(PREVIEW_SIZE),
	addrType(Executable::RAW),
	m_lastRowCount(-1)
{
	connectSignals();
}

void HexDumpModel::onNeedReset()
{
	const int rows = this->rowCount(QModelIndex());
	if (rows != m_lastRowCount) {
		m_lastRowCount = rows;
		m_pendingRegions.clear(); // a full reset supersedes any pending precise update
		reset();
		emit modelUpdated();
		return;
	}
	const int cols = this->columnCount(QModelIndex());
	if (rows <= 0 || cols <= 0) {
		m_pendingRegions.clear();
		return;
	}
	if (!m_pendingRegions.isEmpty()) {
		const QVector<QPair<offset_t, offset_t> > regions = m_pendingRegions;
		m_pendingRegions.clear();
		for (int i = 0; i < regions.size(); i++) {
			emitRegionChanged(regions[i].first, regions[i].second, rows, cols);
		}
		return;
	}
	// Fallback for changes we could not localize (should rarely hit now):
	emit dataChanged(this->index(0, 0, QModelIndex()), this->index(rows - 1, cols - 1, QModelIndex()));
}

void HexDumpModel::onContentReplaced(offset_t modOffset, bufsize_t modSize)
{
	if (modSize == 0 || modOffset == INVALID_ADDR) return;
	offset_t from = modOffset;
	offset_t to = modOffset + (offset_t)modSize; // exclusive

	// Merge into an existing overlapping or adjacent range.
	for (int i = 0; i < m_pendingRegions.size(); i++) {
		QPair<offset_t, offset_t> &r = m_pendingRegions[i];
		if (from <= r.second && to >= r.first) {
			if (from < r.first)  r.first  = from;
			if (to   > r.second) r.second = to;
			return;
		}
	}
	if (m_pendingRegions.size() >= MAX_PENDING_REGIONS) {
		// Too fragmented: collapse everything into a single bounding range.
		offset_t lo = from, hi = to;
		for (int i = 0; i < m_pendingRegions.size(); i++) {
			if (m_pendingRegions[i].first  < lo) lo = m_pendingRegions[i].first;
			if (m_pendingRegions[i].second > hi) hi = m_pendingRegions[i].second;
		}
		m_pendingRegions.clear();
		m_pendingRegions.append(qMakePair(lo, hi));
		return;
	}
	m_pendingRegions.append(qMakePair(from, to));
}

void HexDumpModel::emitRegionChanged(offset_t from, offset_t to, int rows, int cols)
{
	if (from >= to) return;                         // degenerate / empty range
	if (rows <= 0 || cols <= 0) return;             // nothing to address
	if (startOff == INVALID_ADDR || !m_PE) return;  // model not in a paintable state

	const offset_t pageFrom = startOff;
	const offset_t pageTo   = startOff + (offset_t)rows * HEX_COL_NUM; // grid extent
	const offset_t fileTo   = m_PE->getRawSize();   // real data extent (exclusive)

	// Clamp to the tighter of the visible grid and actual EOF, so trailing
	// past-EOF cells of the final partial row are never addressed.
	const offset_t hi = (pageTo < fileTo) ? pageTo : fileTo;
	if (from < pageFrom) from = pageFrom;
	if (to   > hi)       to   = hi;
	if (from >= to) return;                         // collapsed to zero valid cells -> skip

	const offset_t relFrom = from - startOff;
	const offset_t relLast = (to - 1) - startOff;   // inclusive last byte
	const int firstRow = (int)(relFrom / HEX_COL_NUM);
	const int lastRow  = (int)(relLast / HEX_COL_NUM);

	QModelIndex topLeft, bottomRight;
	if (firstRow == lastRow) {
		topLeft     = this->index(firstRow, (int)(relFrom % HEX_COL_NUM), QModelIndex());
		bottomRight = this->index(firstRow, (int)(relLast % HEX_COL_NUM), QModelIndex());
	} else {
		topLeft     = this->index(firstRow, 0, QModelIndex());
		bottomRight = this->index(lastRow, cols - 1, QModelIndex());
	}
	if (topLeft.isValid() && bottomRight.isValid()) {
		emit dataChanged(topLeft, bottomRight);
	}
}

void HexDumpModel::connectSignals()
{
	// Queued: the repaint must run after the editor commit fully unwinds, otherwise
	// the view tears down the open editor mid-commit and Qt's editor bookkeeping desyncs.
	connect(myPeHndl, SIGNAL(modified()), this, SLOT(onNeedReset()), Qt::QueuedConnection);
	connect(myPeHndl, SIGNAL(marked()), this, SLOT(onNeedReset()), Qt::QueuedConnection);
	// Direct is intentional: the slot only records the changed region (no view work),
	// so it is safe to run synchronously during the commit. The actual dataChanged is
	// emitted later, from the queued onNeedReset() above.
	connect(myPeHndl, SIGNAL(contentReplaced(offset_t, bufsize_t)),
		this, SLOT(onContentReplaced(offset_t, bufsize_t)));
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

	return QChar(contentPtr[0]);
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
	const BYTE val = (*contentPtr);
	if (showHex) {
#if QT_VERSION >= 0x050000
		return QString().asprintf("%02X", val);
#else
		return QString().sprintf("%02X", val);
#endif
	}
	
	const QChar c(val);
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
