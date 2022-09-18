#pragma once

#ifdef WITH_QT5
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include <QDateTime>

QString getDateString(const quint64 timestamp);

