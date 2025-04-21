#pragma once

#include <QtCore>
#include <bearparser/bearparser.h>

///----

class CollectorThread : public QThread
{
	Q_OBJECT
public:
	CollectorThread(AbstractByteBuffer* inpBuf)
		: m_buf(nullptr), stopRequested(false)
	{
		if (inpBuf && inpBuf->getContent()) {
			m_buf = new ByteBuffer(inpBuf->getContent(), inpBuf->getContentSize());
		}
	}

	~CollectorThread()
	{
		if (m_buf) {
			delete m_buf;
		}
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
		bool isOk = false;
		try {
			if (setupThread()) {
				runThread();
				isOk = true;
			}
		} catch (const BufferException&) {
			isOk = false;
		}
		return isOk;
	}
	
	
protected slots:
	bool resetOnFinished()
	{
		if (myThread && myThread->isFinished()) {
			delete myThread;
			myThread = nullptr;
		}
		bool isOk = false;
		try {
			if (isQueued) {
				if (setupThread()) {
					runThread();
					isOk = true;
				}
			}
		}
		catch (const BufferException&) {
			isOk = false;
			isQueued = false; // failed to run, remove from the queue
		}
		return isOk;
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

