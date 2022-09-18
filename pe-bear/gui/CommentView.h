#pragma once
#include <QtCore>
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../gui_base/PeGuiItem.h"
#include "../base/CommentHandler.h"

class CommentView : public QWidget, public PeGuiItem, public MainSettingsHolder
{
	Q_OBJECT

signals:
	void commentModified();

public slots:
	void onSetComment(offset_t offset);
	void onSaveComments();
	void onLoadComments();
public:
	bool loadComments(QString fName);
	CommentView(PeHandler *peHandler, QWidget *parent);

protected:
	CommentHandler *commentHndl;

private:
	QString filter;
};
