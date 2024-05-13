#include "Utility.h"

// Returns whether the string is purely a number or not, no other characters at all.
bool BCNet::StringIsNumber(const std::string &str)
{
	return !str.empty() && std::find_if(str.begin(), str.end(), [](unsigned char c) { return !std::isdigit(c); }) == str.end();
}
