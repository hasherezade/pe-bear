#pragma once

#include <QtCore>
#include <stack>

#include <bearparser/bearparser.h>

class ModifBackup
{
public:
	ModifBackup(AbstractByteBuffer *fileBuffer, offset_t modOffset, bufsize_t modSize);
	~ModifBackup();

	bufsize_t getSize() { return size; }
	offset_t getOffset() { return offset; }

	bool apply(AbstractByteBuffer* file);
	bool isOffsetAffected(offset_t offset);

protected:
	BYTE* buffer;
	bufsize_t size;
	offset_t offset;

friend class ModificationHandler;
};

class OperationBackup
{
public:
	OperationBackup() {}

	OperationBackup(ModifBackup* backup)
	{
		appendBackup(backup);
	}

	~OperationBackup() { deleteChildren(); }

	 /* NOT deleting objects! */
	void clear() { modifs.clear(); }

	 /* deleting objects! */
	void deleteChildren();

	bool contains(ModifBackup* backup);
	bool appendBackup(ModifBackup* backup);

	/* removes from vector, not deleting object! */
	bool removeBackup(ModifBackup* backup);

	/* returns: how many modifications undone, not deleting objects! */
	size_t undoOperation(AbstractByteBuffer* file);
	
	bool isOffsetAffected(offset_t offset);

protected:
	std::vector<ModifBackup*> modifs;

friend class ModificationHandler;
};

class ModificationHandler : public QObject
{
	Q_OBJECT
public:
	ModificationHandler(AbstractByteBuffer* fileBuffer, QObject* parent);
	~ModificationHandler();

	bool backupModification(offset_t modifOffset, bufsize_t modifSize, bool continueLastOperation);

	/* unstore last operation - delete object */
	bool unStoreLast();

	bool undoLastOperation();

	size_t countOperations()
	{
		return modifs.size();
	}

	bool isInLastModifiedArea(offset_t offset)
	{
		OperationBackup* op = this->getLastOperation();
		if (!op) return false;

		return op->isOffsetAffected(offset);
	}

	offset_t getLastModifiedOffset()
	{
		ModifBackup* mod = this->getLastModif();
		if (!mod) {
			return INVALID_ADDR;
		}
		return mod->getOffset();
	}

protected:
	
	/* Creates operation with single modification. Throws a CustomException of error. */
	void store(offset_t modOffset, bufsize_t modSize);

	/* stores already prepared operation */
	void  storeOperation(OperationBackup* backup)
	{
		if (!backup) return;
		this->modifs.push(backup);
	}

	OperationBackup* getLastOperation();
	ModifBackup* getLastModif();

	AbstractByteBuffer* file;
	std::stack< OperationBackup* > modifs;
};
