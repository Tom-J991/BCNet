#include "Utility.h"

bool BCNet::StringIsNumber(const std::string &str)
{
	return !str.empty() && std::find_if(str.begin(), str.end(), [](unsigned char c) { return !std::isdigit(c); }) == str.end();
}
