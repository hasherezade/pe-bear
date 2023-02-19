#include "SignaturesBrowseWindow.h"

#include <bearparser/Util.h>

using namespace std;


SignaturesBrowseModel::SignaturesBrowseModel(sig_ma::SigFinder *signs, QObject *parent)
	: QAbstractTableModel(parent)
{
	this->signs = signs;
}

QVariant SignaturesBrowseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	if (orientation == Qt::Horizontal) {
		switch (section) {
			case COL_ID: return "ID";
			case COL_NAME : return "Name";
			case COL_SIZE: return "Size";
			case COL_PREVIEW: return "Signature Content Preview";
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
	if (!signs) return 0;
	return this->signs->signaturesVec().size();
}

QVariant SignaturesBrowseModel::data(const QModelIndex &index, int role) const
{
	int row = index.row();
	int column = index.column();
	
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole) return QVariant();

	std::vector<sig_ma::PckrSign*> &vec = this->signs->signaturesVec();

	if ((size_t)row >= vec.size()) return QVariant();

	sig_ma::PckrSign* sign = vec[row];
	if (sign == NULL) return QVariant();

	switch (column) {
		case COL_ID:
			return row;
		case COL_NAME : 
			return QString::fromStdString(sign->getName());
		case COL_SIZE: 
			return (qulonglong)sign->length();
		case COL_PREVIEW:
			return QString::fromStdString(sign->getContent());
	}
	return QVariant();
}

//------------------------------------------------------------------------------------

SignaturesBrowseWindow::SignaturesBrowseWindow(sig_ma::SigFinder* vSign, QWidget *parent)
	: QMainWindow(parent), signsTree(this), vSign(NULL)
{
	if (vSign == NULL) return;
	this->vSign = vSign;
	//---
	this->sigModel = new SignaturesBrowseModel(vSign, this);
	QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
	proxyModel->setSourceModel( sigModel );
	signsTree.setModel( proxyModel ); 
	signsTree.setSortingEnabled(true);

	signsTree.setItemsExpandable(false);
	signsTree.setRootIsDecorated(false);
	topLayout.addWidget(&signsTree);
	topLayout.addWidget(&sigInfo);
	
	QWidget *widget = new QWidget(this);
	widget->setLayout(&topLayout);
	setCentralWidget(widget);
	//---
	createMenu();
	connect(this->sigModel, SIGNAL(modelUpdated()), this, SLOT(onSigListUpdated()));
	connect(this, SIGNAL(signaturesUpdated()), sigModel, SLOT(onNeedReset()));

	this->onSigListUpdated();
}

void SignaturesBrowseWindow::createMenu()
{
	QMenu* fileSubmenu = menuBar()->addMenu("File");
	QAction* loadAction = new QAction("Load", fileSubmenu);
	connect(loadAction, SIGNAL(triggered()), this, SLOT(openSignatures()));
	fileSubmenu->addAction(loadAction);
}

void SignaturesBrowseWindow::onSigListUpdated()
{
	sigModel->reset();
	signsTree.reset();
	
	int sigCount = 0;
	if (vSign) {
		sigCount = vSign->signaturesVec().size();
	}
	sigInfo.setText("Total signatures: " + QString::number(sigCount, 10));
}

void SignaturesBrowseWindow::openSignatures()
{
	QString filter = "Text Files (*.txt);;All Files (*)";
	QString fName= QFileDialog::getOpenFileName(NULL, "Open file with signatures", NULL, filter);
	std::string filename = fName.toStdString();

	if (filename.length() > 0) {
		int i = vSign->loadSignatures(filename);
		emit signaturesUpdated();
		//---
		QMessageBox msgBox;
		msgBox.setText("Added new signatures: " + QString::number(i));
		msgBox.exec();
	}

	//todo: emit -> signatures loaded
}
