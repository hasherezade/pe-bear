#include "AISettingsWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

AISettingsWindow::AISettingsWindow(AISettings *settings, QWidget *parent)
	: QDialog(parent), aiSettings(settings)
{
	setupUI();
	loadSettings();
	setWindowTitle("AI Assistant Settings");
	resize(600, 500);
}

AISettingsWindow::~AISettingsWindow()
{
}

void AISettingsWindow::setupUI()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	
	// OpenAI Group
	QGroupBox *openaiGroup = new QGroupBox("OpenAI / ChatGPT Configuration", this);
	QVBoxLayout *openaiLayout = new QVBoxLayout(openaiGroup);
	
	useOpenAICBox = new QCheckBox("Enable OpenAI Assistant", this);
	openaiLayout->addWidget(useOpenAICBox);
	
	QHBoxLayout *openaiKeyLayout = new QHBoxLayout();
	QLabel *openaiKeyLabel = new QLabel("API Key:", this);
	openaiKeyEdit = new QLineEdit(this);
	openaiKeyEdit->setEchoMode(QLineEdit::Password);
	openaiKeyEdit->setPlaceholderText("sk-...");
	openaiKeyLayout->addWidget(openaiKeyLabel);
	openaiKeyLayout->addWidget(openaiKeyEdit);
	openaiLayout->addLayout(openaiKeyLayout);
	
	QHBoxLayout *openaiModelLayout = new QHBoxLayout();
	QLabel *openaiModelLabel = new QLabel("Model:", this);
	openaiModelCombo = new QComboBox(this);
	openaiModelCombo->addItem("gpt-4");
	openaiModelCombo->addItem("gpt-4-turbo-preview");
	openaiModelCombo->addItem("gpt-3.5-turbo");
	openaiModelLayout->addWidget(openaiModelLabel);
	openaiModelLayout->addWidget(openaiModelCombo);
	openaiLayout->addLayout(openaiModelLayout);
	
	testOpenAIBtn = new QPushButton("Test Connection", this);
	connect(testOpenAIBtn, &QPushButton::clicked, this, &AISettingsWindow::onTestOpenAI);
	openaiLayout->addWidget(testOpenAIBtn);
	
	QLabel *openaiInfo = new QLabel("Get your API key from: https://platform.openai.com/api-keys", this);
	openaiInfo->setWordWrap(true);
	openaiInfo->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
	openaiLayout->addWidget(openaiInfo);
	
	mainLayout->addWidget(openaiGroup);
	
	// Gemini Group
	QGroupBox *geminiGroup = new QGroupBox("Google Gemini Configuration", this);
	QVBoxLayout *geminiLayout = new QVBoxLayout(geminiGroup);
	
	useGeminiCBox = new QCheckBox("Enable Gemini Assistant", this);
	geminiLayout->addWidget(useGeminiCBox);
	
	QHBoxLayout *geminiKeyLayout = new QHBoxLayout();
	QLabel *geminiKeyLabel = new QLabel("API Key:", this);
	geminiKeyEdit = new QLineEdit(this);
	geminiKeyEdit->setEchoMode(QLineEdit::Password);
	geminiKeyEdit->setPlaceholderText("AIza...");
	geminiKeyLayout->addWidget(geminiKeyLabel);
	geminiKeyLayout->addWidget(geminiKeyEdit);
	geminiLayout->addLayout(geminiKeyLayout);
	
	QHBoxLayout *geminiModelLayout = new QHBoxLayout();
	QLabel *geminiModelLabel = new QLabel("Model:", this);
	geminiModelCombo = new QComboBox(this);
	geminiModelCombo->addItem("gemini-pro");
	geminiModelCombo->addItem("gemini-1.5-pro");
	geminiModelCombo->addItem("gemini-1.5-flash");
	geminiModelLayout->addWidget(geminiModelLabel);
	geminiModelLayout->addWidget(geminiModelCombo);
	geminiLayout->addLayout(geminiModelLayout);
	
	testGeminiBtn = new QPushButton("Test Connection", this);
	connect(testGeminiBtn, &QPushButton::clicked, this, &AISettingsWindow::onTestGemini);
	geminiLayout->addWidget(testGeminiBtn);
	
	QLabel *geminiInfo = new QLabel("Get your API key from: https://makersuite.google.com/app/apikey", this);
	geminiInfo->setWordWrap(true);
	geminiInfo->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
	geminiLayout->addWidget(geminiInfo);
	
	mainLayout->addWidget(geminiGroup);
	
	// VirusTotal Group
	QGroupBox *vtGroup = new QGroupBox("VirusTotal Configuration", this);
	QVBoxLayout *vtLayout = new QVBoxLayout(vtGroup);
	
	QHBoxLayout *vtKeyLayout = new QHBoxLayout();
	QLabel *vtKeyLabel = new QLabel("API Key:", this);
	virusTotalKeyEdit = new QLineEdit(this);
	virusTotalKeyEdit->setEchoMode(QLineEdit::Password);
	virusTotalKeyEdit->setPlaceholderText("Enter your VirusTotal API key");
	vtKeyLayout->addWidget(vtKeyLabel);
	vtKeyLayout->addWidget(virusTotalKeyEdit);
	vtLayout->addLayout(vtKeyLayout);
	
	testVTBtn = new QPushButton("Test Connection", this);
	connect(testVTBtn, &QPushButton::clicked, this, &AISettingsWindow::onTestVirusTotal);
	vtLayout->addWidget(testVTBtn);
	
	QLabel *vtInfo = new QLabel("Get your API key from: https://www.virustotal.com/gui/my-apikey", this);
	vtInfo->setWordWrap(true);
	vtInfo->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
	vtLayout->addWidget(vtInfo);
	
	mainLayout->addWidget(vtGroup);
	
	// Buttons
	QHBoxLayout *buttonLayout = new QHBoxLayout();
	saveBtn = new QPushButton("Save", this);
	cancelBtn = new QPushButton("Cancel", this);
	
	connect(saveBtn, &QPushButton::clicked, this, &AISettingsWindow::onSaveClicked);
	connect(cancelBtn, &QPushButton::clicked, this, &AISettingsWindow::onCancelClicked);
	
	buttonLayout->addStretch();
	buttonLayout->addWidget(saveBtn);
	buttonLayout->addWidget(cancelBtn);
	
	mainLayout->addLayout(buttonLayout);
	
	setLayout(mainLayout);
}

void AISettingsWindow::loadSettings()
{
	if (!aiSettings) return;
	
	useOpenAICBox->setChecked(aiSettings->isUseOpenAI());
	useGeminiCBox->setChecked(aiSettings->isUseGemini());
	
	openaiKeyEdit->setText(aiSettings->getOpenAIApiKey());
	geminiKeyEdit->setText(aiSettings->getGeminiApiKey());
	virusTotalKeyEdit->setText(aiSettings->getVirusTotalApiKey());
	
	int openaiIdx = openaiModelCombo->findText(aiSettings->getOpenAIModel());
	if (openaiIdx >= 0) openaiModelCombo->setCurrentIndex(openaiIdx);
	
	int geminiIdx = geminiModelCombo->findText(aiSettings->getGeminiModel());
	if (geminiIdx >= 0) geminiModelCombo->setCurrentIndex(geminiIdx);
}

void AISettingsWindow::saveSettings()
{
	if (!aiSettings) return;
	
	aiSettings->setUseOpenAI(useOpenAICBox->isChecked());
	aiSettings->setUseGemini(useGeminiCBox->isChecked());
	
	aiSettings->setOpenAIApiKey(openaiKeyEdit->text());
	aiSettings->setGeminiApiKey(geminiKeyEdit->text());
	aiSettings->setVirusTotalApiKey(virusTotalKeyEdit->text());
	
	aiSettings->setOpenAIModel(openaiModelCombo->currentText());
	aiSettings->setGeminiModel(geminiModelCombo->currentText());
	
	aiSettings->writePersistent();
}

void AISettingsWindow::onSaveClicked()
{
	saveSettings();
	QMessageBox::information(this, "Settings Saved", 
							 "AI Assistant settings have been saved successfully.");
	accept();
}

void AISettingsWindow::onCancelClicked()
{
	reject();
}

void AISettingsWindow::onTestOpenAI()
{
	if (openaiKeyEdit->text().isEmpty()) {
		QMessageBox::warning(this, "No API Key", "Please enter your OpenAI API key first.");
		return;
	}
	
	QMessageBox::information(this, "Test Connection", 
							 "This would test the OpenAI connection.\n"
							 "For now, save your settings and try the AI Assistant features.");
}

void AISettingsWindow::onTestGemini()
{
	if (geminiKeyEdit->text().isEmpty()) {
		QMessageBox::warning(this, "No API Key", "Please enter your Gemini API key first.");
		return;
	}
	
	QMessageBox::information(this, "Test Connection", 
							 "This would test the Gemini connection.\n"
							 "For now, save your settings and try the AI Assistant features.");
}

void AISettingsWindow::onTestVirusTotal()
{
	if (virusTotalKeyEdit->text().isEmpty()) {
		QMessageBox::warning(this, "No API Key", "Please enter your VirusTotal API key first.");
		return;
	}
	
	QMessageBox::information(this, "Test Connection", 
							 "This would test the VirusTotal connection.\n"
							 "For now, save your settings and try the AI Assistant features.");
}
