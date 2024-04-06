#pragma once
#include <QtGlobal>

#include "../QtCompat.h"
#include "../gui_base/FollowablePeTreeView.h"
#include "../gui_base/WrapperTableModel.h"

class WrapperTreeView : public FollowablePeTreeView
{
	Q_OBJECT
signals:
	void parentIdSelected(size_t parentId);
public:
	WrapperTreeView(QWidget *parent) : FollowablePeTreeView(parent) {}
	void selectionChanged(const QItemSelection &newSel, const QItemSelection &prevSel);
};
