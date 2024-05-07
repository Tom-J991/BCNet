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

namespace BCNet
{
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

		virtual void SetConnectedCallback(const ConnectedCallback &callback) override;
		virtual void SetDisconnectedCallback(const DisconnectedCallback &callback) override;
		virtual void SetPacketReceivedCallback(const PacketReceivedCallback &callback) override;

		virtual std::string PrintCommandList() override;
		virtual void AddCustomCommand(std::string command, ClientCommandCallback callback) override;

		virtual void SendPacketToServer(const Packet &packet, bool reliable = true) override;

		virtual ConnectionStatus &GetConnectionStatus() override { return m_connectionStatus; }

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
		void DoConnectCommand(const std::string parameters);
		void DoDisconnectCommand(const std::string parameters);
		void DoNickNameCommand(const std::string parameters);
		void DoWhosOnlineCommand(const std::string parameters);

	private:
		std::map<std::string, ClientCommandCallback> m_commandCallbacks;

		std::mutex m_mutexCommandQueue;
		std::queue<std::string> m_commandQueue;

		std::thread m_networkThread;
		std::thread m_commandThread;

		ISteamNetworkingSockets *m_interface;

		uint32 m_connection;
		ConnectionStatus m_connectionStatus = ConnectionStatus::DISCONNECTED;

		ConnectedCallback m_connectedCallback;
		DisconnectedCallback m_disconnectedCallback;
		PacketReceivedCallback m_packetReceivedCallback;

		bool m_shouldQuit = false;
		bool m_networking = false;

		static BCNetClient *s_callbackInstance;

	};

}
