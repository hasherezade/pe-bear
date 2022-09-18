#pragma once

#include "PeHandlersManager.h"

class PeHandlerFactory : public ExeHandlerFactory
{
	Q_OBJECT
public :
	PeHandlerFactory(PeHandlersManager &manager, ExeFactory::exe_type type, bool canTruncate = true)
		: ExeHandlerFactory(manager, type, canTruncate) {}

	virtual bool addHandler(QString path, bool canTruncate);

protected:
	virtual PEFile* makePeFile(FileBuffer* buffer);
};
