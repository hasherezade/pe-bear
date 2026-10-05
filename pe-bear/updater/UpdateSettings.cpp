#include "UpdateSettings.h"

using namespace pe_bear::updater;

const char* UpdateSettings::SETTINGS_GROUP = "Updates";
const int UpdateSettings::DEFAULT_CHECK_INTERVAL_HOURS;

namespace {
	const char* KEY_AUTO_CHECK = "AutoCheck";
	const char* KEY_INTERVAL_HOURS = "CheckIntervalHours";
	const char* KEY_LAST_CHECK = "LastCheck";
	const char* KEY_SKIPPED_VERSION = "SkippedVersion";

	const int MIN_INTERVAL_HOURS = 1;
	const int MAX_INTERVAL_HOURS = 24 * 30;
}; // namespace

UpdateSettings::UpdateSettings()
	: m_autoCheck(true), m_checkIntervalHours(DEFAULT_CHECK_INTERVAL_HOURS)
{
}

void UpdateSettings::setCheckIntervalHours(int hours)
{
	if (hours < MIN_INTERVAL_HOURS) hours = MIN_INTERVAL_HOURS;
	if (hours > MAX_INTERVAL_HOURS) hours = MAX_INTERVAL_HOURS;
	m_checkIntervalHours = hours;
}

void UpdateSettings::setSkippedVersion(const Version &version)
{
	m_skippedVersion = version.isValid() ? version.toString() : QString();
}

bool UpdateSettings::isVersionSkipped(const Version &version) const
{
	if (m_skippedVersion.isEmpty() || !version.isValid()) return false;
	const Version skipped = Version::fromString(m_skippedVersion);
	if (!skipped.isValid()) return false;
	/* Skipping 0.7.3 must not also silence 0.7.4. */
	return (skipped == version);
}

bool UpdateSettings::isAutomaticCheckDue(const QDateTime &now) const
{
	if (!m_autoCheck) return false;
	if (!m_lastCheck.isValid()) return true;
	if (!now.isValid()) return false;
	if (m_lastCheck > now) return true; /* clock moved back, or a bad stored value */

	const qint64 elapsedSecs = m_lastCheck.secsTo(now);
	return elapsedSecs >= (static_cast<qint64>(m_checkIntervalHours) * 3600);
}

void UpdateSettings::read(QSettings &settings)
{
	settings.beginGroup(QLatin1String(SETTINGS_GROUP));
	m_autoCheck = settings.value(QLatin1String(KEY_AUTO_CHECK), true).toBool();
	setCheckIntervalHours(settings.value(QLatin1String(KEY_INTERVAL_HOURS),
		DEFAULT_CHECK_INTERVAL_HOURS).toInt());

	const QString lastCheckStr = settings.value(QLatin1String(KEY_LAST_CHECK)).toString();
	m_lastCheck = lastCheckStr.isEmpty()
		? QDateTime()
		: QDateTime::fromString(lastCheckStr, Qt::ISODate);

	const QString skipped = settings.value(QLatin1String(KEY_SKIPPED_VERSION)).toString();
	/* Anything unparseable in the config is dropped rather than trusted. */
	m_skippedVersion = Version::fromString(skipped).isValid() ? skipped : QString();
	settings.endGroup();
}

void UpdateSettings::write(QSettings &settings) const
{
	settings.beginGroup(QLatin1String(SETTINGS_GROUP));
	settings.setValue(QLatin1String(KEY_AUTO_CHECK), m_autoCheck);
	settings.setValue(QLatin1String(KEY_INTERVAL_HOURS), m_checkIntervalHours);
	settings.setValue(QLatin1String(KEY_LAST_CHECK),
		m_lastCheck.isValid() ? m_lastCheck.toString(Qt::ISODate) : QString());
	settings.setValue(QLatin1String(KEY_SKIPPED_VERSION), m_skippedVersion);
	settings.endGroup();
}
