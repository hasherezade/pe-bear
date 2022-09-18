#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "gui_base/PeGuiItem.h"

//--------------------------------------------------------------------------------------
class SecDiagramModel : public QObject, public PeViewItem
{
	Q_OBJECT
signals:
	void modelUpdated();

public slots:
	void setSelectedArea(offset_t selectedStart, bufsize_t selectedEnd);

protected slots:
	void onSectionsUpdated() { emit modelUpdated(); }

public:
	SecDiagramModel(PeHandler* peHndl);
	size_t getSecNum();
	bufsize_t getUnitSize(bool isRaw);
	size_t getUnitsNum(bool isRaw);

	size_t unitsOfSection(int index, bool isRaw);
	double percentFilledInSection(int index, bool isRaw);
	
	double unitOfEntryPoint(bool isRaw);
	double unitOfHeadersEnd(bool isRaw);
	double unitOfSectionBegin(int index, bool isRaw);
	DWORD getSectionBegin(int index, bool isRaw);
	double unitOfAddress(uint32_t address, bool isRaw);

	DWORD getEntryPoint(bool isRaw);

	QString nameOfSection(int index);
	int secIndexAtUnit(int unitNum, bool isRaw);

	void selectFromAddress(uint32_t offset);
	SectionHdrWrapper* getSectionAtUnit(int unitNum, bool isRaw);

protected:
	DWORD selectedStart, selectedEnd;
friend class SelectableSecDiagram;
};

//--------------------------------------------------------------------------------------

class SectionsDiagram : public QMainWindow
{
	Q_OBJECT

public:
	SectionsDiagram(SecDiagramModel *model, bool viewRawAddresses, QWidget *parent);
	~SectionsDiagram() { destroyActions(); }

	QSize minimumSizeHint() const;
	QSize sizeHint() const;
	void setBackgroundColor(QColor bgColor);
	
	QColor contourColor;
	bool isDrawSelected;

public slots:
	void setEnableGrid(bool enable) { isGridEnabled = enable; refreshPixmap(); }
	void setDrawEP(bool enable) { isDrawEPEnabled = enable; refreshPixmap(); }
	void setDrawSecHdrs(bool enable) { isDrawSecHdrsEnabled = enable; refreshPixmap(); }
	void setDrawOffsets(bool enable) { isDrawOffsets = enable; this->setMinimumWidth(minimumSizeHint().width()); refreshPixmap(); }
	void setDrawSecNames(bool enable) { isDrawSecNames = enable; this->setMinimumWidth(minimumSizeHint().width()); refreshPixmap(); }

	void showMenu(QPoint p);
	void refreshPixmap();

protected:
	void setModel(SecDiagramModel *model);
	void mouseMoveEvent(QMouseEvent *event);
	void paintEvent(QPaintEvent *);
	void resizeEvent(QResizeEvent *) { refreshPixmap(); }
	void showEvent ( QShowEvent *e) { if (needRefresh) refreshPixmap(); }

	void drawSections(QPainter *painter);
	virtual void drawSelected(QPainter *painter, QRect &rect, int LEFT_PAD, int RIGHT_PAD) { }
	void drawEntryPoint(QPainter *painter, QRect &rect, int LEFT_PAD, int RIGHT_PAD);
	void drawSecHeaders(QPainter *painter, QRect &rect, int LEFT_PAD, int RIGHT_PAD);

	void createActions();
	void destroyActions();

	int unitAtPosY(int y);

	QAction *enableGridAction, *enableDrawEPAction, *enableDrawSecHdrsAction;
	QAction *enableOffsetsAction, *enableSecNamesAction;
	QMenu menu;

	SecDiagramModel *myModel;
	QPixmap pixmap;
	QColor bgColor, selectionColor;
	int curZoom;
	bool isGridEnabled;
	bool isDrawEPEnabled, isDrawSecHdrsEnabled, isDrawOffsets, isDrawSecNames;
	bool isRaw;

	int selY1, selY2;
	bool needRefresh;
};

class SelectableSecDiagram : public SectionsDiagram
{
	Q_OBJECT
public:
	SelectableSecDiagram(SecDiagramModel *model, bool isRawView, QWidget *parent);

protected:
	virtual void drawSelected(QPainter *painter, QRect &rect, int LEFT_PAD, int RIGHT_PAD);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	QCursor upCursor, downCursor;
	QPixmap cursorUpPix, cursorDownPix;
};
