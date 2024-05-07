#pragma once

#include <BCNet/Core/Common.h>

#include <string>
#include <functional>
#include <utility>

#define BIND_SERVER_CONNECTED_CALLBACK(fn) std::bind(&fn, this, std::placeholders::_1)
#define BIND_SERVER_DISCONNECTED_CALLBACK(fn) std::bind(&fn, this, std::placeholders::_1)
#define BIND_SERVER_PACKET_RECEIVED_CALLBACK(fn) std::bind(&fn, this, std::placeholders::_1, std::placeholders::_2)

typedef unsigned int uint32;

namespace BCNet
{
	struct Packet; // Forward Declare.

	struct ClientInfo
	{
		uint32 id; // Their connection ID.
		std::string nickName;
	};

	using ServerCommandCallback = std::function<void(const std::string)>;
	using ConnectedCallback = std::function<void(const ClientInfo &)>;
	using DisconnectedCallback = std::function<void(const ClientInfo &)>;
	using PacketReceivedCallback = std::function<void(const ClientInfo &, const Packet)>;
	
	/// <summary>
	/// Server Interface.
	/// Mainly just to avoid exporting STL containers which causes warnings,
	/// but also to hide internals from the end user.
	/// </summary>
	class BCNET_API IBCNetServer
	{
	public:
		virtual ~IBCNetServer() = default;

		virtual void Start(const int port = -1) = 0;
		virtual void Stop() = 0;

		virtual bool IsRunning() = 0;
		virtual bool IsConnected() = 0;

		virtual void SetConnectedCallback(const ConnectedCallback &callback) = 0;
		virtual void SetDisconnectedCallback(const DisconnectedCallback &callback) = 0;
		virtual void SetPacketReceivedCallback(const PacketReceivedCallback &callback) = 0;

		virtual std::string PrintCommandList() = 0;
		virtual void AddCustomCommand(std::string command, ServerCommandCallback callback) = 0;

		virtual std::string PrintConnectedUsers() = 0;

		virtual void SetClientNickname(uint32 clientID, const std::string &nick) = 0;

		template <typename T>
		void SendDataToClient(uint32 clientID, int id, const T &data, bool reliable = true)
		{
			SendPacketToClient(clientID, Packet(id, &data, sizeof(T)), reliable);
		}

		template <typename T>
		void SendDataToAllClients(int id, const T &data, uint32 excludeID = 0, bool reliable = true)
		{
			SendPacketToAllClients(Packet(id, &data, sizeof(T)), excludeID, reliable);
		}

		virtual void SendPacketToClient(uint32 clientID, const Packet &packet, bool reliable = true) = 0;
		virtual void SendPacketToAllClients(const Packet &packet, uint32 excludeID = 0, bool reliable = true) = 0;

		virtual void KickClient(uint32 clientID) = 0;
		virtual void KickClient(const std::string &nickName) = 0;

	};

	/// <summary>
	/// Instantiates a server object.
	/// </summary>
	/// <returns>Pointer to the Server.</returns>
	extern "C" BCNET_API IBCNetServer *InitServer();

}
