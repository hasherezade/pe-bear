/*
 * Covers the version model of PR #1 (UT-01..UT-05, UT-08).
 *
 * The UT-* identifiers come from the milestone issues; the specification that
 * defines them is not in this repository, so each case below names the
 * behaviour it checks rather than relying on the number alone.
 */
#include <QtTest>
#include "../Version.h"
#include "../../rebear_ver_short.h"

using namespace pe_bear::updater;

class TestVersion : public QObject
{
	Q_OBJECT

private slots:
	void parsesAcceptedForms_data();
	void parsesAcceptedForms();

	void rejectsPrereleaseAndMalformed_data();
	void rejectsPrereleaseAndMalformed();

	void comparesNumericallyNotLexically();
	void treatsMissingTrailingComponentsAsZero();
	void rendersCanonicalString_data();
	void rendersCanonicalString();
	void currentMatchesTheVersionHeader();
	void rejectsAbsurdlyLongComponents();
};

void TestVersion::parsesAcceptedForms_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<int>("major");
	QTest::addColumn<int>("minor");
	QTest::addColumn<int>("micro");
	QTest::addColumn<int>("patch");

	QTest::newRow("tag with v") << "v0.7.2" << 0 << 7 << 2 << 0;
	QTest::newRow("plain") << "0.7.2" << 0 << 7 << 2 << 0;
	QTest::newRow("four components") << "0.7.0.4" << 0 << 7 << 0 << 4;
	QTest::newRow("upper V") << "V1.2.3" << 1 << 2 << 3 << 0;
	QTest::newRow("two components") << "1.2" << 1 << 2 << 0 << 0;
	QTest::newRow("surrounding space") << "  v0.6.7.3 " << 0 << 6 << 7 << 3;
	QTest::newRow("double digits") << "0.10.0" << 0 << 10 << 0 << 0;
}

void TestVersion::parsesAcceptedForms()
{
	QFETCH(QString, input);
	QFETCH(int, major);
	QFETCH(int, minor);
	QFETCH(int, micro);
	QFETCH(int, patch);

	const Version v = Version::fromString(input);
	QVERIFY2(v.isValid(), qPrintable(QString("rejected: ") + input));
	QCOMPARE(v.major(), major);
	QCOMPARE(v.minor(), minor);
	QCOMPARE(v.micro(), micro);
	QCOMPARE(v.patch(), patch);
}

void TestVersion::rejectsPrereleaseAndMalformed_data()
{
	QTest::addColumn<QString>("input");

	QTest::newRow("semver prerelease") << "1.0.0-rc1";
	QTest::newRow("build metadata") << "1.0.0+7";
	QTest::newRow("glued suffix") << "0.7.2rc1";
	QTest::newRow("beta word") << "0.7.2 beta";
	QTest::newRow("tilde") << "0.7.2~1";
	QTest::newRow("empty") << "";
	QTest::newRow("only v") << "v";
	QTest::newRow("single component") << "7";
	QTest::newRow("empty component") << "0..1";
	QTest::newRow("trailing dot") << "0.7.";
	QTest::newRow("five components") << "1.2.3.4.5";
	QTest::newRow("letters") << "zero.seven.two";
	QTest::newRow("negative") << "1.-2.3";
	QTest::newRow("hex") << "0x1.2.3";
}

void TestVersion::rejectsPrereleaseAndMalformed()
{
	QFETCH(QString, input);
	const Version v = Version::fromString(input);
	QVERIFY2(!v.isValid(), qPrintable(QString("accepted: ") + input));
}

void TestVersion::comparesNumericallyNotLexically()
{
	/* the classic trap: "0.10.0" sorts before "0.9.0" as text */
	QVERIFY(Version::fromString("0.10.0") > Version::fromString("0.9.0"));
	QVERIFY(Version::fromString("0.7.0.4") > Version::fromString("0.7.0"));
	QVERIFY(Version::fromString("0.7.2") > Version::fromString("0.7.1"));
	QVERIFY(Version::fromString("1.0.0") > Version::fromString("0.99.99"));
	QVERIFY(Version::fromString("0.9.0") < Version::fromString("0.10.0"));
	QVERIFY(!(Version::fromString("0.7.2") > Version::fromString("0.7.2")));
}

void TestVersion::treatsMissingTrailingComponentsAsZero()
{
	QVERIFY(Version::fromString("0.7.2") == Version::fromString("0.7.2.0"));
	QVERIFY(Version::fromString("1.2") == Version::fromString("1.2.0.0"));
	QVERIFY(Version::fromString("0.7.2") != Version::fromString("0.7.2.1"));
}

void TestVersion::rendersCanonicalString_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QString>("expected");

	QTest::newRow("drops trailing zero") << "0.7.2.0" << "0.7.2";
	QTest::newRow("keeps fourth") << "0.7.0.4" << "0.7.0.4";
	QTest::newRow("pads to three") << "1.2" << "1.2.0";
	QTest::newRow("strips v") << "v0.7.2" << "0.7.2";
}

void TestVersion::rendersCanonicalString()
{
	QFETCH(QString, input);
	QFETCH(QString, expected);
	QCOMPARE(Version::fromString(input).toString(), expected);
}

void TestVersion::currentMatchesTheVersionHeader()
{
	/* The version number must live in exactly one place. */
	const Version fromHeader = Version::fromString(QLatin1String(REBEAR_VERSION_STR));
	QVERIFY(fromHeader.isValid());
	QCOMPARE(Version::current().major(), fromHeader.major());
	QCOMPARE(Version::current().minor(), fromHeader.minor());
	QCOMPARE(Version::current().micro(), fromHeader.micro());
}

void TestVersion::rejectsAbsurdlyLongComponents()
{
	QVERIFY(!Version::fromString("1.2.1234567890").isValid());
	QVERIFY(Version::fromString("1.2.123456789").isValid());
}

QTEST_APPLESS_MAIN(TestVersion)
#include "tst_version.moc"
