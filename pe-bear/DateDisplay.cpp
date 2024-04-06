#include "DateDisplay.h"

#include <time.h>

QString getDateString(const quint64 timestamp)
{
	if (timestamp == 0 || timestamp == (-1)) {
		return "";
	}
	const time_t rawtime = (const time_t)timestamp;
	QString format = "dddd, dd.MM.yyyy hh:mm:ss";
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		QDateTime date1(QDateTime::fromSecsSinceEpoch(rawtime));
#else
		QDateTime date1(QDateTime::fromTime_t(rawtime));
#endif
	return date1.toUTC().toString(format) + " UTC";
}

