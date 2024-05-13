#include <BCNet/IBCNetServer.h>

#include "BCNetServer.h"

using namespace BCNet;

// Implement function from interface header.
extern "C" BCNET_API IBCNetServer *InitServer()
{
	return new BCNetServer();
}
