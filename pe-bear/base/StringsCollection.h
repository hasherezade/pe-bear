#pragma once

#include <QtCore>
#include <bearparser/bearparser.h>

class StringsCollection : public QObject
{
	Q_OBJECT

public:
	StringsCollection() {}

	bool insert(offset_t offset, const QString &str, bool isWide)
	{
		QMutexLocker lock(&myMutex);
		stringsMap.insert(offset, str);
		if (isWide) {
			wideStrings.insert(offset);
		}
	}

	void fill(StringsCollection &collection)
	{
		QMutexLocker lock1(&myMutex);
		QMutexLocker lock2(&collection.myMutex);
		_clear();
		for (auto itr = collection.stringsMap.begin(); itr != collection.stringsMap.end(); ++itr) {
			stringsMap.insert( itr.key(), itr.value() );
			offset_t offset = itr.key();
			if (collection._isWide(offset)) {
				wideStrings.insert(offset);
			}
		}
	}

	bool fillStrings(QMap<offset_t, QString> *mapToFill)
	{
		if (!mapToFill) return false;
		
		QMutexLocker lock(&myMutex);
		_clear();
		for (auto itr = mapToFill->begin(); itr != mapToFill->end(); ++itr) {
			offset_t offset = itr.key();
			stringsMap.insert(offset, itr.value());
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

	QList<offset_t> getOffsets()
	{
		QMutexLocker lock(&myMutex);
		return stringsMap.keys();
	}
	
	bool isWide(offset_t offset)
	{
		QMutexLocker lock(&myMutex);
		return _isWide(offset);
	}

	size_t size()
	{
		QMutexLocker lock(&myMutex);
		return stringsMap.size();
	}

	void clear()
	{
		QMutexLocker lock(&myMutex);
		_clear();
	}
	
protected:
	void _clear()
	{
		stringsMap.clear();
		wideStrings.clear();
	}

	bool _isWide(offset_t offset)
	{
		if (wideStrings.find(offset) != wideStrings.end()) {
			return true;
		}
		return false;
	}

	QMap<offset_t, QString> stringsMap;
	QSet<offset_t> wideStrings;

	QMutex myMutex;
};
