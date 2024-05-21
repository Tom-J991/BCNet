#pragma once

#include <BCNet/Core/Common.h>

#include <BCNet/IBCNetClient.h>
#include <BCNet/BCNetPacket.h>

#include <string>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <utility>

// Forward Declare.
struct SteamNetConnectionStatusChangedCallback_t;
struct SteamNetworkingIPAddr;
class ISteamNetworkingSockets;

typedef unsigned int uint32;

namespace BCNet
{
	// Implements the client interface.
	// Most methods are explained in the interface header, don't really wanna repeat it here.
	class BCNetClient : public IBCNetClient
	{
	public:
		BCNetClient();
		virtual ~BCNetClient() override;

		virtual void Start() override;
		virtual void Stop() override;

		virtual void ConnectToServer(const std::string &ipAddress, const int port = -1) override;
		virtual void CloseConnection() override;

		virtual bool IsRunning() override { return !m_shouldQuit; }
		virtual bool IsConnected() override { return m_networking; }

		virtual void SetConnectedCallback(const ClientConnectedCallback &callback) override;
		virtual void SetDisconnectedCallback(const ClientDisconnectedCallback &callback) override;
		virtual void SetPacketReceivedCallback(const ClientPacketReceivedCallback &callback) override;
		virtual void SetOutputLogCallback(const ClientOutputLogCallback &callback) override;

		virtual std::string PrintCommandList() override;
		virtual void AddCustomCommand(std::string command, ClientCommandCallback callback) override;

		virtual void PushInputAsCommand(std::string input) override;

		virtual void SendPacketToServer(const Packet &packet, bool reliable = true) override;

		virtual ConnectionStatus &GetConnectionStatus() override { return m_connectionStatus; }

		virtual void Log(std::string message) override;

		virtual void SetMaxOutputLog(unsigned int max) override { m_maxOutputLog = max; };

		virtual std::string GetLatestOutput() override;

	private:
		void DoNetworking(); // The main network thread function.

		void PollNetworkMessages(); // Handles incoming messages/packets.
		void PollConnectionStateChanges(); // Handles connection state.

		void HandleUserCommands(); // Handles incoming commands.
		bool GetNextCommand(std::string &result);
		// TODO: Should move into the utility header since it's the same in both the server and client classes.
		void ParseCommand(const std::string &command, std::string *outCommand, std::string *outParams); // Utility.

		// GameNetworkingSockets Callbacks.
		void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo); // Handles connection status.
		static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo);

		// Default command implementations.
		void DoQuitCommand(const std::string parameters);
		void DoConnectCommand(const std::string parameters);
		void DoDisconnectCommand(const std::string parameters);
		void DoNickNameCommand(const std::string parameters);
		void DoWhosOnlineCommand(const std::string parameters);

	private:
		std::map<std::string, ClientCommandCallback> m_commandCallbacks;

		std::queue<std::string> m_outputLog;

		std::mutex m_mutexCommandQueue; // Thread stuff.
		std::queue<std::string> m_commandQueue;

		std::thread m_networkThread; // Does networking stuff.
		std::thread m_commandThread; // Does command stuff.

		ISteamNetworkingSockets *m_interface; // GameNetworkingSockets
		uint32 m_connection; // HSteamNetConnection

		ConnectionStatus m_connectionStatus = ConnectionStatus::DISCONNECTED;

		ClientConnectedCallback m_connectedCallback;
		ClientDisconnectedCallback m_disconnectedCallback;
		ClientPacketReceivedCallback m_packetReceivedCallback;
		ClientOutputLogCallback m_outputLogCallback;

		unsigned int m_maxOutputLog = 12;

		bool m_shouldQuit = false; // Whether the network thread is running.
		bool m_networking = false; // Whether the client is connected.

		static BCNetClient *s_callbackInstance;

	};

}
