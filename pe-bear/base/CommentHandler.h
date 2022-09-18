#pragma once
#include <QtCore>
#include <bearparser/bearparser.h>
#include <map>
#include <list>

class Comment;
class CommentHandler;

//-------------------

class Comment
{
public:
	Comment(offset_t Offset, QString Content) :
		offset(Offset),
		content(Content) { }

	offset_t offset;
	QString content;
	uint32_t id;
};

//-------------------

class CommentHandler : public QObject
{
	Q_OBJECT
signals:
	void commentsUpdated();

protected slots:
	void onLoaded();

public:
	CommentHandler() : QObject(), isLoaded(false), t(NULL) {}
	~CommentHandler() { clear(); }

	QString getCommentAt(offset_t rva);
	void setComment(offset_t rva, QString comment);
	//---
	bool saveToFile(QString fileName);
	bool loadFromFile(QString fileName);
	size_t commentsNum() { return commentsMap.size(); }

	std::vector<Comment*> commentsVec;
	size_t getCommentsNum();

protected:
	// BEGIN inner class: LoaderThread
	class LoaderThread : public QThread
	{
	public:
		LoaderThread(QString filePath, CommentHandler *handler)
			: cmntHndl(handler), fPath(filePath)
		{
		}
		
		void breakReading();
		
	private:
		void run();
		void loadFromQFile(QString path);

		bool isLoaded;
		QString fPath;
		CommentHandler *cmntHndl;

		bool isBreakReading;
		QMutex m_readMutex;
	};
	// END of inner class: LoaderThread

	static bool isFileOk(QString fileName);
	static bool _preprocessFile(QString &path, std::map<offset_t, QString> &all_comments);

	void _insertComment(offset_t rva, QString &comment);
	bool _updateComment(offset_t rva, QString &comment);
	void clear();

	std::map<offset_t, Comment*> commentsMap;
	bool isLoaded; //indicates if tags are fresh or loaded from file
	QMutex m_loadMutex;
	LoaderThread *t;

//friend class LoaderThread;
};


