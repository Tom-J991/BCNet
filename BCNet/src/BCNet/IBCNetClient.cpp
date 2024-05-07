#include <BCNet/IBCNetClient.h>

#include "BCNetClient.h"

using namespace BCNet;

extern "C" BCNET_API IBCNetClient *InitClient()
{
	return new BCNetClient();
}
