#pragma once

#include <BCNet/Core/Common.h>

#include <string>
#include <functional>
#include <utility>

#define BIND_COMMAND(fn) std::bind(&fn, this, std::placeholders::_1)

namespace BCNet
{
	BCNET_API void ParseCommandParameters(const std::string &parameters, int *outCount, char **outArray);

}
