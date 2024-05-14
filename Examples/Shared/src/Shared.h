#pragma once

#include <BCNet/BCNetPacket.h>

enum class PacketID : int
{
	PACKET_INVALID = 0,
	PACKET_TEXT_MESSAGE = 1 + DEFAULT_PACKETS_COUNT,

	PACKET_COUNT
};
