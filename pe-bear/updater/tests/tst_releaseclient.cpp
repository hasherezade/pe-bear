/*
 * Covers release discovery of PR #2 (UT-11, UT-12, SEC-08, SEC-09).
 *
 * Everything here is exercised against the pure parsing and policy functions:
 * no live request is made, so the suite is deterministic and runs offline.
 */
#include <QtTest>
#include "../ReleaseClient.h"

using namespace pe_bear::updater;

namespace {

	const char* VALID_DIGEST = "c9aeff95175eed76d47a8dad7d20d531fd1f318ccd202220841813a55e21e4fe";

	QByteArray releasePayload(const QString &tag, bool draft, bool prerelease,
		const QString &assetName, const QString &digest,
		const QString &downloadUrl = QString())
	{
		QJsonObject asset;
		asset.insert(QLatin1String("name"), assetName);
		asset.insert(QLatin1String("size"), 1062380);
		asset.insert(QLatin1String("browser_download_url"), downloadUrl.isEmpty()
			? QLatin1String("https://github.com/hasherezade/pe-bear/releases/download/v0.7.2/") + assetName
			: downloadUrl);
		if (!digest.isNull()) {
			asset.insert(QLatin1String("digest"), digest);
		}
		QJsonArray assets;
		assets.append(asset);

		QJsonObject root;
		root.insert(QLatin1String("tag_name"), tag);
		root.insert(QLatin1String("draft"), draft);
		root.insert(QLatin1String("prerelease"), prerelease);
		root.insert(QLatin1String("published_at"), QLatin1String("2026-06-05T20:23:57Z"));
		root.insert(QLatin1String("html_url"),
			QLatin1String("https://github.com/hasherezade/pe-bear/releases/tag/v0.7.2"));
		root.insert(QLatin1String("assets"), assets);
		return QJsonDocument(root).toJson(QJsonDocument::Compact);
	}

}; // namespace

class TestReleaseClient : public QObject
{
	Q_OBJECT

private slots:
	void parsesDigest_data();
	void parsesDigest();

	void acceptsAStableRelease();
	void rejectsDrafts();
	void rejectsPrereleases();
	void rejectsPrereleaseTags();
	void rejectsMissingOrEmptyTag();
	void rejectsNonJson();
	void rejectsOversizedPayload();
	void recordsMalformedDigestWithoutLosingTheRelease();
	void dropsAssetsFromUnexpectedHosts();
	void dropsAssetNamesThatCouldEscapeADirectory();

	void allowsOnlyTheExpectedHosts_data();
	void allowsOnlyTheExpectedHosts();

	void requestCarriesNothingIdentifying();
	void apiUrlPointsAtTheConfiguredRepository();
	void limitsAreSetToTheDocumentedValues();
};

void TestReleaseClient::parsesDigest_data()
{
	QTest::addColumn<QString>("raw");
	QTest::addColumn<QString>("expected");
	QTest::addColumn<bool>("malformed");

	QTest::newRow("valid") << (QLatin1String("sha256:") + QLatin1String(VALID_DIGEST))
		<< QString(VALID_DIGEST) << false;
	QTest::newRow("uppercase hex") << QString("sha256:AABBCCDDEEFF00112233445566778899AABBCCDDEEFF00112233445566778899")
		<< QString("aabbccddeeff00112233445566778899aabbccddeeff00112233445566778899") << false;
	QTest::newRow("absent") << QString() << QString() << false;
	QTest::newRow("empty") << QString("") << QString() << false;
	QTest::newRow("no prefix") << QString(VALID_DIGEST) << QString() << true;
	QTest::newRow("wrong algorithm") << QString("sha1:aabbccddeeff00112233445566778899aabbccdd")
		<< QString() << true;
	QTest::newRow("too short") << QString("sha256:aabbcc") << QString() << true;
	QTest::newRow("not hex") << QString("sha256:zzbbccddeeff00112233445566778899aabbccddeeff00112233445566778899")
		<< QString() << true;
}

void TestReleaseClient::parsesDigest()
{
	QFETCH(QString, raw);
	QFETCH(QString, expected);
	QFETCH(bool, malformed);

	bool wasMalformed = false;
	const QString digest = ReleaseClient::parseSha256Digest(raw, wasMalformed);
	QCOMPARE(digest, expected);
	QCOMPARE(wasMalformed, malformed);
}

void TestReleaseClient::acceptsAStableRelease()
{
	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;
	const QByteArray payload = releasePayload("v0.7.2", false, false,
		"PE-bear_0.7.2_qt5.15.13_x64_linux.tar.xz",
		QLatin1String("sha256:") + QLatin1String(VALID_DIGEST));

	QVERIFY2(ReleaseClient::parseLatestRelease(payload, release, error, detail), qPrintable(detail));
	QCOMPARE(error, ErrorNone);
	QCOMPARE(release.tagName, QString("v0.7.2"));
	QCOMPARE(release.version.toString(), QString("0.7.2"));
	QCOMPARE(release.assets.size(), 1);
	QCOMPARE(release.assets.first().sha256, QString(VALID_DIGEST));
	QVERIFY(release.assets.first().hasDigest());
	QCOMPARE(release.assets.first().size, Q_INT64_C(1062380));
}

void TestReleaseClient::rejectsDrafts()
{
	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;
	const QByteArray payload = releasePayload("v0.7.3", true, false, "a_0.7.3_qt5_x64_linux.tar.xz",
		QLatin1String("sha256:") + QLatin1String(VALID_DIGEST));

	QVERIFY(!ReleaseClient::parseLatestRelease(payload, release, error, detail));
	QCOMPARE(error, ErrorNoStableRelease);
}

void TestReleaseClient::rejectsPrereleases()
{
	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;
	const QByteArray payload = releasePayload("v0.7.3", false, true, "a_0.7.3_qt5_x64_linux.tar.xz",
		QLatin1String("sha256:") + QLatin1String(VALID_DIGEST));

	QVERIFY(!ReleaseClient::parseLatestRelease(payload, release, error, detail));
	QCOMPARE(error, ErrorNoStableRelease);
}

void TestReleaseClient::rejectsPrereleaseTags()
{
	/* Not flagged as a prerelease by the API, but the tag says otherwise. */
	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;
	const QByteArray payload = releasePayload("v0.8.0-rc1", false, false,
		"a_0.8.0_qt5_x64_linux.tar.xz", QLatin1String("sha256:") + QLatin1String(VALID_DIGEST));

	QVERIFY(!ReleaseClient::parseLatestRelease(payload, release, error, detail));
	QCOMPARE(error, ErrorInvalidVersion);
}

void TestReleaseClient::rejectsMissingOrEmptyTag()
{
	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;

	QVERIFY(!ReleaseClient::parseLatestRelease("{\"assets\":[]}", release, error, detail));
	QCOMPARE(error, ErrorInvalidResponse);

	QVERIFY(!ReleaseClient::parseLatestRelease("{\"tag_name\":\"\",\"assets\":[]}",
		release, error, detail));
	QCOMPARE(error, ErrorInvalidResponse);
}

void TestReleaseClient::rejectsNonJson()
{
	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;

	QVERIFY(!ReleaseClient::parseLatestRelease("<html>not json</html>", release, error, detail));
	QCOMPARE(error, ErrorInvalidResponse);

	QVERIFY(!ReleaseClient::parseLatestRelease("[1,2,3]", release, error, detail));
	QCOMPARE(error, ErrorInvalidResponse);
}

void TestReleaseClient::rejectsOversizedPayload()
{
	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;

	QByteArray huge(ReleaseClient::MAX_RESPONSE_BYTES + 1, 'x');
	QVERIFY(!ReleaseClient::parseLatestRelease(huge, release, error, detail));
	QCOMPARE(error, ErrorResponseTooLarge);
}

void TestReleaseClient::recordsMalformedDigestWithoutLosingTheRelease()
{
	/* One unverifiable asset must not hide the rest of the release. */
	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;
	const QByteArray payload = releasePayload("v0.7.2", false, false,
		"PE-bear_0.7.2_qt5.15.13_x64_linux.tar.xz", QLatin1String("md5:abcdef"));

	QVERIFY(ReleaseClient::parseLatestRelease(payload, release, error, detail));
	QCOMPARE(release.assets.size(), 1);
	QVERIFY(!release.assets.first().hasDigest());
	QVERIFY(release.assets.first().digestMalformed);
}

void TestReleaseClient::dropsAssetsFromUnexpectedHosts()
{
	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;
	const QByteArray payload = releasePayload("v0.7.2", false, false,
		"PE-bear_0.7.2_qt5.15.13_x64_linux.tar.xz",
		QLatin1String("sha256:") + QLatin1String(VALID_DIGEST),
		QLatin1String("https://evil.example.com/pe-bear.tar.xz"));

	QVERIFY(!ReleaseClient::parseLatestRelease(payload, release, error, detail));
	QCOMPARE(error, ErrorInvalidResponse);
	QVERIFY(release.assets.isEmpty());
}

void TestReleaseClient::dropsAssetNamesThatCouldEscapeADirectory()
{
	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;
	const QByteArray payload = releasePayload("v0.7.2", false, false,
		"../../../etc/cron.d/pe-bear.tar.xz",
		QLatin1String("sha256:") + QLatin1String(VALID_DIGEST));

	QVERIFY(!ReleaseClient::parseLatestRelease(payload, release, error, detail));
	QVERIFY(release.assets.isEmpty());
}

void TestReleaseClient::allowsOnlyTheExpectedHosts_data()
{
	QTest::addColumn<QString>("url");
	QTest::addColumn<bool>("allowed");

	QTest::newRow("api over https") << "https://api.github.com/repos/x/y/releases/latest" << true;
	QTest::newRow("api over http") << "http://api.github.com/repos/x/y/releases/latest" << false;
	QTest::newRow("uppercase host") << "https://API.GitHub.com/repos/x/y" << true;
	QTest::newRow("suffix attack") << "https://api.github.com.evil.example/repos/x/y" << false;
	QTest::newRow("prefix attack") << "https://evilapi.github.com.attacker.net/x" << false;
	QTest::newRow("empty host") << "https:///repos/x/y" << false;
	QTest::newRow("file scheme") << "file:///etc/passwd" << false;
	QTest::newRow("userinfo trick") << "https://api.github.com@evil.example/x" << false;
}

void TestReleaseClient::allowsOnlyTheExpectedHosts()
{
	QFETCH(QString, url);
	QFETCH(bool, allowed);
	QCOMPARE(ReleaseClient::isAllowedHost(QUrl(url), ReleaseClient::allowedApiHosts()), allowed);
}

void TestReleaseClient::requestCarriesNothingIdentifying()
{
	const QNetworkRequest request = ReleaseClient::buildRequest(
		ReleaseClient::latestReleaseUrl(QLatin1String(ReleaseClient::DEFAULT_REPOSITORY)));

	const QList<QByteArray> headers = request.rawHeaderList();
	QSet<QByteArray> names;
	for (int i = 0; i < headers.size(); i++) {
		names.insert(headers.at(i).toLower());
	}
	/* Exactly three headers, and the only variable part is the app version. */
	QCOMPARE(names.size(), 3);
	QVERIFY(names.contains("accept"));
	QVERIFY(names.contains("x-github-api-version"));
	QVERIFY(names.contains("user-agent"));
	QVERIFY(!names.contains("cookie"));
	QVERIFY(!names.contains("authorization"));

	const QByteArray agent = request.rawHeader("User-Agent");
	QCOMPARE(agent, QByteArray("PE-bear/") + Version::current().toString().toLatin1());
	/* No machine name, no user name, no path of anything being analysed. */
	QVERIFY(!agent.contains(qgetenv("USER")) || qgetenv("USER").isEmpty());
	QVERIFY(!agent.contains(QSysInfo::machineHostName().toLatin1())
		|| QSysInfo::machineHostName().isEmpty());

	QVERIFY(request.sslConfiguration().peerVerifyMode() == QSslSocket::VerifyPeer);
}

void TestReleaseClient::apiUrlPointsAtTheConfiguredRepository()
{
	const QUrl url = ReleaseClient::latestReleaseUrl(QLatin1String("hasherezade/pe-bear"));
	QCOMPARE(url.toString(),
		QString("https://api.github.com/repos/hasherezade/pe-bear/releases/latest"));
	QVERIFY(ReleaseClient::isAllowedHost(url, ReleaseClient::allowedApiHosts()));
}

void TestReleaseClient::limitsAreSetToTheDocumentedValues()
{
	QCOMPARE(ReleaseClient::MAX_RESPONSE_BYTES, Q_INT64_C(2) * 1024 * 1024);
	QCOMPARE(ReleaseClient::CONNECT_TIMEOUT_MS, 10000);
	QCOMPARE(ReleaseClient::OVERALL_TIMEOUT_MS, 30000);
	QCOMPARE(ReleaseClient::MAX_RETRIES, 2);
}

QTEST_MAIN(TestReleaseClient)
#include "tst_releaseclient.moc"
