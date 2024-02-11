#pragma once

#include <QtCore>
#include <stack>

#include "../REbear.h"
#include <bearparser/bearparser.h>

#include "Modification.h"
#include "CommentHandler.h"
#include "PeHandler.h"

#define SIZE_UNLIMITED (-1)
//-------------------------------------------------

class PeHandlersManager;

class ExeHandlerFactory : public QObject 
{
	Q_OBJECT
public :
	ExeHandlerFactory(PeHandlersManager &manager, ExeFactory::exe_type type, bool canTruncate) 
		: QObject(),
		myManager(manager), exeType(type), allowTruncated(canTruncate) {}

	virtual bool addHandler(QString path, bool canTruncate) = 0;
	virtual bool addHandler(QString path) { return addHandler(path, this->allowTruncated); }
	
protected:
	virtual ~ExeHandlerFactory() {} // will be deleted in its manager

	ExeFactory::exe_type exeType;
	PeHandlersManager &myManager;
	bool allowTruncated;

friend class PeHandlersManager;
};

//-------------------------------------------------

class PeHandlersManager : public QObject
{
	Q_OBJECT

signals:
	void exeHandlerAdded(PeHandler*);
	void exeHandlerRemoved(PeHandler*);

	void PeListUpdated();
	void matchedSignatures();

public:
	PeHandlersManager();
	~PeHandlersManager();
	bool addSupportedType(ExeFactory::exe_type, ExeHandlerFactory *factory);

	bool isSupportedType(ExeFactory::exe_type);
	bool openExe(QString path, ExeFactory::exe_type type, bool canTruncate);

	bool insertHandler(PeHandler* hndl);
	PeHandler* getPeHandler(PEFile* pe);
	PeHandler* getByName(QString name);

	bool removePe(PEFile* pe);
	void clear();
	std::map<PEFile*, PeHandler*> &getHandlersMap() { return PeHandlers; }
	QList<QString> getFilenames() { return nameToHandlerMap.keys(); }

public slots:
	void checkAllSignatures();

protected:
	std::map<PEFile*, PeHandler*> PeHandlers;
	QMap<QString, PeHandler*> nameToHandlerMap;
	std::map<ExeFactory::exe_type, ExeHandlerFactory*> supportedTypes;
	QMutex m_loadMutex;
};
