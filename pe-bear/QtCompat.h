#pragma once
#include <QtGlobal>
#include <QtCore>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	#include <QtWidgets>
	#define QT_SkipEmptyParts Qt::SkipEmptyParts
#else
	#include <QtGui>
	typedef QRegExp QRegularExpression;
	typedef QRegExpValidator QRegularExpressionValidator;
	#define QT_SkipEmptyParts QString::SkipEmptyParts
#endif
