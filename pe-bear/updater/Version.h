#pragma once

#include <QtCore>

namespace pe_bear {
namespace updater {

/**
 * Strict release-version model used by the updater.
 *
 * Accepts only plain numeric releases, optionally prefixed with 'v':
 *   "v0.7.2", "0.7.2", "0.7.0.4"
 * Anything carrying a prerelease or build-metadata part ("1.0.0-rc1", "1.0.0+7")
 * or any non-numeric component is rejected. Missing trailing components are
 * treated as zero, so "0.7.2" and "0.7.2.0" are equal.
 *
 * The application version number itself is never duplicated here; it is taken
 * from rebear_ver_short.h through Version::current().
 */
class Version
{
public:
	static const int ComponentCount = 4;
	/* guards against overflow and absurd tags */
	static const int MaxComponentDigits = 9;

	/** Parses a tag or version string. Returns an invalid Version on failure. */
	static Version fromString(const QString &text);

	/** The version of the running build, from rebear_ver_short.h. */
	static Version current();

	/** True if the string carries a prerelease or build-metadata suffix. */
	static bool looksLikePrerelease(const QString &text);

	Version();
	Version(int major, int minor, int micro, int patch);

	bool isValid() const { return m_valid; }

	int major() const { return m_parts[0]; }
	int minor() const { return m_parts[1]; }
	int micro() const { return m_parts[2]; }
	int patch() const { return m_parts[3]; }

	/** Canonical form with trailing zero components dropped, e.g. "0.7.2". */
	QString toString() const;

	int compare(const Version &other) const;

	bool operator==(const Version &o) const { return compare(o) == 0; }
	bool operator!=(const Version &o) const { return compare(o) != 0; }
	bool operator<(const Version &o) const { return compare(o) < 0; }
	bool operator>(const Version &o) const { return compare(o) > 0; }
	bool operator<=(const Version &o) const { return compare(o) <= 0; }
	bool operator>=(const Version &o) const { return compare(o) >= 0; }

private:
	int m_parts[ComponentCount];
	bool m_valid;
};

}; // namespace updater
}; // namespace pe_bear
