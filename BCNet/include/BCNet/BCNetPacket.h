#pragma once

#include <BCNet/Core/Common.h>

#include <iostream>
#include <string>

#define DEFAULT_PACKETS_COUNT 100

namespace BCNet
{
	/// <summary>
	/// Default packet types.
	/// </summary>
	enum class DefaultPacketID : int
	{
		PACKET_INVALID = 0,

		PACKET_WHOSONLINE = 97, // Who's Online command
		PACKET_NICKNAME = 98, // Nickname command
		PACKET_SERVER = 99, // Server Messages

		PACKET_COUNT = DEFAULT_PACKETS_COUNT
	};

	/// <summary>
	/// An object which contains data that can be sent across a network connection.
	/// </summary>
	struct BCNET_API Packet
	{
		void *data = nullptr; // The actual data.
		size_t size = 0; // The size of the packet.

		Packet() = default;

		/// <summary>
		/// Initializes the packet with the provided data.
		/// </summary>
		Packet(const void *data, const size_t size)
			: data((void*)data)
			, size(size)
		{ }

		/// <summary>
		/// Initializes the packet with the data from another packet.
		/// </summary>
		Packet(const Packet& packet, const size_t size)
			: data(packet.data)
			, size(size)
		{ }

		/// <summary>
		/// Returns the size of the packet.
		/// </summary>
		inline size_t GetSize() const { return size; }

		/// <summary>
		/// Allocates a set amount of data the packet can use.
		/// </summary>
		/// <param name="size">The amount of data in bytes that the packet can use, 1024 by default.</param>
		void Allocate(const size_t size = 1024)
		{
			delete[](uint8_t*)data;
			data = nullptr;

			if (size <= 0)
				return;

			data = new uint8_t[size];
			this->size = size;
		}

		/// <summary>
		/// Cleans up the packet, deleting the data and deallocating.
		/// </summary>
		void Release()
		{
			delete[](uint8_t*)data;
			data = nullptr;
			size = 0;
		}

		/// <summary>
		/// Clears the data.
		/// </summary>
		void Zero()
		{
			if (data)
				memset(data, 0, size);
		}

		/// <summary>
		/// Write the provided data into the packet.
		/// </summary>
		/// <param name="data">The data to write.</param>
		/// <param name="size">The size of the data.</param>
		/// <param name="offset">Where to write the data to.</param>
		void Write(const void *data, size_t size, size_t offset = 0)
		{
			// TODO: Handle overflows.
			memcpy((uint8_t*)data + offset, data, size);
		}

		/// <summary>
		/// Read bytes from the packet.
		/// </summary>
		/// <param name="size">The amount of bytes to read.</param>
		/// <param name="offset">Where to read the bytes from.</param>
		uint8_t *ReadBytes(size_t size, size_t offset) const
		{
			uint8_t *packet = new uint8_t[size];
			memcpy(packet, (uint8_t*)data + offset, size);
			return packet;
		}

		/// <summary>
		/// Read data from the packet.
		/// </summary>
		/// <param name="offset">Where to read from.</param>
		template<typename T>
		T &Read(size_t offset = 0)
		{
			return *(T*)((uint32_t*)data + offset);
		}

		/// <summary>
		/// Read data from the packet.
		/// </summary>
		/// <param name="offset">Where to read from.</param>
		template<typename T>
		const T &Read(size_t offset = 0) const
		{
			return *(T*)((uint32_t*)data + offset);
		}

		/// <summary>
		/// Interpret the data as a specified type.
		/// </summary>
		template <typename T>
		T *As() const
		{
			return (T*)data;
		}

		/// <summary>
		/// Copy the data into this packet.
		/// </summary>
		/// <param name="data">The data to copy.</param>
		/// <param name="size">The size of the data to copy.</param>
		static Packet Copy(const void *data, const size_t size)
		{
			Packet packet;
			packet.Allocate(size);
			memcpy(packet.data, data, size);
			return packet;
		}

		/// <summary>
		/// Copy another packet into this packet.
		/// </summary>
		/// <param name="other">The packet to copy.</param>
		static Packet Copy(const Packet &other)
		{
			Packet packet;
			packet.Allocate(other.size);
			memcpy(packet.data, other.data, other.size);
			return packet;
		}

		// Operator overloads.
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
			return data; // Pretty much returns if there is data in the packet or not.
		}

	};

	// -------------------------- PACKETSTREAMWRITER
	/// <summary>
	/// Utility object, helps writing different types of data into a single packet.
	/// </summary>
	class BCNET_API PacketStreamWriter
	{
	public:
		/// <summary>
		/// Initializes the Packet Stream with the packet to write to.
		/// The provided packet must be allocated.
		/// </summary>
		/// <param name="packet">The packet to write to.</param>
		/// <param name="position">The position to start writing to.</param>
		PacketStreamWriter(Packet packet, size_t position = 0);
		PacketStreamWriter(const PacketStreamWriter &) = delete;
		virtual ~PacketStreamWriter() = default;

		/// <summary>
		/// Writes data into the stream.
		/// </summary>
		/// <param name="data">The data to write.</param>
		/// <param name="size">The size of the data.</param>
		bool WriteData(const char *data, size_t size);

		/// <summary>
		/// Writes the contents of another packet into the stream.
		/// </summary>
		/// <param name="packet">The packet to write.</param>
		/// <param name="writeSize">Whether to write the size of the packet or not.</param>
		void WritePacket(Packet packet, bool writeSize = true);

		/// <summary>
		/// Write a bunch of zeroes.
		/// </summary>
		/// <param name="size">The amount of zeroes to write in bytes.</param>
		void WriteZero(size_t size);

		/// <summary>
		/// Writes a string into the stream.
		/// </summary>
		/// <param name="string">The string to write.</param>
		void WriteString(const std::string &string);

		/// <summary>
		/// Writes a string into the stream.
		/// </summary>
		/// <param name="string">The string to write.</param>
		void WriteString(std::string_view string);

		/// <summary>
		/// Writes data into the stream specified by type.
		/// </summary>
		/// <param name="type">The data to write.</param>
		template <typename T>
		void WriteRaw(const T &type)
		{
			bool success = WriteData((char *)&type, sizeof(T));
			// TODO: Handle failure.
		}

		/// <summary>
		/// Returns whether the packet has data or not.
		/// </summary>
		bool IsStreamGood() const { return (bool)m_packet; }

		/// <summary>
		/// The current position in the stream.
		/// </summary>
		size_t GetStreamPosition() { return m_position; }

		/// <summary>
		/// Sets the stream position.
		/// </summary>
		void SetStreamPosition(size_t position) { m_position = position; }

		/// <summary>
		/// Returns the written packet.
		/// </summary>
		Packet GetPacket() const { return Packet(m_packet, m_position); }

		// Operator overloads.
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
	PacketStreamWriter &operator<<(PacketStreamWriter &writer, const T &type) // Implements operator overload.
	{
		writer.WriteRaw<T>(type);
		return writer;
	}

	// -------------------------- PACKETSTREAMREADER
	/// <summary>
	/// Utility object, helps reading different types of data from a packet.
	/// </summary>
	class BCNET_API PacketStreamReader
	{
	public:
		/// <summary>
		/// Initializes the Packet Stream with the packet to read from.
		/// The specified packet must be allocated.
		/// </summary>
		/// <param name="packet">The packet to read from.</param>
		/// <param name="position">The position to start reading from.</param>
		PacketStreamReader(Packet packet, size_t position = 0);
		PacketStreamReader(const PacketStreamReader &) = delete;
		virtual ~PacketStreamReader() = default;

		/// <summary>
		/// Reads data from the packet and outputs it to the specified destination.
		/// </summary>
		/// <param name="dest">The destination to read the data to.</param>
		/// <param name="size">The size of the data to read.</param>
		bool ReadData(char *dest, size_t size);

		/// <summary>
		/// Reads data from a packet to a packet.
		/// </summary>
		/// <param name="packet">The packet to read to.</param>
		/// <param name="size">The size of the packet being read.</param>
		bool ReadPacket(Packet &packet, size_t size = 0);

		/// <summary>
		/// Reads a string from the packet.
		/// </summary>
		/// <param name="string">The string to read to.</param>
		bool ReadString(std::string &string);

		/// <summary>
		/// Ignore x amount of data from the packet.
		/// </summary>
		/// <param name="size">The amount of data to ignore in bytes.</param>
		/// <returns></returns>
		bool Ignore(size_t size);

		/// <summary>
		/// Reads data from the packet specified by type.
		/// </summary>
		/// <param name="type">The destination to read the data to.</param>
		template <typename T>
		bool ReadRaw(T &type)
		{
			bool success = ReadData((char *)&type, sizeof(T));
			// TODO: Handle failure.
			return success;
		}

		/// <summary>
		/// Returns whether the packet has data or not.
		/// </summary>
		bool IsStreamGood() const { return (bool)m_packet; }

		/// <summary>
		/// The current position in the stream.
		/// </summary>
		size_t GetStreamPosition() { return m_position; }

		/// <summary>
		/// Sets the stream position.
		/// </summary>
		void SetStreamPosition(size_t position) { m_position = position; }

		/// <summary>
		/// Returns the read packet.
		/// </summary>
		Packet GetPacket() const { return Packet(m_packet, m_position); }

		// Operator overloads.
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
	PacketStreamReader &operator>>(PacketStreamReader &reader, T &type) // Implements operator overload.
	{
		reader.ReadRaw<T>(type);
		return reader;
	}

}
