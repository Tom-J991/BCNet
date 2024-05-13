#pragma once

#include <BCNet/Core/Common.h>

#include <string>
#include <functional>
#include <utility>

#define BIND_COMMAND(fn) std::bind(&fn, this, std::placeholders::_1)

// Utility functions that are used throughout the API and can be used by the application.

namespace BCNet
{
	/// <summary>
	/// Parses the std::string of parameters, splitting them and then outputting an array of the tokens, a parameter wrapped in quotes is treated as one token.
	/// </summary>
	/// <param name="parameters">The parameters string.</param>
	/// <param name="outCount">Returns how many parameters there are.</param>
	/// <param name="outArray">Returns the parameter array.</param>
	BCNET_API void ParseCommandParameters(const std::string &parameters, int *outCount, char **outArray);

}
