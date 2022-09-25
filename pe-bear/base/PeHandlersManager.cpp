 #include <new>

#include "PeHandlersManager.h"
#include <bearparser/bearparser.h>

#include "PeHandlerFactory.h"

#include <iostream>

//--------------------------------------------------------------------

PeHandlersManager::PeHandlersManager()
{
	addSupportedType(ExeFactory::PE, new PeHandlerFactory(*this, ExeFactory::PE));
}

bool PeHandlersManager::addSupportedType(ExeFactory::exe_type type, ExeHandlerFactory *factory)
{
	if (isSupportedType(type)) {
		//already supported
		return false;
	}
	this->supportedTypes[type] = factory;
	return true;
}

bool PeHandlersManager::isSupportedType(ExeFactory::exe_type type)
{
	if (this->supportedTypes.find(type) == this->supportedTypes.end()) {
		return false;
	}
	return true;
}


bool PeHandlersManager::openExe(QString path, ExeFactory::exe_type type, bool canTruncate)
{
	if (this->isSupportedType(type) == false) return false;
	this->supportedTypes[type]->addHandler(path, canTruncate);
	return true;
}

PeHandlersManager::~PeHandlersManager()
{
	clear();
	std::map<ExeFactory::exe_type, ExeHandlerFactory*>::iterator stItr;
	for (stItr = this->supportedTypes.begin(); stItr != this->supportedTypes.end(); ++stItr) {
		ExeHandlerFactory* factory = stItr->second;
		delete factory;
	}
	this->supportedTypes.clear();
}

bool PeHandlersManager::insertHandler(PeHandler* hndl)
{
	if (hndl == NULL) return false;
	const QString fileName = hndl->getFullName();

	{ //lock:
		QMutexLocker locker(&this->m_loadMutex);
		if (this->nameToHandlerMap[fileName] != NULL) {
			//printf("Already exist : %s\n", fileName.toStdString().c_str());
			hndl->release();
			return false;
		}
		PEFile *pe = hndl->getPe();
		this->PeHandlers[pe] = hndl;
		this->nameToHandlerMap[fileName] = hndl;
	} //unlock

	emit exeHandlerAdded(hndl);
	//---
	emit PeListUpdated();
	return true;
}

PeHandler* PeHandlersManager::getPeHandler(PEFile* pe)
{
	if (pe == NULL) return NULL;

	//lock:
	QMutexLocker locker(&this->m_loadMutex);
	PeHandler* hndl = NULL;
	if (this->PeHandlers.find(pe) == this->PeHandlers.end()) {
		return NULL;
	}
	hndl = this->PeHandlers[pe];
	return hndl;
}

PeHandler* PeHandlersManager::getByName(QString name)
{
	//lock:
	QMutexLocker locker(&this->m_loadMutex);
	if (nameToHandlerMap.find(name) != nameToHandlerMap.end()) {
		return nameToHandlerMap[name]; 
	}
	return NULL;
}

bool PeHandlersManager::removePe(PEFile* pe)
{
	if (!pe) return false;

	PeHandler* hndl = getPeHandler(pe);
	if (!hndl) return false;

	emit exeHandlerRemoved(hndl);
	{ //lock:
		QMutexLocker locker(&this->m_loadMutex);
		const QString peName = hndl->getFullName();

		// get handler :
		std::map<PEFile*, PeHandler*>::iterator peHndlItr = this->PeHandlers.find(pe);
		if (peHndlItr != this->PeHandlers.end()) {
			/* delete from PE -> Handler map */
			this->PeHandlers.erase(peHndlItr);
		}

		/* delete from name -> Handler map */
		nameToHandlerMap.remove(peName);

		//printf("PE handler release counter: %d\n", hndl->getRefCntr());
		hndl->release();
	} //unlock
	//---
	//emit exeHandlerRemoved(hndl);
	emit PeListUpdated();
	return true;
}

void PeHandlersManager::clear()
{
	{ //lock:
		QMutexLocker locker(&this->m_loadMutex);

		std::map<PEFile*, PeHandler*>::iterator histItr;
		for ( histItr = PeHandlers.begin(); histItr != PeHandlers.end(); ++histItr) {
			PeHandler* hndl = histItr->second;
			hndl->release();
		}
		PeHandlers.clear();
		nameToHandlerMap.clear();
	}//unlock
	//---
	emit PeListUpdated();
}

void PeHandlersManager::checkAllSignatures()
{
	std::map<PEFile*, PeHandler*> &map = PeHandlers;
	std::map<PEFile*, PeHandler*>::iterator peIter;
	bool foundAny = false;
	
	{ //lock:
		QMutexLocker locker(&this->m_loadMutex);
		for (peIter = map.begin(); peIter != map.end(); ++peIter) {
			PEFile *pe = peIter->first;
			PeHandler* hndl = peIter->second;
			if (hndl->findPackerSign(pe->getEntryPoint(), Executable::RVA, sig_ma::FIXED) != NULL) {
				foundAny = true;
			}
		}
	} //unlock

	if (foundAny) {
		emit matchedSignatures();
	}
}

