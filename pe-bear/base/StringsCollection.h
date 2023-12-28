#pragma once

#include <QtCore>
#include <bearparser/bearparser.h>
#include "Releasable.h"

namespace util {
	
	inline size_t getStringSize(const QString &str, bool isWide)
	{
		size_t mul = isWide ? 2 : 1;
		return str.length() * mul;
	}
};

class StringsCollection : public QObject, public Releasable
{
	Q_OBJECT

public:
	StringsCollection() {}

	void insert(offset_t offset, const QString &str, bool isWide)
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
	
	bool saveToFile(const QString &fileName, const QString &delim = ";")
	{
		QFile fOut(fileName);
		if (fOut.open(QFile::WriteOnly | QFile::Text) == false) {
			return false;
		}
		QMutexLocker lock(&myMutex);
		QTextStream out(&fOut);
		for (auto itr = this->stringsMap.begin(); itr != this->stringsMap.end(); ++itr ) {
			offset_t offset = itr.key();
			QString qComment = itr.value();
			QString offsetStr = QString::number(offset, 16);
			qComment = qComment.simplified();
			
			QString line = offsetStr + delim + qComment;
			out << line << '\n';
		}
		fOut.close();
		return true;
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
		return _getString(offset);
	}
	
	size_t getStringSize(offset_t offset)
	{
		QMutexLocker lock(&myMutex);
		return util::getStringSize(_getString(offset), _isWide(offset));
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

	QString _getString(offset_t offset)
	{
		auto itr = stringsMap.find(offset);
		if (itr == stringsMap.end()) return "";
		return itr.value();
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
