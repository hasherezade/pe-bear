#include <QtTest>
#include "../UpdateChecker.h"

using namespace pe_bear::updater;

namespace {

	/* A release source that answers whatever the test tells it to, when the
	   test tells it to -- so every branch of the checker is reached without
	   a network, and without anything asynchronous. */
	class ScriptedSource : public IReleaseSource
	{
	public:
		ScriptedSource() : m_busy(false), m_fetches(0) {}
		int fetches() const { return m_fetches; }

		virtual void fetchLatest() { m_busy = true; m_fetches++; }
		virtual void cancel() { m_busy = false; }
		virtual bool isBusy() const { return m_busy; }

		void answer(const ReleaseInfo &release) { m_busy = false; emit releaseReady(release); }
		void fail(UpdateError error, const QString &detail = QString())
		{
			m_busy = false;
			emit failed(static_cast<int>(error), detail);
		}
	private:
		bool m_busy;
		int m_fetches;
	};

	ReleaseInfo releaseOf(const char *version)
	{
		ReleaseInfo r;
		r.tagName = QLatin1String("v") + QLatin1String(version);
		r.version = Version::fromString(QLatin1String(version));
		r.htmlUrl = QUrl(QLatin1String("https://github.com/hasherezade/pe-bear/releases/tag/") + r.tagName);
		return r;
	}

	/* One component above whatever this build is, so the test does not
	   depend on the version header. */
	QString newerThanCurrent()
	{
		const Version c = Version::current();
		return Version(c.major(), c.minor() + 1, 0, 0).toString();
	}

	QString olderThanCurrent()
	{
		/* Older than any build that was ever released. */
		return Version(0, 0, 1, 0).toString();
	}

}; // namespace

class TestUpdateChecker : public QObject
{
	Q_OBJECT
private slots:
	void startsIdleAndFetchesOnCheck();
	void aNewerReleaseIsOffered();
	void theSameOrAnOlderReleaseIsUpToDate();
	void aFailureRecordsTheReasonAndStaysOutOfTheWay();
	void aCancelledCheckReturnsToIdle();
	void anIncompleteReleaseIsAnInvalidResponse();
	void aSkippedVersionStaysSilentForAutomaticChecksOnly();
	void skippingWorksOnlyWhenSomethingIsOffered();
	void aCheckThatIsNotDueDoesNotFetch();
	void aSuccessfulCheckRecordsItsTime();
	void aSecondCheckWhileBusyIsIgnored();
	void withoutASourceTheCheckFailsHonestly();
};

void TestUpdateChecker::startsIdleAndFetchesOnCheck()
{
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, NULL);
	QCOMPARE(checker.state(), StateIdle);
	QVERIFY(!checker.isBusy());

	QSignalSpy states(&checker, SIGNAL(stateChanged(int)));
	checker.checkForUpdates(true);
	QCOMPARE(checker.state(), StateChecking);
	QVERIFY(checker.isBusy());
	QCOMPARE(src->fetches(), 1);
	QCOMPARE(states.count(), 1);
}

void TestUpdateChecker::aNewerReleaseIsOffered()
{
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, NULL);
	QSignalSpy available(&checker, SIGNAL(updateAvailable(pe_bear::updater::ReleaseInfo)));
	QSignalSpy upToDate(&checker, SIGNAL(upToDate()));

	checker.checkForUpdates(false);
	src->answer(releaseOf(newerThanCurrent().toLatin1().constData()));

	QCOMPARE(checker.state(), StateUpdateAvailable);
	QCOMPARE(available.count(), 1);
	QCOMPARE(upToDate.count(), 0);
	QVERIFY(checker.release().isValid());
	QCOMPARE(checker.release().version.toString(), newerThanCurrent());
	QCOMPARE(checker.lastError(), ErrorNone);
}

void TestUpdateChecker::theSameOrAnOlderReleaseIsUpToDate()
{
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, NULL);
	QSignalSpy available(&checker, SIGNAL(updateAvailable(pe_bear::updater::ReleaseInfo)));
	QSignalSpy upToDate(&checker, SIGNAL(upToDate()));

	checker.checkForUpdates(true);
	src->answer(releaseOf(Version::current().toString().toLatin1().constData()));
	QCOMPARE(checker.state(), StateUpToDate);
	QCOMPARE(upToDate.count(), 1);
	QVERIFY(!checker.release().isValid());

	checker.checkForUpdates(true);
	src->answer(releaseOf(olderThanCurrent().toLatin1().constData()));
	QCOMPARE(checker.state(), StateUpToDate);
	QCOMPARE(upToDate.count(), 2);
	QCOMPARE(available.count(), 0);
}

void TestUpdateChecker::aFailureRecordsTheReasonAndStaysOutOfTheWay()
{
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, NULL);
	QSignalSpy errors(&checker, SIGNAL(errorOccurred(int, QString)));

	checker.checkForUpdates(false);
	src->fail(ErrorTls, QLatin1String("certificate expired"));

	QCOMPARE(checker.state(), StateFailed);
	QCOMPARE(checker.lastError(), ErrorTls);
	QCOMPARE(checker.lastErrorDetail(), QString("certificate expired"));
	QCOMPARE(errors.count(), 1);
	QVERIFY(!checker.isBusy());
	/* And the next check is allowed, as if nothing had happened. */
	checker.checkForUpdates(false);
	QCOMPARE(checker.state(), StateChecking);
	QCOMPARE(checker.lastError(), ErrorNone);
}

void TestUpdateChecker::aCancelledCheckReturnsToIdle()
{
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, NULL);
	checker.checkForUpdates(false);
	src->fail(ErrorCancelled);
	QCOMPARE(checker.state(), StateIdle);
	QCOMPARE(checker.lastError(), ErrorCancelled);
}

void TestUpdateChecker::anIncompleteReleaseIsAnInvalidResponse()
{
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, NULL);
	checker.checkForUpdates(true);
	ReleaseInfo broken;
	broken.tagName = QLatin1String("v9.9.9");
	/* no version */
	src->answer(broken);
	QCOMPARE(checker.state(), StateFailed);
	QCOMPARE(checker.lastError(), ErrorInvalidResponse);
}

void TestUpdateChecker::aSkippedVersionStaysSilentForAutomaticChecksOnly()
{
	UpdateSettings settings;
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, &settings);
	QSignalSpy available(&checker, SIGNAL(updateAvailable(pe_bear::updater::ReleaseInfo)));
	const ReleaseInfo newer = releaseOf(newerThanCurrent().toLatin1().constData());

	checker.checkForUpdates(false);
	src->answer(newer);
	QCOMPARE(checker.state(), StateUpdateAvailable);
	checker.skipCurrentVersion();
	QCOMPARE(checker.state(), StateIdle);
	QCOMPARE(settings.skippedVersion(), newer.version.toString());

	/* Automatic: silent. */
	checker.checkForUpdates(false);
	src->answer(newer);
	QCOMPARE(checker.state(), StateUpToDate);
	QCOMPARE(available.count(), 1);

	/* Asked for by the user: answered. */
	checker.checkForUpdates(true);
	src->answer(newer);
	QCOMPARE(checker.state(), StateUpdateAvailable);
	QCOMPARE(available.count(), 2);

	/* A later version is offered again even automatically. */
	const Version v = newer.version;
	const ReleaseInfo later = releaseOf(Version(v.major(), v.minor(), v.micro() + 1, 0).toString().toLatin1().constData());
	checker.checkForUpdates(false);
	src->answer(later);
	QCOMPARE(checker.state(), StateUpdateAvailable);
	QCOMPARE(available.count(), 3);
}

void TestUpdateChecker::skippingWorksOnlyWhenSomethingIsOffered()
{
	UpdateSettings settings;
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, &settings);
	checker.skipCurrentVersion();
	QVERIFY(settings.skippedVersion().isEmpty());
	QCOMPARE(checker.state(), StateIdle);
}

void TestUpdateChecker::aCheckThatIsNotDueDoesNotFetch()
{
	UpdateSettings settings;
	settings.setLastCheck(QDateTime::currentDateTime());
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, &settings);
	checker.checkForUpdatesIfDue();
	QCOMPARE(src->fetches(), 0);
	QCOMPARE(checker.state(), StateIdle);

	settings.setAutoCheckEnabled(false);
	settings.setLastCheck(QDateTime());
	checker.checkForUpdatesIfDue();
	QCOMPARE(src->fetches(), 0);

	settings.setAutoCheckEnabled(true);
	checker.checkForUpdatesIfDue();
	QCOMPARE(src->fetches(), 1);
}

void TestUpdateChecker::aSuccessfulCheckRecordsItsTime()
{
	UpdateSettings settings;
	QVERIFY(!settings.lastCheck().isValid());
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, &settings);
	checker.checkForUpdates(false);
	src->fail(ErrorNetwork);
	QVERIFY2(!settings.lastCheck().isValid(), "a failed check is not a check");
	checker.checkForUpdates(false);
	src->answer(releaseOf(Version::current().toString().toLatin1().constData()));
	QVERIFY(settings.lastCheck().isValid());
	QVERIFY(settings.lastCheck().secsTo(QDateTime::currentDateTime()) < 5);
}

void TestUpdateChecker::aSecondCheckWhileBusyIsIgnored()
{
	ScriptedSource *src = new ScriptedSource();
	UpdateChecker checker(src, NULL);
	checker.checkForUpdates(false);
	checker.checkForUpdates(true);
	QCOMPARE(src->fetches(), 1);
}

void TestUpdateChecker::withoutASourceTheCheckFailsHonestly()
{
	UpdateChecker checker(NULL, NULL);
	QSignalSpy errors(&checker, SIGNAL(errorOccurred(int, QString)));
	checker.checkForUpdates(true);
	QCOMPARE(checker.state(), StateFailed);
	QCOMPARE(checker.lastError(), ErrorNetwork);
	QCOMPARE(errors.count(), 1);
}

QTEST_GUILESS_MAIN(TestUpdateChecker)
#include "tst_updatechecker.moc"
