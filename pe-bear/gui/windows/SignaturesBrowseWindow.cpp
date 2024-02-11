#include "SignaturesBrowseWindow.h"

#include <bearparser/Util.h>
#include "MainWindow.h"

using namespace std;
using namespace sig_finder;

SignaturesBrowseModel::SignaturesBrowseModel(std::vector<Signature*>& _signatures, QObject *parent)
	: QAbstractTableModel(parent), signatures(_signatures)
{
}

QVariant SignaturesBrowseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	if (orientation == Qt::Horizontal) {
		switch (section) {
			case COL_ID: return "ID";
			case COL_NAME : return tr("Name");
			case COL_SIZE: return tr("Size");
			case COL_PREVIEW: return tr("Signature Content Preview");
		}
	}
	return QVariant();
}

Qt::ItemFlags SignaturesBrowseModel::flags(const QModelIndex &index) const
{	
	if (!index.isValid()) return Qt::NoItemFlags;
	const Qt::ItemFlags fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	return fl;
}

int SignaturesBrowseModel::rowCount(const QModelIndex &parent) const 
{
	return this->signatures.size();
}

QVariant SignaturesBrowseModel::data(const QModelIndex &index, int role) const
{
	int row = index.row();
	int column = index.column();
	
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole) return QVariant();

	auto &vec = this->signatures;

	if ((size_t)row >= vec.size()) return QVariant();

	Signature* sign = vec[row];
	if (sign == NULL) return QVariant();

	switch (column) {
		case COL_ID:
			return row;
		case COL_NAME : 
			return QString::fromStdString(sign->name);
		case COL_SIZE: 
			return (qulonglong)sign->size();
		case COL_PREVIEW:
			return QString::fromStdString(sign->toByteStr());
	}
	return QVariant();
}

//------------------------------------------------------------------------------------

SignaturesBrowseWindow::SignaturesBrowseWindow(std::vector<Signature*>& _signatures, QWidget *parent)
	: QMainWindow(parent), signsTree(this), signatures(_signatures)
{
	//---
	this->sigModel = new SignaturesBrowseModel(signatures, this);
	this->proxyModel = new SigSortFilterProxyModel(this);

	proxyModel->setSourceModel( sigModel );
	signsTree.setModel( proxyModel ); 
	signsTree.setSortingEnabled(true);

	signsTree.setItemsExpandable(false);
	signsTree.setRootIsDecorated(false);

	filterLabel.setText(tr("Search in columns:"));
	topLayout.addWidget(&filterLabel);
	topLayout.addWidget(&filterEdit);

	topLayout.addWidget(&signsTree);
	topLayout.addWidget(&sigInfo);
	
	QWidget *widget = new QWidget(this);
	widget->setLayout(&topLayout);
	setCentralWidget(widget);
	//---
	createMenu();
	connect(this->sigModel, SIGNAL(modelUpdated()), this, SLOT(onSigListUpdated()));
	connect(this, SIGNAL(signaturesUpdated()), sigModel, SLOT(onNeedReset()));

	connect(&filterEdit, SIGNAL(textChanged(QString)), this, SLOT(onFilterChanged(QString)) );
	
	this->onSigListUpdated();
}

void SignaturesBrowseWindow::onFilterChanged(QString str)
{
	if (!proxyModel) return;
	
	QRegExp regExp(str.toLower(), Qt::CaseSensitive, QRegExp::FixedString);
	proxyModel->setFilterRegExp(regExp);
}

void SignaturesBrowseWindow::createMenu()
{
	QMenu* fileSubmenu = menuBar()->addMenu(tr("File"));
	QAction* loadAction = new QAction(tr("Load"), fileSubmenu);
	connect(loadAction, SIGNAL(triggered()), this, SLOT(openSignatures()));
	fileSubmenu->addAction(loadAction);
}

void SignaturesBrowseWindow::onSigListUpdated()
{
	sigModel->reset();
	signsTree.reset();
	
	int sigCount = signatures.size();
	sigInfo.setText(tr("Total signatures: ") + QString::number(sigCount, 10));
}

void SignaturesBrowseWindow::openSignatures()
{
	MainWindow *myWindow = dynamic_cast<MainWindow *>(this->parent());
	if (!myWindow) return;
	myWindow->openSignatures();
}
