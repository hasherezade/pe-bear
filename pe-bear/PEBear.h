#pragma once

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	#include <QtWidgets>
#else
	#include <QtGui>
	typedef QRegularExpression QRegExp;
	typedef QRegularExpressionValidator QRegExpValidator;
#endif
