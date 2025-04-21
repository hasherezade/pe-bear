#pragma once

class SupportedHashes {
public:
	enum hash_type {
		CHECKSUM = 0,
		RICH_HDR_MD5,
		IMP_MD5,
		MD5,
		SHA1,
		SHA256,
		HASHES_NUM
	};
};

