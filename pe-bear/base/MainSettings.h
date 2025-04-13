#pragma once

#include <QtCore>

#include <bearparser/bearparser.h>
#include "../ViewSettings.h"

#define APP_NAME "PE-bear"
#define COMPANY_NAME APP_NAME

//--------------------

class defaultStylesheet;
class ColorSettings : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QColor rawColor	READ rawColor	WRITE setRawColor)
	Q_PROPERTY(QColor rvaColor	READ rvaColor	WRITE setRvaColor)
	Q_PROPERTY(QColor vaColor	READ vaColor	WRITE setVaColor)
	
	Q_PROPERTY(QColor flagsColor	READ flagsColor	WRITE setFlagsColor)
	Q_PROPERTY(QColor dataDirColor	READ dataDirColor	WRITE setDataDirColor)
	Q_PROPERTY(QColor dataDirNameColor	READ dataDirNameColor	WRITE setDataDirNameColor)
	
public:
	static QString defaultRawColor, defaultRvaColor, defaultVaColor,
		defaultFlagsColor, defaultDataDirColor, defaultDataDirNameColor;

	static QString defaultStyle;
	
	ColorSettings()
	{
		init();
	}
	
	void setRawColor(const QColor &_rawColor)
	{
		this->m_rawColor = _rawColor;
	}
	
	QColor rawColor() const
	{
		return m_rawColor;
	}
	
	void setRvaColor(const QColor &_rvaColor)
	{
		this->m_rvaColor = _rvaColor;
	}
	
	QColor rvaColor() const
	{
		return m_rvaColor;
	}
	
	void setVaColor(const QColor &_vaColor)
	{
		this->m_vaColor = _vaColor;
	}
	
	QColor vaColor() const
	{
		return m_vaColor;
	}
	
	QColor addrTypeToColor(const Executable::addr_type &aT) const;
	
	void setFlagsColor(const QColor &_flagsColor)
	{
		this->m_flagsColor = _flagsColor;
	}
	
	QColor flagsColor() const
	{
		return m_flagsColor;
	}

	int flagsAlpha() const
	{
		return 200;
	}

	void setDataDirColor(const QColor &_dataDirColor)
	{
		this->m_dataDirColor = _dataDirColor;
	}
	
	QColor dataDirColor() const
	{
		return m_dataDirColor;
	}

	void setDataDirNameColor(const QColor &_dataDirNameColor)
	{
		this->m_dataDirNameColor = _dataDirNameColor;
	}
	
	QColor dataDirNameColor() const
	{
		return m_dataDirNameColor;
	}
	
	int dataDirNameAlpha() const
	{
		return 150;
	}
	
private:
	virtual void init()
	{
		m_rawColor = QColor(defaultRawColor);
		m_rvaColor = QColor(defaultRvaColor);
		m_vaColor = QColor(defaultVaColor);
		
		m_flagsColor = QColor(defaultFlagsColor);
		m_dataDirColor = QColor(defaultDataDirColor);
		m_dataDirNameColor = QColor(defaultDataDirNameColor);
	}

	QColor m_rawColor, m_rvaColor, m_vaColor;
	QColor m_flagsColor;
	QColor m_dataDirColor;
	QColor m_dataDirNameColor;
};

//--------------------

class GuiSettings : public QObject
{
	Q_OBJECT

signals:
	void disasmViewSettingsChanged(DisasmViewSettings&);
	void hexViewSettingsChanged(HexViewSettings&);
	void globalFontChanged();

private slots:
	void resetStyleSheet()
	{
		QString style = qApp->styleSheet();
		if (style.length() > 0) {
			qApp->setStyleSheet(qApp->styleSheet());
		}
	}

public:
	GuiSettings() 
	: QObject(),
		hexVSettings(), disasmVSettings()
	{
		initDefaultGlobalFont();
		initStyles();
	}

	bool readPersistent();
	bool writePersistent();

	void resetFontsToDefaults()
	{
		QApplication::setFont(defaultGlobalFont);
		hexVSettings.myFont = HexViewSettings::defaultFont();
		disasmVSettings.myFont = DisasmViewSettings::defaultFont();
		
		resetStyleSheet();
		
		emit globalFontChanged();
		emit disasmViewSettingsChanged(disasmVSettings);
		emit hexViewSettingsChanged(hexVSettings);
	}
	
	void resetSizesToDefaults()
	{
		QFont currFont = QApplication::font();
		currFont.setPointSize(defaultGlobalFont.pointSize());
		QApplication::setFont(currFont);
		
		hexVSettings.myFont.setPointSize(HexViewSettings::defaultFont().pointSize());
		disasmVSettings.myFont.setPointSize(DisasmViewSettings::defaultFont().pointSize());
		
		resetStyleSheet();

		emit globalFontChanged();
		emit disasmViewSettingsChanged(disasmVSettings);
		emit hexViewSettingsChanged(hexVSettings);
	}
	
	void setGlobalFont(QFont &font)
	{
		_setGlobalFont(font);
		resetStyleSheet();
		emit globalFontChanged();
	}

	void setHexViewFont(QFont &font)
	{
		if (_setHexViewFont(font)) {
			resetStyleSheet();
			emit hexViewSettingsChanged(hexVSettings);
		}
	}
	
	void setDisasmViewFont(QFont &font)
	{
		if (_setDisasmViewFont(font)) {
			resetStyleSheet();
			emit disasmViewSettingsChanged(disasmVSettings);

		}
	}
	
	void zoomHexViewFont(bool zoomIn)
	{
		if (zoomFont(this->hexVSettings.myFont, zoomIn)) {
			resetStyleSheet();
			emit hexViewSettingsChanged(hexVSettings);
		}
	}
	
	void zoomDisasmViewFont(bool zoomIn)
	{
		if (zoomFont(this->disasmVSettings.myFont, zoomIn)) {
			resetStyleSheet();
			emit disasmViewSettingsChanged(disasmVSettings);
		}
	}

	void zoomGlobalFont(bool zoomIn)
	{
		if (_zoomGlobalFont(zoomIn)) {
			emit globalFontChanged();
		}
	}
	
	void zoomAllFonts(bool zoomIn)
	{
		enum el_states { GLOBAL, HEX, DISASM, COUNT };
		int state[COUNT] = { 0 };
		state[GLOBAL] = _zoomGlobalFont(zoomIn);
		state[HEX] = _zoomHexViewFont(zoomIn);
		state[DISASM] = _zoomDisasmViewFont(zoomIn);
		
		if (state[GLOBAL] || state[HEX] || state[DISASM]) {
			resetStyleSheet();
			emit globalFontChanged();
			emit hexViewSettingsChanged(hexVSettings);
			emit disasmViewSettingsChanged(disasmVSettings);
		}
	}
	
	void resetFonts()
	{
		QFont currFont = QApplication::font();
		_setGlobalFont(currFont);

		emit globalFontChanged();
	}
	
	QFont getHexViewFont()
	{
		return hexVSettings.myFont;
	}
	
	QFont getDisasmViewFont()
	{
		return disasmVSettings.myFont;
	}

	void setDefaultStyle();

	void setStyleByName(const QString &name);
	
	QList<QString> getStyles()
	{
		return this->nameToStyle.keys();
	}
	
	QString currentStyleName()
	{
		if (this->currentStyle.length() == 0) {
			return this->defaultStyleName;
		}
		return this->currentStyle;
	}
	
protected:
	void _setGlobalFont(QFont &font)
	{
		qApp->setFont(font);
	}

	bool _setHexViewFont(QFont &font)
	{
		if (font == hexVSettings.myFont) {
			return false;
		}
		hexVSettings.myFont = font;
		return true;
	}

	bool _setDisasmViewFont(QFont &font)
	{
		if (font == hexVSettings.myFont) {
			return false;
		}
		disasmVSettings.myFont = font;
		return true;
	}

	bool zoomFont(QFont &font, bool zoomIn)
	{
		const int val = (zoomIn) ? 1 : (-1);
		const int minFontSize = 4;
		
		if (zoomIn || font.pointSize() > minFontSize) {
			font.setPointSize(font.pointSize() + val);
			return true;
		}
		return false;
	}
	
	bool _zoomGlobalFont(bool zoomIn)
	{
		QFont gFont = QApplication::font();
		if (zoomFont(gFont, zoomIn)) {
			_setGlobalFont(gFont);
			return true;
		}
		return false;
	}
	
	bool _zoomDisasmViewFont(bool zoomIn)
	{
		return (zoomFont(this->disasmVSettings.myFont, zoomIn));
	}
	
	bool _zoomHexViewFont(bool zoomIn)
	{
		return (zoomFont(this->hexVSettings.myFont, zoomIn));
	}
	
	void initDefaultGlobalFont()
	{
		// retrieve a default system font before loading the saved settings:
		defaultGlobalFont = QApplication::font();
	}

	void initStyles();

	QFont defaultGlobalFont;
	QString defaultStylesheet;
	
	QMap<QString, QString> nameToStyle;
	QString currentStyle;
	QString defaultStyleName;

	//current settings:
	HexViewSettings hexVSettings;
	DisasmViewSettings disasmVSettings;
};

//--------------------

typedef enum _reload_mode {
	RELOAD_ASK = 0,
	RELOAD_IGNORE = 1,
	RELOAD_AUTO = 2,
	RELOAD_MODES_COUNT
} t_reload_mode;

t_reload_mode intToReloadMode(int val);

////

class MainSettings : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool isFollowOnClick READ isFollowOnClick WRITE setFollowOnClick)
	Q_PROPERTY(bool autoSaveTags READ isAutoSaveTags WRITE setAutoSaveTags)
	Q_PROPERTY(t_reload_mode reloadOnFileChange READ isReloadOnFileChange WRITE setReloadOnFileChange )
	
	Q_PROPERTY(QString uDataDir READ userDataDir WRITE setUserDataDir)
	Q_PROPERTY(QString lExePath READ lastExePath WRITE setLastExePath)

signals:
	void settingsChanged();

public:
	static const QString languageDir;
	
	MainSettings(): 
		QObject(), followOnClick(false), autoSaveTags(true), autoReloadOnFileChange(RELOAD_ASK),
		uDataDir("")
	{
	}

	void setFollowOnClick(bool enable) {  this->followOnClick = enable; emit settingsChanged(); }
	bool isFollowOnClick() const { return this->followOnClick; }

	void setUserDataDir(QString newUDD) { this->uDataDir = newUDD; emit settingsChanged(); }
	QString userDataDir() { return uDataDir; }

	void setLastExePath(const QString path) { this->lExePath = path; }
	QString lastExePath() { return lExePath; }

	void setAutoSaveTags(bool enable) {  this->autoSaveTags = enable; emit settingsChanged(); }
	bool isAutoSaveTags() const { return this->autoSaveTags; }

	void setReloadOnFileChange(t_reload_mode enable) {  this->autoReloadOnFileChange = enable; emit settingsChanged(); }
	t_reload_mode isReloadOnFileChange() const { return this->autoReloadOnFileChange; }

	bool readPersistent();
	bool writePersistent();

	QString dirDump;
	QString language;

protected:
	bool followOnClick;
	QString uDataDir;
	QString lExePath;
	bool autoSaveTags;
	t_reload_mode autoReloadOnFileChange;
};
