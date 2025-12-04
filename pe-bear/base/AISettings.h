#pragma once

#include <QtCore>

class AISettings : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString openaiApiKey READ getOpenAIApiKey WRITE setOpenAIApiKey)
	Q_PROPERTY(QString geminiApiKey READ getGeminiApiKey WRITE setGeminiApiKey)
	Q_PROPERTY(QString virusTotalApiKey READ getVirusTotalApiKey WRITE setVirusTotalApiKey)
	Q_PROPERTY(QString openaiModel READ getOpenAIModel WRITE setOpenAIModel)
	Q_PROPERTY(QString geminiModel READ getGeminiModel WRITE setGeminiModel)
	Q_PROPERTY(bool useOpenAI READ isUseOpenAI WRITE setUseOpenAI)
	Q_PROPERTY(bool useGemini READ isUseGemini WRITE setUseGemini)

signals:
	void settingsChanged();

public:
	AISettings() : QObject(),
		useOpenAI(false),
		useGemini(false),
		openaiModel("gpt-4"),
		geminiModel("gemini-pro")
	{
	}

	// OpenAI Settings
	void setOpenAIApiKey(const QString &key) { 
		this->openaiApiKey = key; 
		emit settingsChanged(); 
	}
	QString getOpenAIApiKey() const { return this->openaiApiKey; }

	void setOpenAIModel(const QString &model) { 
		this->openaiModel = model; 
		emit settingsChanged(); 
	}
	QString getOpenAIModel() const { return this->openaiModel; }

	void setUseOpenAI(bool use) { 
		this->useOpenAI = use; 
		emit settingsChanged(); 
	}
	bool isUseOpenAI() const { return this->useOpenAI; }

	// Gemini Settings
	void setGeminiApiKey(const QString &key) { 
		this->geminiApiKey = key; 
		emit settingsChanged(); 
	}
	QString getGeminiApiKey() const { return this->geminiApiKey; }

	void setGeminiModel(const QString &model) { 
		this->geminiModel = model; 
		emit settingsChanged(); 
	}
	QString getGeminiModel() const { return this->geminiModel; }

	void setUseGemini(bool use) { 
		this->useGemini = use; 
		emit settingsChanged(); 
	}
	bool isUseGemini() const { return this->useGemini; }

	// VirusTotal Settings
	void setVirusTotalApiKey(const QString &key) { 
		this->virusTotalApiKey = key; 
		emit settingsChanged(); 
	}
	QString getVirusTotalApiKey() const { return this->virusTotalApiKey; }

	bool readPersistent();
	bool writePersistent();

protected:
	QString openaiApiKey;
	QString geminiApiKey;
	QString virusTotalApiKey;
	QString openaiModel;
	QString geminiModel;
	bool useOpenAI;
	bool useGemini;
};
