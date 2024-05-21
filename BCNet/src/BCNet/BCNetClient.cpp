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

void BCNetClient::SetOutputLogCallback(const ClientOutputLogCallback &callback)
{
	m_outputLogCallback = callback;
}

void BCNetClient::Start()
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
		// Just gets whatever the user has put into the standard input handle then pushes it into the command queue.
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

void BCNetClient::Stop()
{
	if (m_shouldQuit == true) // Shouldn't stop the client when it has already been stopped.
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
	if (m_networking) // Is already connected, return.
		return;
	if (ipAddress.empty()) // No IP Address entered?
		return;

	int usedPort = port;
	if (port <= 0) // Port is invalid, use default.
		usedPort = DEFAULT_SERVER_PORT;

	SteamNetworkingIPAddr addrServer;
	addrServer.Clear();

	// Check if server address is valid.
	if (addrServer.IsIPv6AllZeros())
	{
		if (!addrServer.ParseString(ipAddress.c_str()))
		{
			Log("Error: Invalid IP address.");
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
	Log("Connecting to server " + std::string(szAddr));

	m_networking = true;

	SteamNetworkingConfigValue_t options;
	options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)BCNetClient::SteamNetConnectionStatusChangedCallback);

	m_connection = m_interface->ConnectByIPAddress(addrServer, 1, &options);
	if (m_connection == k_HSteamNetConnection_Invalid)
	{
		Log("Error: Failed to connect to server.");
		m_connectionStatus = ConnectionStatus::FAILED;
		m_networking = false;
		return;
	}
}

void BCNetClient::CloseConnection()
{
	if (m_networking == false) // Shouldn't disconnect if it's already disconnected.
		return;

	m_networking = false;

	m_interface->CloseConnection(m_connection, 0, "Closed by Client", true);
	m_connection = k_HSteamNetConnection_Invalid;
	m_connectionStatus = ConnectionStatus::DISCONNECTED;
	if (m_disconnectedCallback)
		m_disconnectedCallback();
}

std::string BCNetClient::GetLatestOutput()
{
	// TODO: Fix bug that gives garabage output?
	if (m_outputLog.back().empty())
		return "\0";
	return m_outputLog.back(); // Back of the queue should always be the latest.
}

void BCNetClient::Log(std::string message)
{
	std::cout << message << std::endl;

	if ((m_outputLog.size() + 1) > m_maxOutputLog)
		m_outputLog.pop(); // Removes oldest message.
	m_outputLog.push(message); // Adds latest message.

	if (m_outputLogCallback)
		m_outputLogCallback(); // Do callback.
}

// The main network thread function.
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

	Log("Client started...");

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

		if (msg->m_cbSize) // Packet is valid.
		{
			if (m_packetReceivedCallback)
				m_packetReceivedCallback(Packet(msg->m_pData, (size_t)msg->m_cbSize)); // Do callback.
		}

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
			if (strcmp(command.c_str(), cmd.c_str()) == 0) // Command exists.
			{
				if (callback)
				{
					callback(parameters); // Do command.
					commandFinished = true;
				}
			}
		}
		if (commandFinished)
			continue;

		Log("Invalid command entered.");
	}
}

// Utility.
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

// Utility.
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

void BCNetClient::PushInputAsCommand(std::string input)
{
	m_mutexCommandQueue.lock();
	m_commandQueue.push(input);
	m_mutexCommandQueue.unlock();
}

// Gathers all the commands into a string.
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
				Log("Failed to connect to server. " + std::string(pInfo->m_info.m_szEndDebug));
			}
			else if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally) // Connection error.
			{
				Log("Lost connection with server. " + std::string(pInfo->m_info.m_szEndDebug));
			}
			else
			{
				Log("Disconnected from server. " + std::string(pInfo->m_info.m_szEndDebug)); // Connection severed with server.
			}

			m_interface->CloseConnection(m_connection, 0, nullptr, false);
			m_connection = k_HSteamNetConnection_Invalid;
			m_connectionStatus = ConnectionStatus::DISCONNECTED;

			if (m_disconnectedCallback)
				m_disconnectedCallback(); // Do callback.
		} break;
		case k_ESteamNetworkingConnectionState_Connecting:
		{
			// Handle connecting to server.
		} break;
		case k_ESteamNetworkingConnectionState_Connected:
		{
			// Handle on connected.
			Log("Connected to server");
			m_connectionStatus = ConnectionStatus::CONNECTED;

			if (m_connectedCallback)
				m_connectedCallback(); // Do callback.
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
		Log("Warning: Client is not connected to a server.");
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
		Log("Warning: Client is not connected to a server.");
		return;
	}

	if (parameters.empty())
	{
		Log("Command usage: ");
		Log("\t/nick [Nickname]");
		return;
	}

	int count;
	char *params[128];
	ParseCommandParameters(parameters, &count, params); // Get individual parameters.

	std::string nickname = params[0];

	Packet packet;
	packet.Allocate(1024);

	PacketStreamWriter packetWriter(packet);
	packetWriter.WriteRaw<DefaultPacketID>(DefaultPacketID::PACKET_NICKNAME);
	packetWriter.WriteString(nickname);

	SendPacketToServer(packetWriter.GetPacket());
	packet.Release();
}

void BCNetClient::DoWhosOnlineCommand(const std::string parameters)
{
	if (m_connectionStatus != ConnectionStatus::CONNECTED)
	{
		Log("Warning: Client is not connected to a server.");
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
		Log("Warning: Client is already connected to a server.");
		return;
	}

	if (parameters.empty()) // No parameters, don't do anything.
	{
		Log("Command usage: ");
		Log("\t/connect [IP] [Port]");
		Log("\t/join [IP] [Port]");
		return;
	}

	int count;
	char *params[128];
	ParseCommandParameters(parameters, &count, params); // Get individual parameters.

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
