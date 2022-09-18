#include "CommentView.h"


//-------------------------------------------------------------------------------

CommentView::CommentView(PeHandler *peHandler, QWidget *parent) 
		: QWidget(parent), PeGuiItem(peHandler), commentHndl(NULL)
{
	if (peHandler) commentHndl = &peHandler->comments;

	this->filter = "Tag files (*.tag);;Text files (*.txt);;All Files (*)";
}

void CommentView::onSetComment(offset_t offset)
{
	bool isOk = false;
	QString prev = this->commentHndl->getCommentAt(offset);
	QString title = "RVA : "+ QString::number(offset, 16).toUpper();
	QString text = QInputDialog::getText(this, title, "Comment", QLineEdit::Normal, prev,  &isOk);
	if (!isOk) return;

	this->commentHndl->setComment(offset, text);
	emit commentModified();
}

void CommentView::onSaveComments()
{
	QFileDialog dialog;
	MainSettings *set = this->getSettings();
	if (set) {
		dialog.setDirectory(set->userDataDir());
	}
	QString fName = dialog.getSaveFileName(NULL, "Save", "", filter);
	if (fName.size() == 0) {
		return;
	}
	if (this->commentHndl->saveToFile(fName) == false) {
		 QMessageBox::warning(this, "Failed", "Saving failed!", QMessageBox::Ok);
	}
}

void CommentView::onLoadComments()
{
	QFileDialog dialog;
	MainSettings *set = this->getSettings();
	if (set) {
		dialog.setDirectory(set->userDataDir());
	}
	QString fName = dialog.getOpenFileName(NULL, "Open", "", filter);
	if (fName.size() == 0) return;

	if (loadComments(fName) == false) {
		 QMessageBox::warning(this, "Failed", "Loading failed!", QMessageBox::Ok);
		 return;
	}
	//---
	/*size_t loadedNum = this->commentHndl->commentsNum();
	QString infoStr = "Loaded " + QString::number(loadedNum) + " tag" + ((loadedNum > 1) ? "s!" : "!");
	QMessageBox::information(this, "Done!", infoStr, QMessageBox::Ok);*/
}

bool CommentView::loadComments(QString fName)
{
	if (this->commentHndl->loadFromFile(fName) == false) {
		return false;
	}
	emit commentModified();
	return true;
}
