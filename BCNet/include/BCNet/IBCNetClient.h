#pragma once

#include <BCNet/Core/Common.h>

#include <string>
#include <functional>
#include <utility>

#define BIND_CLIENT_CONNECTED_CALLBACK(fn) std::bind(&fn, this)
#define BIND_CLIENT_DISCONNECTED_CALLBACK(fn) std::bind(&fn, this)
#define BIND_CLIENT_PACKET_RECEIVED_CALLBACK(fn) std::bind(&fn, this, std::placeholders::_1)
#define BIND_CLIENT_OUTPUT_LOG_CALLBACK(fn) std::bind(&fn, this)

typedef unsigned int uint32;

namespace BCNet
{
	struct Packet; // Forward Declare.
	
	using ClientCommandCallback = std::function<void(const std::string)>;
	using ClientOutputLogCallback = std::function<void()>;
	using ConnectedCallback = std::function<void()>;
	using DisconnectedCallback = std::function<void()>;
	using PacketReceivedCallback = std::function<void(const Packet)>;

	/// <summary>
	/// Client Interface.
	/// Mainly just to avoid exporting STL containers which causes warnings,
	/// but also to hide internals from the end user.
	/// This is what the client application should be using.
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

		/// <summary>
		/// Starts the client.
		/// </summary>
		virtual void Start() = 0;

		/// <summary>
		/// Stops the client.
		/// </summary>
		virtual void Stop() = 0;

		/// <summary>
		/// Connects to a server using the provided address. The default port is 5456.
		/// </summary>
		/// <param name="ipAddress">The address of the server to connect to.</param>
		/// <param name="port">The port the server is listening to.</param>
		virtual void ConnectToServer(const std::string &ipAddress, const int port = -1) = 0;

		/// <summary>
		/// Disconnects if the client is connected to a server.
		/// </summary>
		virtual void CloseConnection() = 0;

		/// <summary>
		/// Is the network thread running?
		/// </summary>
		virtual bool IsRunning() = 0;

		/// <summary>
		/// Is the client networking?
		/// </summary>
		virtual bool IsConnected() = 0;

		/// <summary>
		/// This callback is called when the client successfully connects to the server.
		/// </summary>
		virtual void SetConnectedCallback(const ConnectedCallback &callback) = 0;

		/// <summary>
		/// This callback is called when the client disconnects from the server.
		/// </summary>
		virtual void SetDisconnectedCallback(const DisconnectedCallback &callback) = 0;

		/// <summary>
		/// This callback is called whenever the client receives a packet.
		/// The callback function should have a copy of the received packet as a parameter.
		/// </summary>
		virtual void SetPacketReceivedCallback(const PacketReceivedCallback &callback) = 0;

		/// <summary>
		/// This callback is called whenever the client logs a message.
		/// </summary>
		virtual void SetOutputLogCallback(const ClientOutputLogCallback &callback) = 0;

		/// <summary>
		/// Returns a string describing all the commands the user can use to interact with the client.
		/// </summary>
		virtual std::string PrintCommandList() = 0;

		/// <summary>
		/// Adds a custom command which the user can execute.
		/// </summary>
		/// <param name="command">The command the user must type, e.g. "/command".</param>
		/// <param name="callback">The callback which the command will call, the command parameters are passed to the callback as an std::string.</param>
		virtual void AddCustomCommand(std::string command, ClientCommandCallback callback) = 0;

		/// <summary>
		/// Sends the provided data through to the server.
		/// </summary>
		/// <param name="data">The data to send.</param>
		/// <param name="reliable">Whether the connection is reliable or not.</param>
		template <typename T>
		void SendDataToServer(const T &data, bool reliable = true)
		{
			SendPacketToServer(Packet(&data, sizeof(T)), reliable);
		}

		/// <summary>
		/// Sends a packet to the server.
		/// </summary>
		/// <param name="packet">The packet to send.</param>
		/// <param name="reliable">Whether the connection is reliable or not.</param>
		virtual void SendPacketToServer(const Packet &packet, bool reliable = true) = 0;

		/// <summary>
		/// Returns the current connection status.
		/// </summary>
		virtual ConnectionStatus &GetConnectionStatus() = 0;

		/// <summary>
		/// Logs and outputs a message.
		/// </summary>
		virtual void Log(std::string message) = 0;

		/// <summary>
		/// Sets the maximum amount of messages the client will log. The default maximum is 12.
		/// </summary>
		virtual void SetMaxOutputLog(unsigned int max) = 0;

		/// <summary>
		/// Gets the latest message from the client's output log.
		/// </summary>
		virtual std::string GetLatestOutput() = 0;

	};

	/// <summary>
	/// Instantiates a client object.
	/// The application must use this to initialize a new client.
	/// </summary>
	/// <returns>A pointer to the client object.</returns>
	extern "C" BCNET_API IBCNetClient *InitClient();

}
