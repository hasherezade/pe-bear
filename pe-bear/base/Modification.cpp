#include "Modification.h"

#include <algorithm>

ModifBackup::ModifBackup(AbstractByteBuffer* fileBuf, offset_t modOffset, bufsize_t modSize)
	: buffer(NULL), offset(INVALID_ADDR), size(0)
{
	if (!fileBuf) throw CustomException("Uninitialized file");
	
	BYTE *content = fileBuf->getContent();
	if (!content) throw CustomException("File buffer is NULL!");

	_storePatchContent(fileBuf, modOffset, modSize);
}

ModifBackup::~ModifBackup()
{
	if (buffer) free(buffer);
}

void ModifBackup::_storePatchContent(AbstractByteBuffer* fileBuf, offset_t modOffset, bufsize_t modSize)
{
	if (modOffset == INVALID_ADDR || modSize == 0) return;
	
	//fetch the file fragment at the offset where the modification will happen:
	BYTE *modPtr = fileBuf->getContentAt(modOffset, modSize);
	if (!modPtr) {
		throw CustomException("Could not fetch the content of size: 0x" + QString::number(modSize, 16) + " at the offset: 0x" + QString::number(modOffset, 16) );
		return;
	}

	//allocate a buffer to store the patch:
	this->buffer = (BYTE*) calloc(modSize, sizeof(BYTE));
	if (!buffer) {
		throw CustomException("Cannot allocate modification buffer!");
		return;
	}
	
	//store the backup in the patch buffer:
	::memcpy(buffer, modPtr, modSize);
	
	this->offset = modOffset;
	this->size = modSize;
}


bool ModifBackup::_applyPatchContent(AbstractByteBuffer* fileBuf)
{
	if (!fileBuf || !this->buffer) {
		return false;
	}
	BYTE *modPtr = fileBuf->getContentAt(this->offset, this->size);
	if (!modPtr) {
		std::cerr << "Cannot apply backup at offset: " << std::hex << this->offset << " on the given file! Area size mismatch!" << std::endl;
		return false;
	}

	::memcpy(modPtr, this->buffer, this->size);
	return true;
}

bool ModifBackup::apply(AbstractByteBuffer* fileBuf)
{
	return _applyPatchContent(fileBuf);
}

bool ModifBackup::isOffsetAffected(offset_t curr_offset)
{
	if (this->offset == INVALID_ADDR) {
		return false;
	}
	const offset_t modE = this->offset + this->size;
	return (curr_offset >= this->offset && curr_offset < modE);
}

//---

ResizeBackup::ResizeBackup(AbstractByteBuffer *fileBuf, bufsize_t newSize)
	: ModifBackup(), fullSize(0)
{
	if (!fileBuf) throw CustomException("Uninitialized file");
	
	this->fullSize = fileBuf->getContentSize();
	if (fullSize == newSize) return;
	
	if (fullSize > newSize) {
		// shrinking operation
		const size_t modSize = fullSize - newSize;
		_storePatchContent(fileBuf, newSize, modSize);
	}
}

bool ResizeBackup::apply(AbstractByteBuffer* fileBuf)
{
	if (!fileBuf) return false;
	
	size_t currSize = fileBuf->getContentSize();
	if (fileBuf->resize(this->fullSize)) {
		if (currSize < this->fullSize) {
			_applyPatchContent(fileBuf);
		}
		return true;
	}
	return false;
}

//-------

void OperationBackup::deleteChildren()
{
	std::vector<ModifBackup*>::iterator itr =  modifs.begin();

	while (itr != modifs.end()) {
		ModifBackup* modif = *itr;
		++itr;
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

	ModifBackup *newModif = new ModifBackup(this->file, modifOffset, modifSize);
	if (_backupModification(newModif, continueLastOperation)) {
		return true;
	}
	delete newModif; // storage of the modification failed, so delete it
	return false;
}

bool ModificationHandler::backupResize(bufsize_t newSize, bool continueLastOperation)
{
	ResizeBackup *newModif = new ResizeBackup(this->file, newSize);
	if (_backupModification(newModif, continueLastOperation)) {
		return true;
	}
	delete newModif; // storage of the modification failed, so delete it
	return false;
}

bool ModificationHandler::_backupModification(ModifBackup *newModif, bool continueLastOperation)
{
	if (!newModif) return false;
	
	bool isOk = true;
	try {
		OperationBackup *last = this->getLastOperation();
		if (!continueLastOperation || !last) {
			isOk = store(newModif);
		} else {
			isOk = last->appendBackup(newModif);
		}

	} catch (CustomException &e) {
		std::cerr << "Backup error: " << e.what() << std::endl;
		isOk = false;
	}
	return isOk;
}

bool ModificationHandler::store(ModifBackup* modif)
{
	bool isOk = true;
	try {
		OperationBackup* backup = new OperationBackup(modif);
		isOk = storeOperation(backup);
	}
	catch (CustomException &e)
	{
		std::cerr << "Backup error: " << e.what() << std::endl;
		isOk = false;
	}
	return isOk;
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
	for (std::vector<ModifBackup*>::reverse_iterator itr = op->modifs.rbegin();
		itr != op->modifs.rend();
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

