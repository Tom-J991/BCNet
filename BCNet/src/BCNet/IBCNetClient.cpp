#include <BCNet/IBCNetClient.h>

#include "BCNetClient.h"

using namespace BCNet;

// Implement function from interface header.
extern "C" BCNET_API IBCNetClient *InitClient()
{
	return new BCNetClient();
}
