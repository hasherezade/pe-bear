#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "base/PeHandlersManager.h"

class ExeDependentAction : public QAction
{
	Q_OBJECT

signals:
	void currHndlChanged();

public slots:
	void onHandlerSelected(PeHandler *hndl);

public :
	ExeDependentAction(QObject *parent) : QAction(parent), currHndl(NULL) { init(); }
	ExeDependentAction(const QString text, QObject *parent) : QAction(text, parent), currHndl(NULL) { init(); }
	ExeDependentAction(const QIcon ico, const QString text, QObject *parent) : QAction(ico, text, parent), currHndl(NULL) { init(); }

signals:
    void triggered(PeHandler*);

private slots:
	void onTriggered() { emit triggered(currHndl); }

protected:
	void init();
	void updateEnabled();

	PeHandler *currHndl;
};

class ExeDependentMenu : public QMenu
{
	Q_OBJECT

signals:
	void handlerSet(PeHandler* hndl);

protected slots:
	void onExeChanged(PeHandler* hndl);

public:
	ExeDependentMenu(QWidget *parent = 0) : QMenu(parent),  myHndl(NULL) {}
	virtual ~ExeDependentMenu() { }

	void addAction(QAction *action);
	void removeAction(QAction *action);

protected:
	PeHandler* myHndl;
};
