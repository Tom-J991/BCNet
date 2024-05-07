#include <BCNet/IBCNetServer.h>

#include "BCNetServer.h"

using namespace BCNet;

extern "C" BCNET_API IBCNetServer *InitServer()
{
	return new BCNetServer();
}
