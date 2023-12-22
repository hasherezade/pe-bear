#pragma once

#include <QtCore>
#include <bearparser/bearparser.h>

class StringsCollection : public QObject
{
	Q_OBJECT

public:
	StringsCollection() {}

	bool fillStrings(QMap<offset_t, QString> *mapToFill)
	{
		if (!mapToFill) return false;
		
		QMutexLocker lock(&myMutex);
		stringsMap.clear();
		for (auto itr = mapToFill->begin(); itr != mapToFill->end(); ++itr) {
			stringsMap.insert( itr.key(), itr.value() );
		}
		//qDebug() << "Extracted map: " << stringsMap.size();
		return true;
	}
	
	QString getString(offset_t offset)
	{
		QMutexLocker lock(&myMutex);
		auto itr = stringsMap.find(offset);
		if (itr == stringsMap.end()) return "";
		return itr.value();
	}

	QList<offset_t> getOffsets() const
	{
		return stringsMap.keys();
	}
	
	size_t size() const
	{
		return stringsMap.size();
	}
	
protected:
	QMap<offset_t, QString> stringsMap;
	QMutex myMutex;
};
