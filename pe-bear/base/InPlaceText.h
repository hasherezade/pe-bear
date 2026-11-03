#pragma once

#include <QtCore>
#include <bearparser/bearparser.h>

/**
 * Renaming a string that lives inside a PE image: library names, function
 * names, forwarders.
 *
 * Such a string has no storage of its own. It sits at an RVA the header points
 * to, packed against whatever follows (the next name, a thunk table, padding
 * that another tool may have used), so the only space that is certainly the
 * string's own is the bytes it occupies now. A new name longer than that
 * overwrites its neighbours -- which is how renaming one import used to break
 * the table behind it. These helpers state that limit, and write inside it.
 */
namespace in_place_text {

	/** Number of characters the string at textPtr can be replaced by without
	 *  touching a byte that is not its own: its current length, excluding the
	 *  terminator, which stays where it is. */
	inline bufsize_t capacity(AbstractByteBuffer *buf, const char *textPtr)
	{
		if (!buf || !textPtr) return 0;
		const bufsize_t avail = buf->getMaxSizeFromPtr((BYTE*) textPtr);
		bufsize_t len = 0;
		while (len < avail && textPtr[len] != '\0') len++;
		return len;
	}

	/** True when the text fits into a string of the given capacity. */
	inline bool fits(const QString &text, bufsize_t capacity)
	{
		return static_cast<bufsize_t>(text.toStdString().length()) <= capacity;
	}

	/** Writes the text over the string, clearing the rest of its old extent.
	 *  Writes exactly `capacity` bytes at textPtr, never more. Returns false,
	 *  and writes nothing, when the text does not fit or equals the old one. */
	inline bool write(char *textPtr, bufsize_t capacity, const QString &text)
	{
		if (!textPtr) return false;
		const std::string bytes = text.toStdString();
		if (bytes.length() > capacity) return false;
		/* The capacity is the current length, so only an equally long text can be
		   the same text. */
		if (bytes.length() == capacity && ::memcmp(textPtr, bytes.c_str(), capacity) == 0) {
			return false;
		}
		::memset(textPtr, 0, capacity);
		::memcpy(textPtr, bytes.c_str(), bytes.length());
		return true;
	}

}; // namespace in_place_text
