#include "PeWrapperModel.h"

bool PeWrapperModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;

	size_t fID = this->getFID(index);
	size_t sID = this->getSID(index);

	ExeElementWrapper* wrap = wrapperAt(index);
	if (!wrap) return false;

	ExeElementWrapper* mainWrapper = wrapper();
	if (!mainWrapper) return false;
	
	if (wrap != mainWrapper) { //it is subWrapper
		fID = sID;
		sID = FIELD_NONE;
	}

	QString text = value.toString();

	bool isOk = false;
	ULONGLONG number = text.toULongLong(&isOk, 16);
	if (!isOk) return false;
	
	offset_t offset = wrap->getFieldOffset(fID);
	if (offset == INVALID_ADDR) {
		return false;
	}
	bufsize_t fieldSize = wrap->getFieldSize(fID);

	this->myPeHndl->backupModification(offset, fieldSize);
	
	bool isModified = wrap->setNumValue(fID, sID, number);
	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}

Qt::ItemFlags PeWrapperModel::flags(const QModelIndex &index) const
{	
	if (!index.isValid()) return Qt::NoItemFlags;
	Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if (this->containsOffset(index)) return f;
	if (this->containsValue(index)) return f | Qt::ItemIsEditable;
	return f;
}

QString PeWrapperModel::makeDockerTitle(size_t upId)
{
	ExeNodeWrapper* node = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (node == NULL) {
		return "-";
	}
	ExeNodeWrapper *childEntry = node->getEntryAt(upId);
	if (childEntry == NULL) {
		return "-";
	}
	QString name = childEntry->getName();
	size_t funcNum = childEntry->getEntriesCount();
	QString numDesc = funcNum == 1 ? tr(" entry") : tr(" entries");
	QString desc = name + "   [ " + QString::number(funcNum) + numDesc + " ]"; 
	return desc;
}
