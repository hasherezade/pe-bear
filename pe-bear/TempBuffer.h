#pragma once

#include <stdio.h>
#include <stdlib.h>

//util:
struct TempBuffer
{
public:
	TempBuffer()
		:buf(NULL), buf_size(0)
	{
	}

	bool init(size_t _buf_size)
	{
		buf = (BYTE*)calloc(_buf_size, 1);
		if (!buf) {
			return false;
		}
		buf_size = _buf_size;
		return true;
	}

	bool init(const BYTE *_buf, size_t _buf_size)
	{
		buf = (BYTE*)calloc(_buf_size, 1);
		if (!buf) {
			return false;
		}
		buf_size = _buf_size;
		::memcpy(buf, _buf, _buf_size);
		return true;
	}

	~TempBuffer()
	{
		if (buf) {
			free(buf);
			buf = NULL;
		}
		buf_size = 0;
	}

	BYTE* getContent() { return buf; }

protected:
	BYTE *buf;
	size_t buf_size;
};
