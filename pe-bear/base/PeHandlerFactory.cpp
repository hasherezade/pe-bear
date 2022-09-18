 #include <new>
#include "PeHandlerFactory.h"

bool PeHandlerFactory::addHandler(QString path, bool canTruncate)
{
	FileBuffer *fileBuffer = NULL;
	PEFile *newPE = NULL;
	try {
		const size_t peMinSize = sizeof(IMAGE_DOS_HEADER) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER64);
		fileBuffer = new FileBuffer(path, peMinSize, canTruncate);
		newPE = makePeFile(fileBuffer);
		if (!newPE) {
			delete fileBuffer; fileBuffer = NULL;
		}
	} catch (CustomException &e) {
		Logger::append(Logger::D_ERROR, e.what());
		return false;
	}
	if (!newPE) return false;

	PeHandler* hndl = new PeHandler(newPE, fileBuffer);
	bool isInserted = this->myManager.insertHandler(hndl);
	return isInserted;
}

PEFile*  PeHandlerFactory::makePeFile(FileBuffer* fileBuffer)
{
	PEFile *newPE = NULL;
	if (!fileBuffer) return NULL;
	try {
		newPE = new PEFile(fileBuffer);
	}
	catch (CustomException &e) {
		Logger::append(Logger::D_ERROR, e.what());
	}
	return newPE;
}

