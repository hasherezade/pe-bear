#pragma once

#include <QtCore>
#include <bearparser/bearparser.h>

///----

class CollectorThread : public QThread
{
	Q_OBJECT
public:
	CollectorThread(ByteBuffer* buf)
		: m_buf(buf), stopRequested(false)
	{
		m_buf->addRef();
	}

	~CollectorThread()
	{
		ByteBuffer::release(m_buf);
	}

	bool isByteArrInit() { return (m_buf && m_buf->getContent()); }
	
	void stop()
	{
		QMutexLocker lock(&stopMutex);
		stopRequested = true;
	}
	
	bool isStopRequested()
	{
		QMutexLocker lock(&stopMutex);
		return stopRequested;
	}
	
protected:
	ByteBuffer* m_buf;
	QMutex myMutex;
	QMutex stopMutex;
	bool stopRequested;
};

//---

class CollectorThreadManager : public QObject
{
	Q_OBJECT

public:
	CollectorThreadManager() : QObject(), myThread(nullptr), isQueued(false)
	{
	}
	
	~CollectorThreadManager()
	{
		deleteThread();
	}
	
	void stopThread()
	{
		if (this->myThread) {
			this->myThread->stop();
		}
	}
	
	void deleteThread()
	{
		if (this->myThread) {
			this->myThread->stop();
			while (myThread->isFinished() == false) {
				myThread->wait();
			}
			delete myThread;
			myThread = nullptr;
		}
	}
	
	bool recreateThread()
	{
		if (this->myThread) {
			this->myThread->stop();
			isQueued = true;
			return false; //previous thread didn't finished
		}
		if (setupThread()) {
			runThread();
			return true;
		}
		return false;
	}
	
	
protected slots:
	bool resetOnFinished()
	{
		if (myThread && myThread->isFinished()) {
			delete myThread;
			myThread = nullptr;
		}
		if (isQueued) {
			if (setupThread()) {
				runThread();
				return true;
			}
		}
		return false;
	}
	
protected:
	virtual bool setupThread() = 0;
	
	virtual void runThread()
	{
		if (!myThread) return;
		QObject::connect(myThread, SIGNAL(finished()), this, SLOT(resetOnFinished()));
		if (!myThread->isStopRequested()) {
			myThread->start();
		}
		this->isQueued = false;
	}
	
	bool isQueued;
	CollectorThread *myThread;
	QMutex myMutex;
};

///----

