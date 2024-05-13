#pragma once

#include <BCNet/Core/Common.h>

#include <BCNet/IBCNetServer.h>
#include <BCNet/BCNetPacket.h>

#include <string>
#include <map>
#include <queue>
#include <thread>
#include <mutex>

// Foward Declare.
struct SteamNetConnectionStatusChangedCallback_t;
class ISteamNetworkingSockets;

typedef unsigned int uint32;

namespace BCNet
{
	// Implements the server interface.
	// Most methods are explained in the interface header, don't really wanna repeat it here.
	class BCNetServer : public IBCNetServer
	{
	public:
		BCNetServer();
		virtual ~BCNetServer() override;

		virtual void Start(const int port = -1) override;
		virtual void Stop() override;

		virtual bool IsRunning() override { return !m_shouldQuit; }
		virtual bool IsConnected() override { return m_networking; }

		virtual void SetConnectedCallback(const ConnectedCallback &callback) override;
		virtual void SetDisconnectedCallback(const DisconnectedCallback &callback) override;
		virtual void SetPacketReceivedCallback(const PacketReceivedCallback &callback) override;

		virtual std::string PrintCommandList() override;
		virtual void AddCustomCommand(std::string command, ServerCommandCallback callback) override;

		virtual std::string PrintConnectedUsers() override;

		virtual void SetClientNickname(uint32 clientID, const std::string &nick) override;

		virtual void SendPacketToClient(uint32 clientID, const Packet &packet, bool reliable = true) override;
		virtual void SendPacketToAllClients(const Packet &packet, uint32 excludeID = 0, bool reliable = true) override;

		virtual void KickClient(uint32 clientID) override;
		virtual void KickClient(const std::string &nickName) override;

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
		void DoKickCommand(const std::string parameters);

	private:
		std::map<std::string, ServerCommandCallback> m_commandCallbacks;

		std::mutex m_mutexCommandQueue; // Thread stuff.
		std::queue<std::string> m_commandQueue;

		std::thread m_networkThread; // Does networking stuff.
		std::thread m_commandThread; // Does command stuff.

		ISteamNetworkingSockets *m_interface; // GameNetworkingSockets
		uint32 m_listenSocket;
		uint32 m_pollGroup;

		std::map<uint32, ClientInfo> m_connectedClients; // <HSteamNetConnection, ClientInfo>
		int m_clientCount = 0;

		ConnectedCallback m_connectedCallback;
		DisconnectedCallback m_disconnectedCallback;
		PacketReceivedCallback m_packetReceivedCallback;

		bool m_shouldQuit = false; // Whether the network thread is running.
		bool m_networking = false; // Whether the server is running.

		static BCNetServer *s_callbackInstance;

	};

}
