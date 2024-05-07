#include "BCNetClient.h"

#include <BCNet/BCNetUtil.h>
#include "Misc/Utility.h"

#include <iostream>
#include <sstream>
#include <chrono>
#include <random>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

using namespace BCNet;

IBCNetClient *InitClient()
{
	return new BCNetClient();
}

BCNetClient *BCNetClient::s_callbackInstance = nullptr;
BCNetClient::BCNetClient()
{
	srand((unsigned int)time(nullptr)); // Seed RNG.
}
BCNetClient::~BCNetClient()
{
	m_shouldQuit = true;
	m_networking = false;

	if (m_networkThread.joinable())
		m_networkThread.join(); // Wait for the thread to finish execution.

	if (m_commandThread.joinable())
		m_commandThread.join(); // Wait for the thread to finish execution.
}

void BCNetClient::SetConnectedCallback(const ConnectedCallback &callback)
{
	m_connectedCallback = callback;
}

void BCNetClient::SetDisconnectedCallback(const DisconnectedCallback &callback)
{
	m_disconnectedCallback = callback;
}

void BCNetClient::SetPacketReceivedCallback(const PacketReceivedCallback &callback)
{
	m_packetReceivedCallback = callback;
}

void BCNet::BCNetClient::Start()
{
	if (m_networking)
		return;

	std::cout << "Starting Client..." << std::endl;

	if (m_networkThread.joinable())
		m_networkThread.join(); // Wait for the thread to finish execution.
	m_networkThread = std::thread([this]() { DoNetworking(); });

	if (m_commandThread.joinable())
		m_commandThread.join(); // Wait for the thread to finish execution.
	m_commandThread = std::thread([this]()
	{
		while (!m_shouldQuit)
		{
			char szLine[4000];
			if (!fgets(szLine, sizeof(szLine), stdin))
			{
				if (m_shouldQuit)
					return;
				m_shouldQuit = true;
				std::cout << "Error: Failed to read command on stdin." << std::endl;

				break;
			}

			m_mutexCommandQueue.lock();
			m_commandQueue.push(std::string(szLine));
			m_mutexCommandQueue.unlock();
		}
	});

	// Setup Default Commands.
	m_commandCallbacks["/quit"] = BIND_COMMAND(BCNetClient::DoQuitCommand);
	m_commandCallbacks["/exit"] = BIND_COMMAND(BCNetClient::DoQuitCommand);
	m_commandCallbacks["/join"] = BIND_COMMAND(BCNetClient::DoConnectCommand);
	m_commandCallbacks["/connect"] = BIND_COMMAND(BCNetClient::DoConnectCommand);
	m_commandCallbacks["/disconnect"] = BIND_COMMAND(BCNetClient::DoDisconnectCommand);
	m_commandCallbacks["/nick"] = BIND_COMMAND(BCNetClient::DoNickNameCommand);
	m_commandCallbacks["/nickname"] = BIND_COMMAND(BCNetClient::DoNickNameCommand);
	m_commandCallbacks["/whosonline"] = BIND_COMMAND(BCNetClient::DoWhosOnlineCommand);
	m_commandCallbacks["/online"] = BIND_COMMAND(BCNetClient::DoWhosOnlineCommand);
	std::cout << PrintCommandList() << std::endl;
}

void BCNet::BCNetClient::Stop()
{
	if (m_shouldQuit == true)
		return;

	m_shouldQuit = true;

	if (m_networking)
	{
		CloseConnection();
	}

	if (m_networkThread.joinable())
		m_networkThread.join(); // Wait for the thread to finish execution.

	if (m_commandThread.joinable())
		m_commandThread.join(); // Wait for the thread to finish execution.
}

void BCNetClient::ConnectToServer(const std::string &ipAddress, const int port)
{
	if (m_networking)
		return;
	if (ipAddress.empty())
		return;

	int usedPort = port;
	if (port <= 0)
		usedPort = DEFAULT_SERVER_PORT;

	SteamNetworkingIPAddr addrServer;
	addrServer.Clear();

	if (addrServer.IsIPv6AllZeros())
	{
		if (!addrServer.ParseString(ipAddress.c_str()))
		{
			std::cout << "Error: Invalid IP address." << std::endl;
			m_connectionStatus = ConnectionStatus::FAILED;
			return;
		}
		if (addrServer.m_port == 0)
		{
			addrServer.m_port = (uint16)port;
		}
	}

	char szAddr[SteamNetworkingIPAddr::k_cchMaxString];
	addrServer.ToString(szAddr, sizeof(szAddr), true);
	m_connectionStatus = ConnectionStatus::CONNECTING;
	std::cout << "Connecting to server " << szAddr << std::endl;

	m_networking = true;

	SteamNetworkingConfigValue_t options;
	options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)BCNetClient::SteamNetConnectionStatusChangedCallback);

	m_connection = m_interface->ConnectByIPAddress(addrServer, 1, &options);
	if (m_connection == k_HSteamNetConnection_Invalid)
	{
		std::cout << "Error: Failed to connect to server." << std::endl;
		m_connectionStatus = ConnectionStatus::FAILED;
		m_networking = false;
		return;
	}
}

void BCNetClient::CloseConnection()
{
	m_networking = false;

	m_interface->CloseConnection(m_connection, 0, "Closed by Client", true);
	m_connection = k_HSteamNetConnection_Invalid;
	m_connectionStatus = ConnectionStatus::DISCONNECTED;
	if (m_disconnectedCallback)
		m_disconnectedCallback();
}

void BCNetClient::DoNetworking()
{
	s_callbackInstance = this;

	SteamDatagramErrMsg msg;
	if (!GameNetworkingSockets_Init(nullptr, msg)) // Initialize networking library.
	{
		std::cout << "Error: Failed to initialize GameNetworkingSockets" << std::endl;
		return;
	}

	m_interface = SteamNetworkingSockets();

	std::cout << "Client started..." << std::endl;

	// Loop.
	while (!m_shouldQuit)
	{
		if (m_networking)
		{
			PollNetworkMessages();
			PollConnectionStateChanges();
		}
		HandleUserCommands();
		m_networking = !m_shouldQuit;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// Quit.
	CloseConnection();

	GameNetworkingSockets_Kill();
}

void BCNetClient::PollNetworkMessages()
{
	while (m_networking)
	{
		ISteamNetworkingMessage *msg = nullptr;

		int numMsgs = m_interface->ReceiveMessagesOnConnection(m_connection, &msg, 1); // Get incoming packets.
		if (numMsgs == 0)
		{
			break; // Break because no packets.
		}
		if (numMsgs < 0)
		{
			//std::cout << "Error whilst polling incoming messages" << std::endl;
			m_networking = false;
			return;
		}
		assert(numMsgs == 1 && msg);

		if (m_packetReceivedCallback)
			m_packetReceivedCallback(Packet(msg->m_pData, (size_t)msg->m_cbSize));

		msg->Release(); // No longer needed.
	}
}

void BCNetClient::PollConnectionStateChanges()
{
	m_interface->RunCallbacks();
}

void BCNetClient::HandleUserCommands()
{
	std::string input;
	while (!m_shouldQuit && GetNextCommand(input))
	{
		bool commandFinished = false;

		std::string command;
		std::string parameters;
		ParseCommand(input, &command, &parameters);

		for (auto [cmd, callback] : m_commandCallbacks)
		{
			if (strcmp(command.c_str(), cmd.c_str()) == 0)
			{
				if (callback)
				{
					callback(parameters);
					commandFinished = true;
				}
			}
		}
		if (commandFinished)
			continue;

		std::cout << "Invalid command entered." << std::endl;
	}
}

bool BCNetClient::GetNextCommand(std::string &result)
{
	bool input = false;

	m_mutexCommandQueue.lock();
	while (!m_commandQueue.empty() && !input)
	{
		result = m_commandQueue.front();
		m_commandQueue.pop();

		{ // Left trim.
			result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](int c) { return !std::isspace(c); }));
		}
		{ // Right trim.
			result.erase(std::find_if(result.rbegin(), result.rend(), [](int c) { return !std::isspace(c); }).base(), result.end());
		}

		input = !result.empty();
	}
	m_mutexCommandQueue.unlock();

	return input;
}

// Basically just splits the command out into two strings which make up the initial command and it's parameters.
void BCNetClient::ParseCommand(const std::string &command, std::string *outCommand, std::string *outParams)
{
	if (outCommand == nullptr || outParams == nullptr)
		return;

	char *first = nullptr, *next = nullptr; // Temp stuff.
	char *second = nullptr;

	size_t length = command.length() + 1;
	char *wholeCommand = new char[length];
	strcpy_s(wholeCommand, (length * sizeof(char)), command.c_str()); // String to character array.

	first = strtok_s(wholeCommand, " ", &next); // Get first part of string before the first space, this hopefully should always be the command.
	second = strtok_s(nullptr, "", &next); // Get the rest, this is hopefully all the parameters.

	std::string cmd(first);

	std::string params = "";
	if (second != nullptr) // Cause strtok may return null if there's nothing.
		params = std::string(second);

	{ // Left trim.
		cmd.erase(cmd.begin(), std::find_if(cmd.begin(), cmd.end(), [](int c) { return !std::isspace(c); }));
		params.erase(params.begin(), std::find_if(params.begin(), params.end(), [](int c) { return !std::isspace(c); }));
	}
	{ // Right trim.
		cmd.erase(std::find_if(cmd.rbegin(), cmd.rend(), [](int c) { return !std::isspace(c); }).base(), cmd.end());
		params.erase(std::find_if(params.rbegin(), params.rend(), [](int c) { return !std::isspace(c); }).base(), params.end());
	}

	*outCommand = cmd;
	*outParams = params;
}

void BCNetClient::AddCustomCommand(std::string command, ClientCommandCallback callback)
{
	m_commandCallbacks[command] = callback;
}

std::string BCNetClient::PrintCommandList()
{
	std::stringstream ss;
	ss << "Commands: " << std::endl << "\t";
	for (auto it = m_commandCallbacks.begin(); it != m_commandCallbacks.end(); )
	{
		ss << it->first;
		it++;
		if (it != m_commandCallbacks.end())
			ss << ", ";
	}
	return ss.str();
}

void BCNetClient::SendPacketToServer(const Packet &packet, bool reliable)
{
	EResult result = m_interface->SendMessageToConnection(m_connection, packet.data, (uint32_t)packet.size, reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
}

void BCNetClient::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo)
{
	assert(pInfo->m_hConn == m_connection || m_connection == k_HSteamNetConnection_Invalid);

	switch (pInfo->m_info.m_eState)
	{
		case k_ESteamNetworkingConnectionState_None:
		{
		} break;
		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		{
			m_networking = false;
			m_connectionStatus = ConnectionStatus::FAILED;

			// Handle client disconnection or connection error.
			if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting) // Failed to connect.
			{
				std::cout << "Failed to connect to server. " << pInfo->m_info.m_szEndDebug << std::endl;
			}
			else if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally) // Connection error.
			{
				std::cout << "Lost connection with server. " << pInfo->m_info.m_szEndDebug << std::endl;
			}
			else
			{
				std::cout << "Disconnected from server. " << pInfo->m_info.m_szEndDebug << std::endl; // Connection severed with server.
			}

			m_interface->CloseConnection(m_connection, 0, nullptr, false);
			m_connection = k_HSteamNetConnection_Invalid;
			m_connectionStatus = ConnectionStatus::DISCONNECTED;
			if (m_disconnectedCallback)
				m_disconnectedCallback();
		} break;
		case k_ESteamNetworkingConnectionState_Connecting:
		{
			// Handle connecting to server.
		} break;
		case k_ESteamNetworkingConnectionState_Connected:
		{
			// Handle on connected.
			std::cout << "Connected to server" << std::endl;
			m_connectionStatus = ConnectionStatus::CONNECTED;
			if (m_connectedCallback)
				m_connectedCallback();
		} break;
		default:
		{
		} break;
	}
}

void BCNetClient::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo)
{
	s_callbackInstance->OnSteamNetConnectionStatusChanged(pInfo); // Handle callbacks.
}

// Default commands.
void BCNetClient::DoQuitCommand(const std::string parameters) // /quit
{
	if (!parameters.empty())
	{
		std::cout << "Warning: Ignoring parameters." << std::endl;
	}

	m_shouldQuit = true;
}

void BCNetClient::DoDisconnectCommand(const std::string parameters) // /disconnect
{
	if (m_connectionStatus != ConnectionStatus::CONNECTED)
	{
		std::cout << "Warning: Client is not connected to a server." << std::endl;
		return;
	}

	if (!parameters.empty())
	{
		std::cout << "Warning: Ignoring parameters." << std::endl;
	}

	std::cout << "Closing Connection.." << std::endl;
	CloseConnection();
}

void BCNetClient::DoNickNameCommand(const std::string parameters)
{
	if (m_connectionStatus != ConnectionStatus::CONNECTED)
	{
		std::cout << "Warning: Client is not connected to a server." << std::endl;
		return;
	}

	if (parameters.empty())
	{
		std::cout << "Command usage: " << std::endl;
		std::cout << "\t" << "/nick [Nickname]" << std::endl;
		return;
	}

	int count;
	char *params[128];
	BCNet::ParseCommandParameters(parameters, &count, params); // Get individual parameters.

	std::string nickname = params[0];

	Packet packet;
	packet.Allocate(1024);

	PacketStreamWriter packetWriter(packet);
	packetWriter.WriteRaw<DefaultPacketID>(DefaultPacketID::PACKET_NICKNAME);
	packetWriter.WriteString(nickname);

	SendPacketToServer(packetWriter.GetPacket());
	packet.Release();
}

void BCNet::BCNetClient::DoWhosOnlineCommand(const std::string parameters)
{
	if (m_connectionStatus != ConnectionStatus::CONNECTED)
	{
		std::cout << "Warning: Client is not connected to a server." << std::endl;
		return;
	}

	if (!parameters.empty())
	{
		std::cout << "Warning: Ignoring parameters." << std::endl;
	}
	
	Packet packet;
	packet.Allocate(1024);

	PacketStreamWriter packetWriter(packet);
	packetWriter.WriteRaw<DefaultPacketID>(DefaultPacketID::PACKET_WHOSONLINE);

	SendPacketToServer(packetWriter.GetPacket());
	packet.Release();
}

void BCNetClient::DoConnectCommand(const std::string parameters) // /connect [IP] [Port], /join [IP] [Port]
{
	if (m_connectionStatus == ConnectionStatus::CONNECTED)
	{
		std::cout << "Warning: Client is already connected to a server." << std::endl;
		return;
	}

	if (parameters.empty()) // No parameters, don't do anything.
	{
		std::cout << "Command usage: " << std::endl;
		std::cout << "\t" << "/connect [IP] [Port]" << std::endl;
		std::cout << "\t" << "/join [IP] [Port]" << std::endl;
		return;
	}

	int count;
	char *params[128];
	BCNet::ParseCommandParameters(parameters, &count, params); // Get individual parameters.

	// Handle command parameters.
	std::string ipAddress = "127.0.0.1";
	int port = DEFAULT_SERVER_PORT;

	if (count > 1)
	{
		std::string portParam = params[1];
		if (StringIsNumber(portParam))
			port = std::stoi(params[1]);
	}

	std::string ipAddressParam = params[0];
	if (strcmp(ipAddressParam.c_str(), "default") != 0)
		ipAddress = ipAddressParam;

	ConnectToServer(ipAddress, port);
}
