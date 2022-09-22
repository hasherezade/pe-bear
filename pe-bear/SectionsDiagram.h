#pragma once
#include <QtGlobal>
#include <vector>

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

	size_t unitsOfSection(int index, bool isRaw, bool showMapped);
	double percentFilledInSection(int index, bool isRaw, bool showMapped);
	
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

class SectionsDiagramSettings : public QObject
{
	Q_OBJECT

signals:
	void settingsUpdated();

public:
	SectionsDiagramSettings(bool _isRaw, QObject *parent)
		: QObject(parent),
		isRaw(_isRaw),
		showMapped(true),
		isGridEnabled(false), isDrawEPEnabled(true), isDrawSecHdrsEnabled(true), isDrawOffsets(true), isDrawSecNames(true),
		isDrawSelected(false)
	{
		loadDefaultColors();

		bgColor = QColor(DIAGRAM_BG);
		selectionColor = QColor(DIAGRAM_BG);
		selectionColor.setAlpha(150);
	}

public slots:
	void setEnableGrid(bool enable) { isGridEnabled = enable; refreshParent(); }
	void setDrawEP(bool enable) { isDrawEPEnabled = enable; refreshParent(); }
	void setDrawSecHdrs(bool enable) { isDrawSecHdrsEnabled = enable; refreshParent(); }
	void setDrawOffsets(bool enable) { isDrawOffsets = enable;  refreshParent(); }
	void setDrawSecNames(bool enable) { isDrawSecNames = enable; refreshParent(); }
	void setShowMapped(bool enable) { showMapped = enable; refreshParent(); }

protected:
	void loadDefaultColors()
	{
		colors.push_back(QColor(0, 0, 255, 100));
		colors.push_back(QColor(255, 255, 0, 100));
		colors.push_back(QColor(25, 255, 0, 100));
		colors.push_back(QColor(255, 34, 0, 100));
		colors.push_back(QColor(200, 0, 255, 100));
	}

	void refreshParent()
	{
		emit settingsUpdated();
	}

	bool isRaw;
	bool showMapped;
	bool isGridEnabled;
	bool isDrawEPEnabled;
	bool isDrawSecHdrsEnabled;
	bool isDrawOffsets;
	bool isDrawSecNames;
	bool isDrawSelected;

	std::vector<QColor> colors;
	QColor bgColor, selectionColor;

	friend class SectionsDiagram;
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

public slots:
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

	QAction *dataViewAction;
	QAction *enableGridAction, *enableDrawEPAction, *enableDrawSecHdrsAction;
	QAction *enableOffsetsAction, *enableSecNamesAction;
	QMenu menu;

	SecDiagramModel *myModel;
	QPixmap pixmap;
	int curZoom;
	SectionsDiagramSettings settings;
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
