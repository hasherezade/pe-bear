#include "HexDiffModel.h"
#include <QtGlobal>
#include <bear_disasm.h>

#define PREVIEW_SIZE 0x200

bufsize_t HexDiffModel::getDiffStart(BYTE *content1Ptr, bufsize_t size1, BYTE *content2Ptr, bufsize_t size2)
{
	if (content1Ptr == NULL && content2Ptr == NULL) return DIFF_NOT_FOUND;
	
	if (content1Ptr == NULL || content2Ptr == NULL) return 0;

	if (content1Ptr == content2Ptr && size1 == size2) return DIFF_NOT_FOUND;
	
	size_t cmpSize = (size1 > size2 )? size2 : size1;
	int cmpRes = memcmp(content1Ptr, content2Ptr, cmpSize);
	if (cmpRes == 0) return DIFF_NOT_FOUND;

	bufsize_t minSize = (size1 < size2) ? size1 : size2;
	for (bufsize_t i = 0; i < minSize; i++) {
		if (content1Ptr[i] != content2Ptr[i]) return i;
	}
	return DIFF_NOT_FOUND;
}

bufsize_t HexDiffModel::getDiffEnd(BYTE *content1Ptr, bufsize_t size1, BYTE *content2Ptr, bufsize_t size2)
{
	if (content1Ptr == NULL && content2Ptr == NULL) return DIFF_NOT_FOUND;
	
	if (content1Ptr == NULL || content2Ptr == NULL) return 0;

	if (content1Ptr == content2Ptr && size1 == size2) return DIFF_NOT_FOUND;
		
	bufsize_t minSize = (size1 < size2) ? size1 : size2;

	for (bufsize_t i = 0; i < minSize; i++) {
		if (content1Ptr[i] == content2Ptr[i]) return i;
	}
	return DIFF_NOT_FOUND;
}


HexDiffModel::HexDiffModel(ContentIndx indx, QObject *parent)
	: QAbstractTableModel(parent), myIndx(indx),
	showHex(true), startOff(0), pageSize(PREVIEW_SIZE), 
	relativeOffset(false)
{
	clearContent(CNTR);

	diffColor = QColor("red");
	diffColor.setAlpha(150);

	limitColor = QColor("yellow");
	limitColor.setAlpha(150);
}

void HexDiffModel::setContent(BYTE* contentPtr, int size, offset_t contentOff, ContentIndx indx)
{
	startOff = 0;
	if (contentPtr == NULL || size == 0) {
		clearContent(indx);
		return;
	}
	if (indx == CNTR) return;
	
	this->contentPtr[indx] = contentPtr;
	this->contentSize[indx] = size;
	contentOffset[indx] = contentOff;
	this->reset();
}

void HexDiffModel::clearContent(ContentIndx indx)
{
	startOff = 0;
	if (indx == CNTR) {
		contentPtr[LEFT] = NULL;
		contentPtr[RIGHT] = NULL;
		
		contentSize[LEFT] = 0;
		contentSize[RIGHT] = 0;

		contentOffset[LEFT] = 0;
		contentOffset[RIGHT] = 0;
	} else {
		contentPtr[indx] = NULL;
		contentSize[indx] = 0;
		contentOffset[indx] = 0;
	}
	this->reset();
}

void HexDiffModel::onGoToNextDiff()
{
	if (contentSize[LEFT] == 0 || contentSize[RIGHT] == 0) return;
	if (contentSize[LEFT] < this->startOff) return;
	if (contentSize[RIGHT] < this->startOff) return;

	offset_t offset = this->startOff;
	int leftSize = contentSize[LEFT] - offset;
	int rightSize = contentSize[RIGHT] - offset;

	bufsize_t diff = HexDiffModel::getDiffStart(
		&contentPtr[LEFT][this->startOff],
		contentSize[LEFT],
		&contentPtr[RIGHT][this->startOff],
		contentSize[RIGHT]
	);
	
	if (diff == DIFF_NOT_FOUND) return;
	
	offset += diff;
	if (diff != 0) {
		setStartingOffset(offset);
		return;
	}

	leftSize = contentSize[LEFT] - offset;
	rightSize = contentSize[RIGHT] - offset;

	diff = HexDiffModel::getDiffEnd(&contentPtr[LEFT][offset], leftSize, &contentPtr[RIGHT][offset], rightSize);

	if (diff == DIFF_NOT_FOUND) return;

	offset += diff;
	leftSize = contentSize[LEFT] - offset;
	rightSize = contentSize[RIGHT] - offset;
	diff = HexDiffModel::getDiffStart(&contentPtr[LEFT][offset], leftSize, &contentPtr[RIGHT][offset], rightSize);

	if (diff != DIFF_NOT_FOUND) {
		offset += diff;
		if (diff != 0)
			setStartingOffset(offset);
	}
}

void HexDiffModel::onGoToPrevDiff()
{
	//TODO....
}

int HexDiffModel::rowCount(const QModelIndex &parent) const
{
	if (this->contentSize[myIndx] < this->startOff)
		return 0;
	int rows = pageSize / HEX_COL_NUM;
	return rows;
}

int HexDiffModel::columnCount(const QModelIndex &parent) const
{
	return COL_NUM;
}

QVariant HexDiffModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::FontRole) {
		QFont hdrFont = settings.myFont;
		hdrFont.setBold(true);
		hdrFont.setItalic(false);
		return hdrFont;
	}
	
	if (orientation == Qt::Horizontal) {
		if (role == Qt::SizeHintRole) {
			int iconDim = ViewSettings::getIconDim(settings.myFont);
			return QSize(iconDim, iconDim);
		}
		
		if (role == Qt::DisplayRole) {
			if (section >= COL_NUM) return QVariant();
			return QString::number(section, 16).toUpper();
		}
		return QVariant();
	}
	
	if (orientation == Qt::Vertical) {
		if (role == Qt::SizeHintRole) {
			return settings.getVerticalSize();
		}
		if (role == Qt::DisplayRole) {
			int offset = startOff + (section * HEX_COL_NUM);
			if (isRelativeOffset() == false) {
				offset += this->contentOffset[myIndx];
			}
			const QString offsetStr = QString::number(offset, 16).toUpper();
			return offsetStr;
		}
		return QVariant();
	}
	return QVariant();
}

Qt::ItemFlags HexDiffModel::flags(const QModelIndex &index) const 
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant HexDiffModel::data(const QModelIndex &index, int role) const
{
	if (index.isValid() == false) return QVariant();

	if (index.column() == COL_NUM) {
		return QVariant();
	}

	int x = index.column();
	int y = index.row() * HEX_COL_NUM;
	size_t indx = (y + x) + startOff;

	if (role == Qt::FontRole) {
		return settings.myFont;
	}
	if (role == Qt::ForegroundRole) {
		return QVariant();
	}
	if (role == Qt::BackgroundRole) {
		ContentIndx otherIndx = (myIndx == LEFT) ? RIGHT : LEFT;
		bufsize_t minSize = (contentSize[LEFT] < contentSize[RIGHT]) ? contentSize[LEFT] : contentSize[RIGHT];
		
		if (indx >= minSize && indx < contentSize[myIndx])
			return limitColor;

		if (indx < minSize && indx < (this->startOff + this->pageSize)) {
			if (this->contentPtr[myIndx][indx] != this->contentPtr[otherIndx][indx])
				return diffColor;
		}
		return QVariant();
	}
	if (role == Qt::DisplayRole) {
		if (indx < contentSize[myIndx]) {
			if (showHex) {
#if QT_VERSION >= 0x050000
				return QString().asprintf("%02X", this->contentPtr[myIndx][indx]);
#else
				return QString().sprintf("%02X", this->contentPtr[myIndx][indx]);
#endif
			} else {
				QChar c = QChar(this->contentPtr[myIndx][indx]);
				
				if (c.isPrint() && !c.isSpace())
					return c;
				return QChar('.');
			}
		}
	}
	return QVariant();
}
