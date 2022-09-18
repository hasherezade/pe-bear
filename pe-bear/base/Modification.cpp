#include "Modification.h"

#include <algorithm>
	
ModifBackup::ModifBackup(AbstractByteBuffer* file, offset_t modOffset, bufsize_t modSize)
	: buffer(NULL), offset(INVALID_ADDR), size(0)
{
	if (!file) throw CustomException("Uninitialized file");
	
	BYTE *content = file->getContent();
	if (!content) throw CustomException("File buffer is NULL!");

	//fetch the file fragment at the offset where the modification will happen:
	BYTE *modPtr = file->getContentAt(modOffset, modSize);
	if (!modPtr) {
		throw CustomException("Could not fetch the content at the offset!");
	}

	//allocate a buffer to store the patch:
	this->buffer = (BYTE*) calloc(modSize, sizeof(BYTE));
	if (!buffer) throw CustomException("Cannot allocate modification buffer!");
	
	this->offset = modOffset;
	this->size = modSize;

	//store the backup in the patch buffer:
	::memcpy(buffer, modPtr, modSize);
}

ModifBackup::~ModifBackup()
{
	if (buffer) free(buffer);
}

bool ModifBackup::apply(AbstractByteBuffer* file)
{
	if (!file) return false;

	BYTE *modPtr = file->getContentAt(this->offset, this->size);
	
	if (!modPtr) {
		std::cerr << "Cannot apply backup at: " << std::hex << this->offset << "  on the given file! Area size mismatch!" << std::endl;
		return false;
	}

	::memcpy(modPtr, this->buffer, this->size);
	return true;
}

bool ModifBackup::isOffsetAffected(offset_t curr_offset)
{
	const offset_t modS = getOffset();
	const offset_t modE = modS + getSize();

	return (curr_offset >= modS && curr_offset < modE);
}
//-------

void OperationBackup::deleteChildren()
{
	std::vector<ModifBackup*>::iterator itr =  modifs.begin();

	while (itr != modifs.end()) {
		ModifBackup* modif = *itr;
		itr++;
		delete modif;
	}
	this->clear();
}

bool OperationBackup::contains(ModifBackup* backup)
{
	if (!backup) return false;
	std::vector<ModifBackup*>::iterator found = find(modifs.begin(), modifs.end(), backup);

	if (found != modifs.end()) {
		return true;
	}
	return false;
}

bool OperationBackup::appendBackup(ModifBackup* backup)
{
	if (!backup) return false;
	if (contains(backup)) return false;

	modifs.push_back(backup);
	return true;
}


bool OperationBackup::removeBackup(ModifBackup* backup) //it is not deleting object!
{
	if (!backup) return false;

	std::vector<ModifBackup*>::iterator found = find(modifs.begin(), modifs.end(), backup);

	if (found != modifs.end()) {
		modifs.erase(found);
		return true;
	}
	return false;
}

size_t OperationBackup::undoOperation(AbstractByteBuffer* file)
{
	size_t undone = 0;

	for (std::vector<ModifBackup*>::iterator itr = modifs.begin(); 
		itr != modifs.end(); 
		++itr)
	{
		ModifBackup* modif = *itr;
		if (!modif) continue;
		if (modif->apply(file)) undone++;
	}
	return undone;
}

bool OperationBackup::isOffsetAffected(offset_t offset)
{
	for (std::vector<ModifBackup*>::iterator itr = modifs.begin();
		itr != modifs.end();
		++itr)
	{
		ModifBackup* modif = *itr;
		if (!modif) continue;
		if (modif->isOffsetAffected(offset)) return true;
	}
	return false;
}
//----

ModificationHandler::ModificationHandler(AbstractByteBuffer* fileBuffer, QObject* parent)
	: QObject(parent)
{
	if (!fileBuffer) throw CustomException("Uninitialized file");
	this->file = fileBuffer;
}

ModificationHandler::~ModificationHandler()
{
	while (modifs.size()) {
		OperationBackup* backup = modifs.top();
		modifs.pop();
		delete backup;
	}
}

bool ModificationHandler::backupModification(offset_t modifOffset, bufsize_t modifSize, bool continueLastOperation)
{
	if (modifOffset == INVALID_ADDR || !modifSize) return false;

	bool isOk = true;
	try {
		OperationBackup *last = this->getLastOperation();
		if (!continueLastOperation || !last) {
			store(modifOffset, modifSize);
		} else {
			isOk = last->appendBackup(new ModifBackup(this->file, modifOffset, modifSize));
		}

	} catch (CustomException &e) {
		std::cerr << "Backup error: " << e.what() << std::endl;
		isOk = false;
	}
	return isOk;
}

void ModificationHandler::store(offset_t modOffset, bufsize_t modSize)
{
	try {
		ModifBackup * modif = new ModifBackup(file, modOffset, modSize);
		OperationBackup * backup = new OperationBackup(modif);
		storeOperation(backup);
	}
	catch (CustomException &e)
	{
		std::cerr << "Backup error: " << e.what() << std::endl;
	}
}

bool ModificationHandler::unStoreLast()
{
	if (modifs.size() == 0) return false;

	OperationBackup* last = modifs.top();
	//TODO... do not remove full if have to destroy only one modification in operation!
	modifs.pop();
	delete last;

	return true;
}

bool ModificationHandler::undoLastOperation()
{
	OperationBackup* op = getLastOperation();
	if (!op) return false;

	size_t undone = 0;
	for (std::vector<ModifBackup*>::iterator itr = op->modifs.begin();
		itr != op->modifs.end();
		++itr)
	{
		ModifBackup* patch = *itr;
		if (!patch) continue;

		if (patch->apply(this->file)) {
			undone++;
		}
	}
	this->unStoreLast();
	return undone ? true : false;
}

OperationBackup* ModificationHandler::getLastOperation()
{
	if (modifs.size() == 0) return NULL;
	return modifs.top();
}

ModifBackup* ModificationHandler::getLastModif()
{
	OperationBackup* op = getLastOperation();
	if (!op) return NULL;

	if (op->modifs.size() == 0) return NULL;

	ModifBackup* mod = op->modifs.back();
	return mod;
}

