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
		void DoNetworking();

		void PollNetworkMessages();
		void PollConnectionStateChanges();

		void HandleUserCommands();
		bool GetNextCommand(std::string &result);
		void ParseCommand(const std::string &command, std::string *outCommand, std::string *outParams);

		void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo);
		static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo);

		void DoQuitCommand(const std::string parameters);
		void DoKickCommand(const std::string parameters);

	private:
		std::map<std::string, ServerCommandCallback> m_commandCallbacks;

		std::mutex m_mutexCommandQueue;
		std::queue<std::string> m_commandQueue;

		std::thread m_networkThread;
		std::thread m_commandThread;

		ISteamNetworkingSockets *m_interface;

		uint32 m_listenSocket;
		uint32 m_pollGroup;

		std::map<uint32, ClientInfo> m_connectedClients;
		int m_clientCount = 0;

		ConnectedCallback m_connectedCallback;
		DisconnectedCallback m_disconnectedCallback;
		PacketReceivedCallback m_packetReceivedCallback;

		bool m_shouldQuit = false;
		bool m_networking = false;

		static BCNetServer *s_callbackInstance;

	};

}
