#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include <bearparser/bearparser.h>
#include "../ViewSettings.h"

#define HEX_COL_NUM 16
#define COL_NUM (HEX_COL_NUM)
#define COL_WIDTH 20

enum ContentIndx { LEFT = 0, RIGHT = 1, CNTR = 2 };

class HexDiffModel : public QAbstractTableModel
{
	Q_OBJECT

public slots:
	void setRelativeOffset(bool flag) { relativeOffset = flag; reset(); } 
	void setContent(BYTE* contentPtr, int size, offset_t contentOffset, ContentIndx indx);
	void clearContent(ContentIndx indx);
	void setHexView(bool isSet) { showHex = isSet; reset();}
	void setStartingOffset(offset_t start) { startOff = start; reset(); }
	void setPage(const int page) { startOff = pageSize * page; reset(); }

	void onGoToPrevDiff();
	void onGoToNextDiff();

	void changeSettings(HexViewSettings &newSettings) 
	{
		settings = newSettings;
		reset();
	}
	
public:
	static int32_t getDiffStart(BYTE *content1Ptr, bufsize_t size1, BYTE *content2Ptr, bufsize_t size2);
	static int32_t getDiffEnd(BYTE *content1Ptr, bufsize_t size1, BYTE *content2Ptr, bufsize_t size2);
//------
	HexDiffModel(ContentIndx indx, QObject *parent = 0);

	bool isHexView() { return showHex; }
	bufsize_t getPageSize() { return pageSize; };
	bufsize_t getStartOff() { return startOff; }

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int) { return false; } //external modifications not allowed

	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool isRelativeOffset() const { return relativeOffset; }

protected:
	void reset()
	{
		//>
		this->beginResetModel();
		this->endResetModel();
		//<
	}

private:
	bool relativeOffset;
	offset_t contentOffset[CNTR];
	BYTE* contentPtr[CNTR];
	bufsize_t contentSize[CNTR];
	ContentIndx myIndx;

	QColor diffColor;
	QColor limitColor;
	//QColor activeColor;

	bool showHex;
	bufsize_t startOff;
	bufsize_t pageSize;
	
	HexViewSettings settings;
};
