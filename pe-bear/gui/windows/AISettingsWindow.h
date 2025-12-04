#pragma once

#include <QtWidgets>
#include "../../base/AISettings.h"

class AISettingsWindow : public QDialog
{
	Q_OBJECT

public:
	explicit AISettingsWindow(AISettings *settings, QWidget *parent = nullptr);
	~AISettingsWindow();

private slots:
	void onSaveClicked();
	void onCancelClicked();
	void onTestOpenAI();
	void onTestGemini();
	void onTestVirusTotal();

private:
	void setupUI();
	void loadSettings();
	void saveSettings();
	
	AISettings *aiSettings;
	
	// UI Components
	QLineEdit *openaiKeyEdit;
	QLineEdit *geminiKeyEdit;
	QLineEdit *virusTotalKeyEdit;
	QComboBox *openaiModelCombo;
	QComboBox *geminiModelCombo;
	QCheckBox *useOpenAICBox;
	QCheckBox *useGeminiCBox;
	QPushButton *testOpenAIBtn;
	QPushButton *testGeminiBtn;
	QPushButton *testVTBtn;
	QPushButton *saveBtn;
	QPushButton *cancelBtn;
};
