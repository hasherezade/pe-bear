#pragma once

#include <stack>
#include <QtGlobal>

#include "QtCompat.h"
#include "REbear.h"
#include "base/PeHandlersManager.h"
#include "PEFileTreeModel.h"

#include "HexDumpModel.h"

class OffsetHeader : public QHeaderView
{
	Q_OBJECT
public:
	OffsetHeader(QWidget *parent);
	virtual void setHexModel(HexDumpModel *model) { this->hexModel = model; }

	QMenu defaultMenu;

public slots:
	virtual void customMenuEvent(QPoint p);
	void followOffset();
	void copyOffset();
	offset_t getSelectedOffset();

protected:
	void mousePressEvent(QMouseEvent *event);

	HexDumpModel *hexModel;
	QAction *followAction;
	QAction *copyAction;

	Executable::addr_type selectedType;
	offset_t selectedOffset;
};
