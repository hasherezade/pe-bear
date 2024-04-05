#pragma once
#include <QtGlobal>

#include "../PEBear.h"

class TreeCpView : public QTreeView
{
	Q_OBJECT

public:
	TreeCpView(QWidget *parent);
	virtual ~TreeCpView() {}

	void resizeColsToContent();

	void keyPressEvent(QKeyEvent *key);
	void setMenu(QMenu *menu);

	QMenu* getMenu() { return myMenu; }
	void setDefaultMenu() { myMenu = &defaultMenu; }
	void enableMenu(bool enable);
	void removeAllActions();

public slots:
	virtual void copySelected();
	virtual void customMenuEvent(QPoint p);
	
protected:
	QMenu *myMenu;
	QMenu defaultMenu;
};
