#pragma once

#include <BCNet/Core/Common.h>

#include <string>
#include <functional>
#include <utility>

#define BIND_SERVER_CONNECTED_CALLBACK(fn) std::bind(&fn, this, std::placeholders::_1)
#define BIND_SERVER_DISCONNECTED_CALLBACK(fn) std::bind(&fn, this, std::placeholders::_1)
#define BIND_SERVER_PACKET_RECEIVED_CALLBACK(fn) std::bind(&fn, this, std::placeholders::_1, std::placeholders::_2)
#define BIND_SERVER_OUTPUT_LOG_CALLBACK(fn) std::bind(&fn, this)

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
	using ServerOutputLogCallback = std::function<void()>;
	using ConnectedCallback = std::function<void(const ClientInfo &)>;
	using DisconnectedCallback = std::function<void(const ClientInfo &)>;
	using PacketReceivedCallback = std::function<void(const ClientInfo &, const Packet)>;
	
	/// <summary>
	/// Server Interface.
	/// Mainly just to avoid exporting STL containers which causes warnings,
	/// but also to hide internals from the end user.
	/// This is what the server application should be using.
	/// </summary>
	class BCNET_API IBCNetServer
	{
	public:
		virtual ~IBCNetServer() = default;

		/// <summary>
		/// Starts the server. The default port is 5456.
		/// </summary>
		/// <param name="port">The port the server will listen to.</param>
		virtual void Start(const int port = -1) = 0;

		/// <summary>
		/// Stops the server.
		/// </summary>
		virtual void Stop() = 0;

		/// <summary>
		/// Is the network thread running?
		/// </summary>
		virtual bool IsRunning() = 0;

		/// <summary>
		/// Is the server networking? 
		/// </summary>
		virtual bool IsConnected() = 0;

		/// <summary>
		/// This callback is called whenever a client successfully connects to the server.
		/// The callback function should have a reference to the ClientInfo as a parameter.
		/// </summary>
		virtual void SetConnectedCallback(const ConnectedCallback &callback) = 0;

		/// <summary>
		/// This callback is called whenever a client disconnects from the server.
		/// The callback function should have a reference to the ClientInfo as a parameter.
		/// </summary>
		virtual void SetDisconnectedCallback(const DisconnectedCallback &callback) = 0;

		/// <summary>
		/// This callback is called whenever the client logs a message.
		/// </summary>
		virtual void SetOutputLogCallback(const ServerOutputLogCallback &callback) = 0;

		/// <summary>
		/// This callback is called whenever the server receives a packet.
		/// The callback function should have a reference to the ClientInfo as a parameter, as well as a copy of the Received Packet.
		/// </summary>
		virtual void SetPacketReceivedCallback(const PacketReceivedCallback &callback) = 0;

		/// <summary>
		/// Returns a string describing all the commands the user can use to interact with the server.
		/// </summary>
		virtual std::string PrintCommandList() = 0;

		/// <summary>
		/// Adds a custom command which the user can execute.
		/// </summary>
		/// <param name="command">The command the user must type, e.g. "/command".</param>
		/// <param name="callback">The callback which the command will call, the command parameters are passed to the callback as an std::string.</param>
		virtual void AddCustomCommand(std::string command, ServerCommandCallback callback) = 0;

		/// <summary>
		/// Returns a string listing all of the currently connected clients.
		/// </summary>
		virtual std::string PrintConnectedUsers() = 0;

		/// <summary>
		/// Sets a clients nickname.
		/// </summary>
		/// <param name="clientID">The ID of the client to rename.</param>
		/// <param name="nick">The new nickname.</param>
		virtual void SetClientNickname(uint32 clientID, const std::string &nick) = 0;

		/// <summary>
		/// Sends the provided data through to the specified client.
		/// </summary>
		/// <param name="clientID">The ID of the client who will receive the data.</param>
		/// <param name="data">The data to send.</param>
		/// <param name="reliable">Whether the connection is reliable or not.</param>
		template <typename T>
		void SendDataToClient(uint32 clientID, const T &data, bool reliable = true)
		{
			SendPacketToClient(clientID, Packet(&data, sizeof(T)), reliable);
		}

		/// <summary>
		/// Sends the provided data through to all connected clients, except excluded.
		/// </summary>
		/// <param name="data">The data to send.</param>
		/// <param name="excludeID">The ID of whoever shouldn't recieve the data.</param>
		/// <param name="reliable">Whether the connection is reliable or not.</param>
		template <typename T>
		void SendDataToAllClients(const T &data, uint32 excludeID = 0, bool reliable = true)
		{
			SendPacketToAllClients(Packet(&data, sizeof(T)), excludeID, reliable);
		}

		/// <summary>
		/// Sends a packet to the specified client.
		/// </summary>
		/// <param name="clientID">The ID of the client who will receive the packet.</param>
		/// <param name="packet">The packet to send.</param>
		/// <param name="reliable">Whether the connection is reliable or not.</param>
		virtual void SendPacketToClient(uint32 clientID, const Packet &packet, bool reliable = true) = 0;

		/// <summary>
		/// Sends a packet to all connected clients, except excluded.
		/// </summary>
		/// <param name="packet">The packet to send.</param>
		/// <param name="excludeID">The ID of whoever shouldn't recieve the packet.</param>
		/// <param name="reliable">Whether the connection is reliable or not.</param>
		virtual void SendPacketToAllClients(const Packet &packet, uint32 excludeID = 0, bool reliable = true) = 0;

		/// <summary>
		/// Kicks a connected client, severing their connection.
		/// </summary>
		/// <param name="clientID">The ID of the client to kick.</param>
		virtual void KickClient(uint32 clientID) = 0;

		/// <summary>
		/// Kicks a connected client, severing their connection.
		/// </summary>
		/// <param name="nickName">The nickname of the client to kick.</param>
		virtual void KickClient(const std::string &nickName) = 0;

		/// <summary>
		/// Logs and outputs a message.
		/// </summary>
		virtual void Log(std::string message) = 0;

		/// <summary>
		/// Sets the maximum amount of messages the server will log. The default maximum is 12.
		/// </summary>
		virtual void SetMaxOutputLog(unsigned int max) = 0;

		/// <summary>
		/// Gets the latest message from the server's output log.
		/// </summary>
		virtual std::string GetLatestOutput() = 0;

		/// <summary>
		/// Gets how many clients are currently connected to the server.
		/// </summary>
		virtual unsigned int GetConnectedCount() = 0;

	};

	/// <summary>
	/// Instantiates a server object.
	/// The application must use this to initialize a new server.
	/// </summary>
	/// <returns>A pointer to the server object.</returns>
	extern "C" BCNET_API IBCNetServer *InitServer();

}
