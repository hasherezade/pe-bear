#pragma once

#include <QtGlobal>

#include "QtCompat.h"

class ViewSettings
{
public:

	static int scalingPercent()
	{
#if QT_VERSION >= 0x050000
		QScreen *screen = QApplication::primaryScreen();
		const int scaling_percent = (screen->logicalDotsPerInch() / 96) * 100;
		return scaling_percent;
#else
		return 100;
#endif
	}

	static qreal scaledFontSize(const QFont &font)
	{
		int sFactor = scalingPercent();
		
		int size = font.pointSize();
		if (size == -1) size = font.pixelSize();

		const qreal pointSize = (qreal(size) * sFactor) / 100.0;
		return pointSize;
	}
	
	static int getSmallIconDim(const QFont &font)
	{
		const int minDim = 12;
		int iconDim = ceil(scaledFontSize(font));
		if (iconDim < minDim) {
			iconDim = minDim;
		}
		return iconDim;
	}
	
	static int getMediumIconDim(const QFont &font)
	{
		const qreal minDim = 16;
		qreal iconDim = scaledFontSize(font) * 1.5;
		if (iconDim < minDim) {
			iconDim = minDim;
		}
		return static_cast<int>(ceil(iconDim));
	}
	
	static int getIconDim(const QFont &font)
	{
		const qreal minDim = 16;
		qreal iconDim = scaledFontSize(font) * 2.2;
		if (iconDim < minDim) {
			iconDim = minDim;
		}
		return static_cast<int>(ceil(iconDim));
	}

	static QPixmap makeScaledPixMap(const QString &resource,int w, int h);
	static QIcon makeScaledIcon(const QString &resource, int w, int h);

	static QPixmap getScaledPixmap(const QString &resource)
	{
		const int dim = ViewSettings::getMediumIconDim(QApplication::font());
		return ViewSettings::makeScaledPixMap(resource, dim, dim);
	}

	QFont myFont;
};

class HexViewSettings : public ViewSettings
{
public:

	static QFont defaultFont()
	{
#ifdef __APPLE__
		QFont myFont;
#else
		QFont myFont("TypeWriter");
#endif

#if QT_VERSION >= 0x040700
		myFont.setStyleHint (QFont::Monospace);
#else
		myFont.setStyleHint (QFont::TypeWriter);
#endif
		myFont.setPointSize(8);
		myFont.setLetterSpacing(QFont::AbsoluteSpacing, 0);
		myFont.setStretch(QFont::Unstretched);
		return myFont;
	}

	HexViewSettings()
	{
		myFont = defaultFont();

		hoveredColor = QColor("CornflowerBlue");
		hoveredColor.setAlpha(100);

		vHdrColor = QColor("LightGrey");
		vHdrColor.setAlpha(100);

		modifColor = QColor("red");
		inactiveModifColor = modifColor;
		inactiveModifColor.setAlpha(150);

		inactiveColor = QColor("grey");
	}

	~HexViewSettings() {}
	
	QSize getVerticalSize() const
	{
		const int fontDim = ViewSettings::getIconDim(myFont);
		return QSize(ceil(scaledFontSize(myFont) * 7), fontDim);
	}

	QColor hoveredColor;
	QColor vHdrColor;
	QColor modifColor, inactiveModifColor;
	QColor inactiveColor;
};

//---

class DisasmViewSettings : public ViewSettings
{
public:
	static QFont defaultFont()
	{
#ifdef __APPLE__
		QFont myFont;
#else
		QFont myFont("TypeWriter");
#endif

#if QT_VERSION >= 0x040700
		myFont.setStyleHint (QFont::Monospace);
#else
		myFont.setStyleHint (QFont::TypeWriter);
#endif
		myFont.setPointSize(8);
		myFont.setLetterSpacing(QFont::AbsoluteSpacing, 0);
		myFont.setStretch(QFont::Unstretched);
		return myFont;
	}

	DisasmViewSettings()
	{
		myFont = defaultFont();

		vHdrColor = QColor("LightGrey");
		vHdrColor.setAlpha(50);

		branchingColor = QColor("darkGrey");
		branchingColor.setAlpha(30);

		retColor = QColor("DodgerBlue");
		nopColor = QColor("grey");
		int3Color = QColor("#FF00FF");
		intXColor = QColor("#da7bef");
		callColor = QColor("orange");
		jumpColor = QColor("khaki");
		invalidColor = QColor("red");
		importColor = QColor("cyan");
		delayImpColor = QColor("pink");
		conditionalColor = QColor("yellow");
	}
	
	QSize getIconSize() const
	{
		const int fontDim = ViewSettings::getIconDim(myFont);
		return QSize(fontDim, fontDim);
	}
	
	QSize getVerticalSize() const
	{
		const int fontDim = ViewSettings::getIconDim(myFont);
		return QSize(fontDim * 5, fontDim);
	}
	
	QColor vHdrColor;
	QColor branchingColor;
	QColor retColor, nopColor, int3Color, intXColor, invalidColor, callColor, jumpColor,
		importColor, delayImpColor,
		conditionalColor;
};
