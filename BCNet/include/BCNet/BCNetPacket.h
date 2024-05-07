#pragma once

#include <BCNet/Core/Common.h>

#include <iostream>
#include <string>

#define DEFAULT_PACKETS_COUNT 100

namespace BCNet
{
	// Based off of:
	// https://github.com/StudioCherno/Walnut/blob/7e478a3828059eafabc31cb0559a31d1d83dad12/Walnut/Source/Walnut/Core/Buffer.h
	// https://github.com/StudioCherno/Walnut/blob/7e478a3828059eafabc31cb0559a31d1d83dad12/Walnut/Source/Walnut/Serialization/BufferStream.h

	enum class DefaultPacketID : int
	{
		PACKET_INVALID = 0,

		PACKET_WHOSONLINE = 97, // Who's Online command
		PACKET_NICKNAME = 98, // Nickname command
		PACKET_SERVER = 99, // Server Messages

		PACKET_COUNT = DEFAULT_PACKETS_COUNT
	};

	struct BCNET_API Packet
	{
		void *data = nullptr;
		size_t size = 0;

		Packet() = default;
		Packet(const void *data, const size_t size)
			: data((void*)data)
			, size(size)
		{ }
		Packet(const Packet& packet, const size_t size)
			: data(packet.data)
			, size(size)
		{ }

		inline size_t GetSize() const { return size; }

		void Allocate(const size_t size = 1024)
		{
			delete[](uint8_t*)data;
			data = nullptr;

			if (size <= 0)
				return;

			data = new uint8_t[size];
			this->size = size;
		}

		void Release()
		{
			delete[](uint8_t*)data;
			data = nullptr;
			size = 0;
		}

		void Zero()
		{
			if (data)
				memset(data, 0, size);
		}

		void Write(const void *data, size_t size, size_t offset = 0)
		{
			// TODO: Handle overflows.
			memcpy((uint8_t*)data + offset, data, size);
		}

		uint8_t *ReadBytes(size_t size, size_t offset) const
		{
			uint8_t *packet = new uint8_t[size];
			memcpy(packet, (uint8_t*)data + offset, size);
			return packet;
		}

		template<typename T>
		T &Read(size_t offset = 0)
		{
			return *(T*)((uint32_t*)data + offset);
		}

		template<typename T>
		const T &Read(size_t offset = 0) const
		{
			return *(T*)((uint32_t*)data + offset);
		}

		template <typename T>
		T *As() const
		{
			return (T*)data;
		}

		static Packet Copy(const void *data, const size_t size)
		{
			Packet packet;
			packet.Allocate(size);
			memcpy(packet.data, data, size);
			return packet;
		}
		static Packet Copy(const Packet &other)
		{
			Packet packet;
			packet.Allocate(other.size);
			memcpy(packet.data, other.data, other.size);
			return packet;
		}

		uint8_t &operator[](int index)
		{
			return ((uint8_t*)data)[index];
		}
		
		uint8_t operator[](int index) const
		{
			return ((uint8_t*)data)[index];
		}

		operator bool() const
		{
			return data;
		}

	};

	// -------------------------- PACKETSTREAMWRITER
	class BCNET_API PacketStreamWriter
	{
	public:
		PacketStreamWriter(Packet packet, size_t position = 0);
		PacketStreamWriter(const PacketStreamWriter &) = delete;
		virtual ~PacketStreamWriter() = default;

		bool WriteData(const char *data, size_t size);

		void WritePacket(Packet packet, bool writeSize = true);
		void WriteZero(size_t size);
		void WriteString(const std::string &string);
		void WriteString(std::string_view string);

		template <typename T>
		void WriteRaw(const T &type)
		{
			bool success = WriteData((char *)&type, sizeof(T));
			// TODO: Handle failure.
		}

		bool IsStreamGood() const { return (bool)m_packet; }
		size_t GetStreamPosition() { return m_position; }
		void SetStreamPosition(size_t position) { m_position = position; }

		Packet GetPacket() const { return Packet(m_packet, m_position); }

		operator bool() const
		{
			return IsStreamGood();
		}

		template <typename T>
		friend PacketStreamWriter &operator<<(PacketStreamWriter &writer, const T &type);
		
		BCNET_API friend PacketStreamWriter &operator<<(PacketStreamWriter &writer, Packet packet);
		BCNET_API friend PacketStreamWriter &operator<<(PacketStreamWriter &writer, const std::string &string);
		BCNET_API friend PacketStreamWriter &operator<<(PacketStreamWriter &writer, std::string_view string);

	private:
		Packet m_packet;
		size_t m_position = 0;

	};

	template<typename T>
	PacketStreamWriter &operator<<(PacketStreamWriter &writer, const T &type)
	{
		writer.WriteRaw<T>(type);
		return writer;
	}

	// -------------------------- PACKETSTREAMREADER
	class BCNET_API PacketStreamReader
	{
	public:
		PacketStreamReader(Packet packet, size_t position = 0);
		PacketStreamReader(const PacketStreamReader &) = delete;
		virtual ~PacketStreamReader() = default;

		bool ReadData(char *dest, size_t size);

		bool ReadPacket(Packet &packet, size_t size = 0);
		bool ReadString(std::string &string);

		bool Ignore(size_t size);

		template <typename T>
		bool ReadRaw(T &type)
		{
			bool success = ReadData((char *)&type, sizeof(T));
			// TODO: Handle failure.
			return success;
		}

		bool IsStreamGood() const { return (bool)m_packet; }
		size_t GetStreamPosition() { return m_position; }
		void SetStreamPosition(size_t position) { m_position = position; }

		Packet GetPacket() const { return Packet(m_packet, m_position); }

		operator bool() const
		{
			return IsStreamGood();
		}

		template <typename T>
		friend PacketStreamReader &operator>>(PacketStreamReader &reader, T &type);

		BCNET_API friend PacketStreamReader &operator>>(PacketStreamReader &reader, Packet &packet);
		BCNET_API friend PacketStreamReader &operator>>(PacketStreamReader &reader, std::string &string);

	private:
		Packet m_packet;
		size_t m_position = 0;

	};

	template<typename T>
	PacketStreamReader &operator>>(PacketStreamReader &reader, T &type)
	{
		reader.ReadRaw<T>(type);
		return reader;
	}

}
