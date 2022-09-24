#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include <bearparser/bearparser.h>

#include "../../base/PeHandlersManager.h"
#include "../../gui_base/FollowablePeTreeView.h"
#include "../../gui_base/OffsetDependentAction.h"
#include "../../gui/CommentView.h"

//----------------------------------------------------

class SignaturesBrowseModel : public QAbstractTableModel
{
	Q_OBJECT

signals:
	void modelUpdated();

protected slots:
	virtual void onNeedReset() { reset(); emit modelUpdated(); }

public:
	SignaturesBrowseModel(sig_ma::SigFinder *signs, QObject *parent = 0);
	
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	
	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	int rowCount(const QModelIndex &parent) const;//{ return INFO_COUNTER; }

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int) { return false; }

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const { return createIndex(row, column); } //no index item pointer
	QModelIndex parent(const QModelIndex &index) const { return QModelIndex(); } // no parent

	enum COLS {
		COL_ID = 0,
		COL_NAME,
		COL_SIZE,
		MAX_COL
	};

protected:
	void reset()
	{
		//>
		this->beginResetModel();
		this->endResetModel();
		//<
	}

	sig_ma::SigFinder *signs;
};

//----------------------------------------------------

class SignaturesBrowseWindow : public QMainWindow
{
    Q_OBJECT

signals:
	void signaturesUpdated();

public slots:
	void openSignatures();

public:
	SignaturesBrowseWindow(sig_ma::SigFinder *vSign, QWidget *parent);

private:
	void createMenu();

	QTreeView signsTree;
	sig_ma::SigFinder* vSign;
};

