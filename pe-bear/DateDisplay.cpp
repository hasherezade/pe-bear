#include "DateDisplay.h"

#include <time.h>

QString getDateString(const quint64 timestamp)
{
	const time_t rawtime = (const time_t)timestamp;
	QString format = "dddd, dd.MM.yyyy hh:mm:ss";
	QDateTime date1(QDateTime(QDateTime::fromTime_t(rawtime)));
	return date1.toUTC().toString(format) + " UTC";
}

