#include "OffsetsBrowseWindow.h"
#include <bearparser/Util.h>

//-------------------------

QVariant OffsetsBrowseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	if (orientation == Qt::Horizontal) {
		switch (section) {
			case COL_OFFSET : return "RVA";
			case COL_ID : return "ID";
			case COL_NAME : return "Comment";
		}
	}
	return QVariant();
}

Qt::ItemFlags OffsetsBrowseModel::flags(const QModelIndex &index) const
{	
	if (!index.isValid()) return Qt::NoItemFlags;
	Qt::ItemFlags fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if (index.column() == COL_NAME) return fl | Qt::ItemIsEditable;
	return fl;
}

int OffsetsBrowseModel::rowCount(const QModelIndex &parent) const 
{
	CommentHandler &cHndl = myPeHndl->comments;
	size_t num = cHndl.commentsVec.size();
	return num;
}

QVariant OffsetsBrowseModel::data(const QModelIndex &index, int role) const
{
	if (myPeHndl == NULL) return QVariant();
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	int row = index.row();
	int column = index.column();
	
	CommentHandler &cHndl = myPeHndl->comments;
	if (row >= cHndl.commentsVec.size()) return QVariant();

	Comment *cmnt = cHndl.commentsVec[row];
	if (cmnt == NULL) return QVariant();

	if (role == Qt::BackgroundRole && column == COL_OFFSET) {
		Executable::addr_type aT = addrTypeAt(index);
		if (m_PE->toRaw(cmnt->offset, aT) == INVALID_ADDR) return this->errColor;
	}
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole) return QVariant();

	switch (column) {
			case COL_OFFSET :
				return QString::number(cmnt->offset, 16);
			case COL_ID : 
				return cmnt->id;
			case COL_NAME : 
				return cmnt->content;
	}
	return QVariant();
}

bool OffsetsBrowseModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (myPeHndl == NULL) return false;

	int row = index.row();
	int column = index.column();
	if (column != COL_NAME) return false;

	Comment *cmnt = myPeHndl->comments.commentsVec[row];
	if (cmnt == NULL) return false;

	myPeHndl->comments.setComment(cmnt->offset, value.toString());
	return true;
}

offset_t OffsetsBrowseModel::getFieldOffset(QModelIndex index) const
{
	if (myPeHndl == NULL) return INVALID_ADDR;

	CommentHandler &cHndl = myPeHndl->comments;
	Comment *cmnt = cHndl.commentsVec[index.row()];
	if (cmnt == NULL) return INVALID_ADDR;

	return m_PE->toRaw(cmnt->offset, Executable::RVA);
}

bufsize_t OffsetsBrowseModel::getFieldSize(QModelIndex index) const
{
	return 1;
}
//------------------------------------------------------------------------------------

OffsetsBrowseWindow::OffsetsBrowseWindow(PeHandler *peHndlr, QWidget *parent)
    : QMainWindow(parent), tagsTree(this), commentsView(NULL), tagModel(NULL)
{
	if (peHndlr == NULL) return;
	this->commentsView = new CommentView(peHndlr, this);

	setAcceptDrops(true);

	QPixmap tagIco(":/icons/star.ico");
	setWindowIcon(tagIco);
	setWindowTitle("Tags of ["+ peHndlr->getFullName()+"]");

	createMenu();
	this->tagModel = new OffsetsBrowseModel(peHndlr, this);
	tagsTree.setModel(tagModel);
	//tagsTree.setSortingEnabled(true);
	setCentralWidget(&tagsTree);

	CommentHandler &cHndl = peHndlr->comments;
	connect(&cHndl, SIGNAL(commentsUpdated()), tagModel, SLOT(onNeedReset()));
}

OffsetsBrowseWindow::~OffsetsBrowseWindow()
{
	delete this->tagModel;
	delete this->commentsView;
}

void OffsetsBrowseWindow::createMenu()
{
	// tag menu
	QMenu* tagSubmenu = menuBar()->addMenu("File");

	QAction* loadCommentAction = new QAction("Load", tagSubmenu);
	connect(loadCommentAction, SIGNAL(triggered()), this, SLOT(onLoadComment()));
	tagSubmenu->addAction(loadCommentAction);
	
	QAction* saveCommentAction = new QAction("Save", tagSubmenu);
	connect(saveCommentAction, SIGNAL(triggered()), this, SLOT(onSaveComment()));
	tagSubmenu->addAction(saveCommentAction);
}

void OffsetsBrowseWindow::dropEvent(QDropEvent* ev) 
{
	if (!commentsView) return;

    QList<QUrl> urls = ev->mimeData()->urls();
	QList<QUrl>::Iterator urlItr;
	QCursor cur = this->cursor();
	this->setCursor(Qt::BusyCursor);

	bool loaded = false;
	//try to load any from tre list, till the first success
	for (urlItr = urls.begin() ; urlItr != urls.end(); urlItr++) {
        QString fName = urlItr->toLocalFile();
		if (commentsView->loadComments(fName) == true) {
			loaded = true;
			break;
		}
    }
	this->setCursor(cur);

	if (loaded == false) {
		QMessageBox::warning(this, "Failed", "Loading failed!", QMessageBox::Ok);
	}
}
