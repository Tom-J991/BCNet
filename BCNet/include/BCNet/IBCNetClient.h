#pragma once

#include <BCNet/Core/Common.h>

#include <string>
#include <functional>
#include <utility>

#define BIND_CLIENT_CONNECTED_CALLBACK(fn) std::bind(&fn, this)
#define BIND_CLIENT_DISCONNECTED_CALLBACK(fn) std::bind(&fn, this)
#define BIND_CLIENT_PACKET_RECEIVED_CALLBACK(fn) std::bind(&fn, this, std::placeholders::_1)

typedef unsigned int uint32;

namespace BCNet
{
	struct Packet; // Forward Declare.
	
	using ClientCommandCallback = std::function<void(const std::string)>;
	using ConnectedCallback = std::function<void()>;
	using DisconnectedCallback = std::function<void()>;
	using PacketReceivedCallback = std::function<void(const Packet)>;

	/// <summary>
	/// Client Interface.
	/// Mainly just to avoid exporting STL containers which causes warnings,
	/// but also to hide internals from the end user.
	/// </summary>
	class BCNET_API IBCNetClient
	{
	public:
		enum class ConnectionStatus
		{
			DISCONNECTED = 0,
			CONNECTING,
			CONNECTED,
			FAILED
		};

	public:
		virtual ~IBCNetClient() = default;

		virtual void Start() = 0;
		virtual void Stop() = 0;

		virtual void ConnectToServer(const std::string &ipAddress, const int port = -1) = 0;
		virtual void CloseConnection() = 0;

		virtual bool IsRunning() = 0;
		virtual bool IsConnected() = 0;

		virtual void SetConnectedCallback(const ConnectedCallback &callback) = 0;
		virtual void SetDisconnectedCallback(const DisconnectedCallback &callback) = 0;
		virtual void SetPacketReceivedCallback(const PacketReceivedCallback &callback) = 0;

		virtual std::string PrintCommandList() = 0;
		virtual void AddCustomCommand(std::string command, ClientCommandCallback callback) = 0;

		template <typename T>
		void SendDataToServer(int id, const T &data, bool reliable = true)
		{
			SendPacketToServer(Packet(id, &data, sizeof(T)), reliable);
		}

		virtual void SendPacketToServer(const Packet &packet, bool reliable = true) = 0;

		virtual ConnectionStatus &GetConnectionStatus() = 0;

	};

	/// <summary>
	/// Instantiates a client object.
	/// </summary>
	/// <returns>Pointer to the Client.</returns>
	extern "C" BCNET_API IBCNetClient *InitClient();

}
