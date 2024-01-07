#pragma once

class SupportedHashes {
public:
	enum hash_type {
		MD5 = 0,
		SHA1 = 1,
		SHA256,
		CHECKSUM,
		RICH_HDR_MD5,
		IMP_MD5,
		HASHES_NUM
	};
};

