#include "PckrSign.h"

#include <iomanip>
#include <sstream>


namespace sig_ma {
	namespace util {

		std::string to_hex(const uint8_t val)
		{
			std::stringstream ss;
			ss << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(val);
			return ss.str();
		}

	};
};

bool sig_ma::PckrSign::addNode(uint8_t val, sig_type type)
{
	nodes.push_back(SigNode(val, type));

	const size_t kMaxPreview = 255;
	if (nodes.size() < kMaxPreview) {
		// add to the content preview:
		if (type == IMM) {
			signContent += util::to_hex(val) + " ";
		}
		else if (type == WILDC) {
			signContent += "?? ";
		}
	}
	else if (nodes.size() == kMaxPreview) {
		signContent += "...";
	}
	return true;
}
