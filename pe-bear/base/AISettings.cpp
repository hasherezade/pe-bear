#include "AISettings.h"
#include "MainSettings.h"
#include <QSettings>

bool AISettings::readPersistent()
{
	QSettings settings(COMPANY_NAME, APP_NAME);
	
	this->openaiApiKey = settings.value("AI/openai_api_key", "").toString();
	this->geminiApiKey = settings.value("AI/gemini_api_key", "").toString();
	this->virusTotalApiKey = settings.value("AI/virustotal_api_key", "").toString();
	this->openaiModel = settings.value("AI/openai_model", "gpt-4").toString();
	this->geminiModel = settings.value("AI/gemini_model", "gemini-2.0-flash").toString();
	this->useOpenAI = settings.value("AI/use_openai", false).toBool();
	this->useGemini = settings.value("AI/use_gemini", true).toBool();
	
	return true;
}

bool AISettings::writePersistent()
{
	QSettings settings(COMPANY_NAME, APP_NAME);
	
	settings.setValue("AI/openai_api_key", this->openaiApiKey);
	settings.setValue("AI/gemini_api_key", this->geminiApiKey);
	settings.setValue("AI/virustotal_api_key", this->virusTotalApiKey);
	settings.setValue("AI/openai_model", this->openaiModel);
	settings.setValue("AI/gemini_model", this->geminiModel);
	settings.setValue("AI/use_openai", this->useOpenAI);
	settings.setValue("AI/use_gemini", this->useGemini);
	
	return true;
}
