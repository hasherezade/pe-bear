#pragma once

#include <QtCore>
#include <stack>

#include <bearparser/bearparser.h>

class ModifBackup
{
public:

	ModifBackup()
		: buffer(NULL), offset(INVALID_ADDR), size(0) {}

	ModifBackup(AbstractByteBuffer *fileBuffer, offset_t modOffset, bufsize_t modSize);
	~ModifBackup();

	bufsize_t getSize() { return size; }
	offset_t getOffset() { return offset; }

	virtual bool apply(AbstractByteBuffer* file);
	bool isOffsetAffected(offset_t offset);

protected:
	void _storePatchContent(AbstractByteBuffer* fileBuf, offset_t modOffset, bufsize_t modSize);
	bool _applyPatchContent(AbstractByteBuffer* fileBuf);
	
	BYTE* buffer;
	bufsize_t size;
	offset_t offset;

friend class ModificationHandler;
};

//---

class ResizeBackup : public ModifBackup
{
public:
	ResizeBackup(AbstractByteBuffer *fileBuffer, bufsize_t newSize);
	virtual bool apply(AbstractByteBuffer* file);

protected:
	bufsize_t fullSize; // saved size of the original fileBuffer before the resize
};

//---

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

//---

class ModificationHandler : public QObject
{
	Q_OBJECT
public:
	ModificationHandler(AbstractByteBuffer* fileBuffer, QObject* parent);
	~ModificationHandler();

	bool backupModification(offset_t modifOffset, bufsize_t modifSize, bool continueLastOperation);
	bool backupResize(bufsize_t newSize, bool continueLastOperation);
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
	bool _backupModification(ModifBackup *newModif, bool continueLastOperation);
	
	/* Creates operation with single modification. Throws a CustomException of error. */
	bool store(ModifBackup* modif);

	/* stores already prepared operation */
	bool storeOperation(OperationBackup* backup)
	{
		if (!backup) return false;
		this->modifs.push(backup);
		return true;
	}

	OperationBackup* getLastOperation();
	ModifBackup* getLastModif();

	AbstractByteBuffer* file;
	std::stack< OperationBackup* > modifs;
};
