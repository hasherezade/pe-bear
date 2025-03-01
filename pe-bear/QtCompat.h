#pragma once
#include <QtGlobal>
#include <QtCore>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	#include <QtWidgets>
#else
	#include <QtGui>
	typedef QRegExp QRegularExpression;
	typedef QRegExpValidator QRegularExpressionValidator;
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        #define QT_SkipEmptyParts Qt::SkipEmptyParts
#else
        #define QT_SkipEmptyParts QString::SkipEmptyParts
#endif
