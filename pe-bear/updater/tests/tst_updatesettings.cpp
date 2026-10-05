/*
 * Covers the updater preferences of PR #4 (UT-10 and the consent defaults).
 *
 * The defaults are the policy: checking on, downloading off, installing never
 * a setting at all.
 */
#include <QtTest>
#include "../UpdateSettings.h"

using namespace pe_bear::updater;

class TestUpdateSettings : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();

	void defaultsMatchTheConsentModel();
	void automaticCheckRunsAtMostOncePerInterval();
	void automaticCheckIsDueWhenNeverRun();
	void automaticCheckIsDueAfterTheClockMovesBackwards();
	void automaticCheckIsNeverDueWhenDisabled();
	void skippingOneVersionDoesNotSilenceLaterOnes();
	void intervalIsClampedToSaneValues();
	void survivesARoundTripThroughQSettings();
	void ignoresACorruptStoredVersion();
};

void TestUpdateSettings::initTestCase()
{
	QCoreApplication::setOrganizationName(QLatin1String("PE-bear-tests"));
	QCoreApplication::setApplicationName(QLatin1String("UpdateSettingsTest"));
	QSettings settings;
	settings.clear();
}

void TestUpdateSettings::defaultsMatchTheConsentModel()
{
	const UpdateSettings s;
	QVERIFY2(s.isAutoCheckEnabled(), "checking should be on by default");
	QCOMPARE(s.checkIntervalHours(), int(UpdateSettings::DEFAULT_CHECK_INTERVAL_HOURS));
	QVERIFY(s.skippedVersion().isEmpty());
}

void TestUpdateSettings::automaticCheckRunsAtMostOncePerInterval()
{
	const QDateTime now = QDateTime::currentDateTime();
	UpdateSettings s;
	s.setLastCheck(now.addSecs(-23 * 3600));
	QVERIFY2(!s.isAutomaticCheckDue(now), "checked again after 23 hours");

	s.setLastCheck(now.addSecs(-25 * 3600));
	QVERIFY2(s.isAutomaticCheckDue(now), "did not check after 25 hours");

	s.setLastCheck(now.addSecs(-24 * 3600));
	QVERIFY(s.isAutomaticCheckDue(now));
}

void TestUpdateSettings::automaticCheckIsDueWhenNeverRun()
{
	UpdateSettings s;
	QVERIFY(s.isAutomaticCheckDue(QDateTime::currentDateTime()));
}

void TestUpdateSettings::automaticCheckIsDueAfterTheClockMovesBackwards()
{
	/* A stored timestamp in the future would otherwise block checks forever. */
	const QDateTime now = QDateTime::currentDateTime();
	UpdateSettings s;
	s.setLastCheck(now.addSecs(3600));
	QVERIFY(s.isAutomaticCheckDue(now));
}

void TestUpdateSettings::automaticCheckIsNeverDueWhenDisabled()
{
	UpdateSettings s;
	s.setAutoCheckEnabled(false);
	QVERIFY(!s.isAutomaticCheckDue(QDateTime::currentDateTime()));
}

void TestUpdateSettings::skippingOneVersionDoesNotSilenceLaterOnes()
{
	UpdateSettings s;
	s.setSkippedVersion(Version::fromString(QLatin1String("0.7.3")));

	QVERIFY(s.isVersionSkipped(Version::fromString(QLatin1String("0.7.3"))));
	QVERIFY(s.isVersionSkipped(Version::fromString(QLatin1String("0.7.3.0"))));
	QVERIFY(!s.isVersionSkipped(Version::fromString(QLatin1String("0.7.4"))));
	QVERIFY(!s.isVersionSkipped(Version::fromString(QLatin1String("0.8.0"))));

	s.clearSkippedVersion();
	QVERIFY(!s.isVersionSkipped(Version::fromString(QLatin1String("0.7.3"))));
}

void TestUpdateSettings::intervalIsClampedToSaneValues()
{
	UpdateSettings s;
	s.setCheckIntervalHours(0);
	QVERIFY(s.checkIntervalHours() >= 1);
	s.setCheckIntervalHours(-5);
	QVERIFY(s.checkIntervalHours() >= 1);
	s.setCheckIntervalHours(100000);
	QVERIFY(s.checkIntervalHours() <= 24 * 30);
}

void TestUpdateSettings::survivesARoundTripThroughQSettings()
{
	const QDateTime when = QDateTime::fromString(
		QLatin1String("2026-09-01T10:00:00"), Qt::ISODate);

	UpdateSettings written;
	written.setAutoCheckEnabled(false);
	written.setCheckIntervalHours(12);
	written.setLastCheck(when);
	written.setSkippedVersion(Version::fromString(QLatin1String("0.9.1")));

	QSettings settings;
	written.write(settings);
	settings.sync();

	UpdateSettings read;
	read.read(settings);

	QCOMPARE(read.isAutoCheckEnabled(), false);
	QCOMPARE(read.checkIntervalHours(), 12);
	QCOMPARE(read.lastCheck(), when);
	QCOMPARE(read.skippedVersion(), QString("0.9.1"));
}

void TestUpdateSettings::ignoresACorruptStoredVersion()
{
	QSettings settings;
	settings.beginGroup(QLatin1String(UpdateSettings::SETTINGS_GROUP));
	settings.setValue(QLatin1String("SkippedVersion"), QLatin1String("not-a-version"));
	settings.endGroup();
	settings.sync();

	UpdateSettings s;
	s.read(settings);
	QVERIFY2(s.skippedVersion().isEmpty(), "a corrupt stored version was trusted");
}

QTEST_MAIN(TestUpdateSettings)
#include "tst_updatesettings.moc"
