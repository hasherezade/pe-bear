#pragma once

#include <QtGui>

#include "PeTreeView.h"

//--------------------------------------------------------------------------

class FollowablePeTreeView : public PeTreeView, public MainSettingsHolder
{
	Q_OBJECT

public:
	FollowablePeTreeView(QWidget *parent);
	virtual ~FollowablePeTreeView() { }

public slots:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void setFollowOnClick(bool isEnabled);

protected slots:
	void onSettingsChanged();
	virtual void customMenuEvent(QPoint p);
	void followOffset();

protected:
	bool hasAnyActionEnabled();

	QAction *followOffsetAction, *onClick;

	offset_t selectedOffset;
	Executable::addr_type addrType;
};
