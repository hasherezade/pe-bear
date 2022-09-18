#pragma once

#include <stack>
#include <QtGlobal>
#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include <QStyledItemDelegate>

#include "REbear.h"
#include "base/PeHandlersManager.h"
#include "PEFileTreeModel.h"

#include "gui_base/ExtTableView.h"

class MenuHeader : public QHeaderView
{
	Q_OBJECT
public:
	MenuHeader(QWidget *parent);

	virtual void setTableModel(QAbstractItemModel *model)
	{
		this->tableModel = model;
	}
	
	QMenu defaultMenu;

public slots:
	virtual void customMenuEvent(QPoint p);
	
	void copyOffset()
	{
		QMimeData *mimeData = new QMimeData;
		mimeData->setText(QString::number(selectedOffset, 16));
		QApplication::clipboard()->setMimeData(mimeData);
	}

protected:
	void mousePressEvent(QMouseEvent *event);
	
	QAbstractItemModel *tableModel;
	QAction *copyAction;
	
	offset_t selectedOffset;
};


class HexCompareView : public ExtTableView
{
	Q_OBJECT
	
public:
	HexCompareView(QWidget *parent = 0);
	
	virtual void setModel(QAbstractItemModel *model) override
	{
		ExtTableView::setModel(model);
		if (vHdr) {
			vHdr->setTableModel(model);
		}
	}

protected:
	void init();
	void initHeader();
	
	MenuHeader *vHdr;
};



