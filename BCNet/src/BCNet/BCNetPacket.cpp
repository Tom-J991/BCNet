#include <BCNet/BCNetPacket.h>

using namespace BCNet;

// ------------ PACKETSTREAMWRITER
PacketStreamWriter::PacketStreamWriter(Packet packet, size_t position)
	: m_packet(packet)
	, m_position(position)
{ }

bool PacketStreamWriter::WriteData(const char *data, size_t size)
{
	bool valid = m_position + size <= m_packet.size; // Is it outside the range?
	if (!valid)
		return false;

	memcpy(m_packet.As<uint8_t>() + m_position, data, size); // Write into position.
	m_position += size;

	return true;
}

void PacketStreamWriter::WritePacket(Packet packet, bool writeSize)
{
	if (writeSize)
		WriteData((char*)&packet.size, sizeof(size_t));
	WriteData((char*)packet.data, packet.size);
}

void PacketStreamWriter::WriteZero(size_t size)
{
	char zero = 0;
	for (size_t i = 0; i < size; i++) // Write a bunch of zeroes.
		WriteData(&zero, 1);
}

void PacketStreamWriter::WriteString(const std::string &string)
{
	size_t size = string.size();
	WriteData((char*)&size, sizeof(size_t));
	WriteData((char*)string.data(), sizeof(char) * string.size());
}

// Implements operator overloads.
PacketStreamWriter &BCNet::operator<<(PacketStreamWriter &writer, Packet packet)
{
	writer.WritePacket(packet);
	return writer;
}
PacketStreamWriter &BCNet::operator<<(PacketStreamWriter &writer, const std::string &string)
{
	writer.WriteString(string);
	return writer;
}

// ------------ PACKETSTREAMREADER
PacketStreamReader::PacketStreamReader(Packet packet, size_t position)
	: m_packet(packet)
	, m_position(position)
{ }

bool PacketStreamReader::ReadData(char *dest, size_t size)
{
	bool valid = m_position + size <= m_packet.size; // Is it outside the range?
	if (!valid)
		return false;

	memcpy(dest, m_packet.As<uint8_t>() + m_position, size); // Read into destination.
	m_position += size;

	return true;
}

bool PacketStreamReader::ReadPacket(Packet &packet, size_t size)
{
	packet.size = size;
	if (size <= 0) // Get packet size if it has been written.
	{
		if (!ReadData((char*)&packet.size, sizeof(size_t)))
		{
			return false;
		}
	}

	packet.Allocate(packet.size);
	return ReadData((char*)packet.data, packet.size);
}

bool PacketStreamReader::ReadString(std::string &string)
{
	size_t size;
	if (!ReadData((char*)&size, sizeof(size_t))) // Get string size if it has been written.
		return false;

	string.resize(size);
	return ReadData((char*)string.data(), sizeof(char) * size);
}

bool BCNet::PacketStreamReader::Ignore(size_t size)
{
	bool valid = m_position + size <= m_packet.size; // Is it outside the range?
	if (!valid)
		return false;

	m_position += size; // Just go past it.

	return true;
}

// Implements operator overloads.
PacketStreamReader &BCNet::operator>>(PacketStreamReader &reader, Packet &packet)
{
	reader.ReadPacket(packet);
	return reader;
}
PacketStreamReader &BCNet::operator>>(PacketStreamReader &reader, std::string &string)
{
	reader.ReadString(string);
	return reader;
}
