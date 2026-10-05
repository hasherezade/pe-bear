#pragma once

#include <QtCore>
#include "Version.h"

namespace pe_bear {
namespace updater {

/**
 * User-facing updater preferences.
 *
 * Checking is on by default, because knowing a security tool is out of date
 * is itself useful; it can be turned off, and the interval is bounded.
 *
 * Stored under the "Updates" group of the application's existing QSettings,
 * so it travels with the rest of the configuration.
 */
class UpdateSettings
{
public:
	static const char* SETTINGS_GROUP;
	static const int DEFAULT_CHECK_INTERVAL_HOURS = 24;

	UpdateSettings();

	bool isAutoCheckEnabled() const { return m_autoCheck; }
	void setAutoCheckEnabled(bool enabled) { m_autoCheck = enabled; }

	int checkIntervalHours() const { return m_checkIntervalHours; }
	void setCheckIntervalHours(int hours);

	QDateTime lastCheck() const { return m_lastCheck; }
	void setLastCheck(const QDateTime &when) { m_lastCheck = when; }

	/** Version the user asked not to be reminded about; empty when none. */
	QString skippedVersion() const { return m_skippedVersion; }
	void setSkippedVersion(const Version &version);
	void clearSkippedVersion() { m_skippedVersion.clear(); }
	bool isVersionSkipped(const Version &version) const;

	/**
	 * True when an automatic check may run now: enabled, and the interval has
	 * elapsed since the last one. A clock that has moved backwards (or a
	 * corrupt timestamp) counts as due rather than blocking checks forever.
	 */
	bool isAutomaticCheckDue(const QDateTime &now) const;

	void read(QSettings &settings);
	void write(QSettings &settings) const;

private:
	bool m_autoCheck;
	int m_checkIntervalHours;
	QDateTime m_lastCheck;
	QString m_skippedVersion;
};

}; // namespace updater
}; // namespace pe_bear
