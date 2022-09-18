#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

class ExtTableView : public QTableView
{
	Q_OBJECT

public:
	ExtTableView(QWidget *parent = 0);

	void setMenu(QMenu *menu);
	QMenu* getMenu() { return myMenu; }
	void setDefaultMenu() { myMenu = &defaultMenu; }
	void enableMenu(bool enable);
	void removeAllActions();

protected slots:
	virtual void copySelected() { return copyText("", "\n"); }
	virtual void pasteToSelected() { /* not supported */ }

	virtual void copyText(QString hSeparator, QString vSeperator);
	virtual QString getSelectedText(QString hSeparator, QString vSeperator);
	virtual void customMenuEvent(QPoint p);
	QString errorString() { return lastErrorString; }

protected:
	void init();
	void keyPressEvent(QKeyEvent *kEvent);

	QMenu *myMenu;
	QMenu defaultMenu;
	QString lastErrorString;
};
