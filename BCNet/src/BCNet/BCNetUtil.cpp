#include <BCNet/BCNetUtil.h>

#include "Misc/Utility.h"

#include <iostream>
#include <sstream>

#include <stdio.h>
#include <string.h>

static void SplitLine(char *line, char **outArgs) // Utility used for ParseCommandParameters()
{
	if (outArgs == nullptr)
		return;

	char *tmp = strchr(line, ' ');
	if (tmp)
	{
		*tmp = '\0';
		tmp++;
		while ((*tmp) && (*tmp == ' '))
			tmp++;
		if (*tmp == '\0')
			tmp = nullptr;
	}

	*outArgs = tmp;
}

char *Trim(char *s)
{
	if (!s)
		return nullptr;
	if (!*s)
		return s;

	while (isspace(*s)) s++;

	char *ptr;
	for (ptr = s + strlen(s) - 1; (ptr >= s) && isspace(*ptr); --ptr);
	ptr[1] = '\0';

	return s;
}

// Splits the string of parameters into an array of tokens, parameters enclosed in quotes will be treated as one token.
void BCNet::ParseCommandParameters(const std::string &parameters, int *outCount, char **outArray)
{
	if (outCount == nullptr || outArray == nullptr)
		return;

	const int maxArgs = 128;

	size_t length = parameters.length() + 1;
	char *rawParams = new char[length];
	strcpy_s(rawParams, (length * sizeof(char)), parameters.c_str()); // String to character array.

	char *next = rawParams;
	length = strlen(rawParams);

	// Remove quotes, keeps spaces.
	bool quoted = false;
	for (size_t i = 0; i < length; i++)
	{
		if ((!quoted) && ('"' == rawParams[i]))
		{
			quoted = true;
			rawParams[i] = ' ';
		}
		else if ((quoted) && ('"' == rawParams[i]))
		{
			quoted = false;
			rawParams[i] = ' ';
		}
		else if ((quoted) && (' ' == rawParams[i]))
		{
			rawParams[i] = '\1'; // Sets spaces we want to keep to another character.
		}
	}
	rawParams = Trim(rawParams); // Trim away extra spaces.

	memset(outArray, 0x00, sizeof(char *) * maxArgs);
	*outCount = 1;
	outArray[0] = rawParams;

	while ((nullptr != next) && (*outCount < maxArgs)) // Split the line into tokens.
	{
		SplitLine(next, &(outArray[*outCount]));
		next = outArray[*outCount];
		if (nullptr != outArray[*outCount])
			*outCount += 1;
	}

	for (int j = 0; j < *outCount; j++) // Convert preserved spaces back into actual spaces.
	{
		length = strlen(outArray[j]);
		for (size_t i = 0; i < length; i++)
		{
			if ('\1' == outArray[j][i])
				outArray[j][i] = ' ';
		}
	}
}
