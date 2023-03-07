#include "CommentHandler.h"
#include <algorithm>

#define START_ID 1
#define DELIMITER ';'
#define MAX_COMMENTLINE 0xFF

//--------------------
void CommentHandler::LoaderThread::breakReading()
{
	QMutexLocker locker(&m_readMutex);
	isBreakReading = true;
}

bool CommentHandler::_preprocessFile(QString &path, std::map<offset_t, QString> &all_comments)
{
	QFile fIn(path);
	if (fIn.open(QFile::ReadOnly | QFile::Text) == false)  {
		return false;
	}
	
	QTextStream sIn(&fIn);
	while (!sIn.atEnd()) {
		
		QString line = sIn.readLine();
		
		int delimOffset = line.indexOf(DELIMITER);
		if (delimOffset == (-1)) continue; // delimiter not found
		
		QString offsetStr = line.left(delimOffset);

		bool isOk = false;
		offset_t offset = offsetStr.toLongLong(&isOk, 16);
		if (!isOk) continue;
		
		QString comment = line.mid(delimOffset + 1);
		all_comments[offset] = comment;
	}
	fIn.close();
	return true;
}

void CommentHandler::LoaderThread::loadFromQFile(QString path)
{
	std::map<offset_t, QString> all_comments;
	
	if (!_preprocessFile(path, all_comments)) {
		return;
	}
	
	std::map<offset_t, QString>::iterator itr;
	
	for (itr = all_comments.begin(); itr != all_comments.end(); ++itr) {
		
		QMutexLocker locker(&m_readMutex);
		if (isBreakReading) break;
		
		QString &line = itr->second;
		offset_t offset = itr->first;
		
		cmntHndl->_insertComment(offset, line);
	}
}

void CommentHandler::LoaderThread::run()
{
	isBreakReading = false;
	loadFromQFile(this->fPath);
	//printf("thread stopped\n");
}

//--------------------

void CommentHandler::clear()
{
	if (t != NULL) { 
		t->breakReading();
		while (t->isFinished() == false) {
			printf("Waiting to finish...\n");
			t->wait();
		}
	}
	delete this->t;
	this->t = NULL;

	size_t entriesCount = this->commentsVec.size();
	for (int i = 0; i < entriesCount; i++) {
		delete this->commentsVec[i];
	}
	this->commentsVec.clear();
	this->commentsMap.clear();
}

QString CommentHandler::getCommentAt(offset_t rva)
{
	QMutexLocker ml(&m_loadMutex);

	if (this->commentsMap.find(rva) == this->commentsMap.end()) {
		return "";
	}
	return this->commentsMap.at(rva)->content;
}

void CommentHandler::setComment(offset_t rva, QString comment)
{
	QMutexLocker ml(&m_loadMutex);
	_insertComment(rva, comment);
	emit commentsUpdated();
}

bool CommentHandler::_updateComment(offset_t rva, QString &comment)
{
	std::map<offset_t, Comment*>::iterator itr = this->commentsMap.find(rva);

	if (itr == this->commentsMap.end()) {
		return false;
	}
	if (comment.length() == 0) {
		Comment *cmnt = itr->second;
		std::vector<Comment*>::iterator vecItr = std::find(this->commentsVec.begin(), commentsVec.end(), cmnt);
		this->commentsVec.erase(vecItr);
		this->commentsMap.erase(itr);
		delete cmnt; // delete comment

	} else {
		itr->second->content = comment;
	}
	return true;
}

void CommentHandler::_insertComment(offset_t rva, QString &comment)
{
	if (_updateComment(rva, comment)) {
		return; //updated
	}
	
	if (comment.length() == 0) {
		return; // do not add empty tags
	}
	// create a new comment
	Comment* commentVal = new Comment(rva, comment);
	if (commentsVec.size() == 0) {
		commentVal->id = START_ID;
	} else {
		commentVal->id = commentsVec.back()->id + 1;
	}
	commentsVec.push_back(commentVal);
	commentsMap[rva] = commentVal;
}

bool CommentHandler::saveToFile(QString fileName)
{
	QMutexLocker ml(&m_loadMutex);

	if (isLoaded == false && this->commentsVec.size() == 0) {
		return true;
	}

	QFile fOut(fileName);
	if (fOut.open(QFile::WriteOnly| QFile::Text) == false) {
		return false;
	}
	QTextStream out(&fOut);

	std::vector<Comment*>::iterator vecItr;
    for (vecItr = this->commentsVec.begin(); vecItr != this->commentsVec.end(); ++vecItr ) {
		Comment* cmnt = *vecItr;
		QString offsetStr = QString::number(cmnt->offset, 16);
		QString qComment = cmnt->content;
		qComment = qComment.trimmed();
		
		QString line = offsetStr + DELIMITER + qComment;
		out << line << Qt::endl;
	}
	fOut.close();
	return true;
}

size_t CommentHandler::getCommentsNum()
{
	QMutexLocker ml(&m_loadMutex);

	size_t size = this->commentsMap.size();
	return size;
}

void CommentHandler::onLoaded()
{
	// delete loader thread:
	delete this->t;
	this->t = NULL;
	emit commentsUpdated();
}

bool CommentHandler::isFileOk(QString fileName)
{
	QFile fIn(fileName);
	if (fIn.open(QFile::ReadOnly | QFile::Text) == false) return false;
	fIn.close();
	return true;
}

bool CommentHandler::loadFromFile(QString fileName)
{
	if (this->t != NULL) return false;
	if (isFileOk(fileName) == false) return false;

	clear(); // clear all before load!

	this->t = new LoaderThread(fileName, this);
	QObject::connect(t, SIGNAL(finished()), this, SLOT(onLoaded()));
	t->start();
	return true;
}

//--------------------
