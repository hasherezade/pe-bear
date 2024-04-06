#pragma once
#include <QtGlobal>

#include <bearparser/bearparser.h>

#include "../../PEBear.h"
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
	enum COLS {
		COL_ID = 0,
		COL_NAME,
		COL_SIZE,
		COL_PREVIEW,
		MAX_COL
	};
	
	SignaturesBrowseModel(std::vector<sig_finder::Signature*>& _signatures, QObject *parent = 0);
	
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	
	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	int rowCount(const QModelIndex &parent) const;//{ return INFO_COUNTER; }

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int) { return false; }

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const { return createIndex(row, column); } //no index item pointer
	QModelIndex parent(const QModelIndex &index) const { return QModelIndex(); } // no parent

	void reset()
	{
		//>
		this->beginResetModel();
		this->endResetModel();
		//<
	}
	
protected:
	std::vector<sig_finder::Signature*>& signatures;
};

//----------------------------------------------------

class SigSortFilterProxyModel : public QSortFilterProxyModel
{
public:
	SigSortFilterProxyModel(QObject *parent)
		: QSortFilterProxyModel(parent)
	{
	}
	
	bool filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const
	{
		QAbstractItemModel *source = sourceModel();
		if (!source) return false;
		
		for(int i = 0; i < source->columnCount(); i++)
		{
			QModelIndex index = source->index(sourceRow, i, sourceParent);
			if (source->data(index).toString().toLower().trimmed().contains(filterRegularExpression()))
				return true;
		}
		return false;  
	}
};

//----------------------------------------------------

class SignaturesBrowseWindow : public QMainWindow
{
    Q_OBJECT

signals:
	void signaturesUpdated();

public slots:
	void openSignatures();
	void onSigListUpdated();

public:
	SignaturesBrowseWindow(std::vector<sig_finder::Signature*>& signatures, QWidget *parent);
	
private slots:
	void onFilterChanged(QString);

private:
	void createMenu();

	QTreeView signsTree;
	std::vector<sig_finder::Signature*>& signatures;
	SignaturesBrowseModel *sigModel;
	SigSortFilterProxyModel *proxyModel;
	
	QVBoxLayout topLayout;

	QLabel filterLabel;
	QLineEdit filterEdit;
	QLabel sigInfo;
};

