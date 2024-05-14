#include "BCNetServer.h"

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

int g_port;

BCNetServer *BCNetServer::s_callbackInstance = nullptr;
BCNetServer::BCNetServer()
{
	srand((unsigned int)time(nullptr)); // Seed RNG.
}
BCNetServer::~BCNetServer()
{
	if (m_networkThread.joinable())
		m_networkThread.join(); // Wait for the thread to finish execution.

	if (m_commandThread.joinable())
		m_commandThread.join(); // Wait for the thread to finish execution.

	m_shouldQuit = true;
	m_networking = false;
}

void BCNetServer::SetConnectedCallback(const ConnectedCallback &callback)
{
	m_connectedCallback = callback;
}

void BCNetServer::SetDisconnectedCallback(const DisconnectedCallback &callback)
{
	m_disconnectedCallback = callback;
}

void BCNetServer::SetPacketReceivedCallback(const PacketReceivedCallback &callback)
{
	m_packetReceivedCallback = callback;
}

void BCNetServer::SetOutputLogCallback(const ServerOutputLogCallback &callback)
{
	m_outputLogCallback = callback;
}

void BCNetServer::Start(const int port)
{
	if (m_networking)
		return;

	if (port <= 0)
		g_port = DEFAULT_SERVER_PORT;
	else
		g_port = port;

	std::cout << "Starting Server..." << std::endl;

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

	// Setup default commands.
	m_commandCallbacks["/quit"] = BIND_COMMAND(BCNetServer::DoQuitCommand);
	m_commandCallbacks["/exit"] = BIND_COMMAND(BCNetServer::DoQuitCommand);
	m_commandCallbacks["/kick"] = BIND_COMMAND(BCNetServer::DoKickCommand);
	std::cout << PrintCommandList() << std::endl;
}

void BCNetServer::Stop()
{
	if (m_shouldQuit == true || m_networking == false) // Shouldn't stop the server when it has already been stopped.
		return;

	if (m_networkThread.joinable())
		m_networkThread.join(); // Wait for the thread to finish execution.

	if (m_commandThread.joinable())
		m_commandThread.join(); // Wait for the thread to finish execution.

	m_shouldQuit = true;
	m_networking = false;
}

std::string BCNetServer::GetLatestOutput()
{
	return m_outputLog.back(); // Back of the queue should always be the latest.
}

// The main network thread function.
void BCNetServer::DoNetworking()
{
	s_callbackInstance = this;

	SteamDatagramErrMsg msg;
	if (!GameNetworkingSockets_Init(nullptr, msg)) // Initialize networking library.
	{
		std::cout << "Error: Failed to initialize GameNetworkingSockets" << std::endl;
		return;
	}

	// Setup listening socket and poll group.
	m_interface = SteamNetworkingSockets();

	SteamNetworkingIPAddr localAddr;
	localAddr.Clear();
	localAddr.m_port = g_port;

	SteamNetworkingConfigValue_t options;
	options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)BCNetServer::SteamNetConnectionStatusChangedCallback);

	m_listenSocket = m_interface->CreateListenSocketIP(localAddr, 1, &options);
	if (m_listenSocket == k_HSteamListenSocket_Invalid)
	{
		std::cout << "Failed to listen on port " << localAddr.m_port << std::endl;
		std::cout << "Error: Invalid Listen Socket" << std::endl;
		return;
	}

	m_pollGroup = m_interface->CreatePollGroup();
	if (m_pollGroup == k_HSteamNetPollGroup_Invalid)
	{
		std::cout << "Failed to listen on port " << localAddr.m_port << std::endl;
		std::cout << "Error: Invalid Poll Group" << std::endl;
		return;
	}

	std::cout << "Server started.." << std::endl;

	// Loop.
	m_networking = true;
	while (!m_shouldQuit)
	{
		if (m_networking)
		{
			PollNetworkMessages();
			PollConnectionStateChanges();
		}
		HandleUserCommands();
		m_networking = !m_shouldQuit;
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Might be fast?
	}

	// Quit.
	std::cout << "Closing all connections..." << std::endl;
	for (auto [clientID, clientData] : m_connectedClients)
	{
		m_interface->CloseConnection(clientID, 0, "Server Shutdown", true);
	}
	m_connectedClients.clear();
	m_clientCount = 0;

	m_interface->CloseListenSocket(m_listenSocket);
	m_listenSocket = k_HSteamListenSocket_Invalid;

	m_interface->DestroyPollGroup(m_pollGroup);
	m_pollGroup = k_HSteamNetPollGroup_Invalid;

	std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait a bit for all connections to close.
	GameNetworkingSockets_Kill();

	std::cout << "Server Shutting down.." << std::endl;
}

void BCNetServer::PollNetworkMessages()
{
	while (m_networking)
	{
		ISteamNetworkingMessage *msg = nullptr;

		int numMsgs = m_interface->ReceiveMessagesOnPollGroup(m_pollGroup, &msg, 1); // Get incoming packets.
		if (numMsgs == 0)
		{
			break; // Break because no packets.
		}
		if (numMsgs < 0)
		{
			std::cout << "Error whilst polling incoming messages" << std::endl;
			m_networking = false;
			break;
		}
		assert(numMsgs == 1 && msg);

		auto itClient = m_connectedClients.find(msg->m_conn); // The client who sent the packet.
		assert(itClient != m_connectedClients.end());

		if (msg->m_cbSize) // Packet is valid.
		{
			Packet packet(msg->m_pData, (size_t)msg->m_cbSize);

			DefaultPacketID id;
			PacketStreamReader packetReader(packet);
			packetReader >> id;
			
			// Handle packet defaults.
			switch (id)
			{
				case DefaultPacketID::PACKET_NICKNAME:
				{
					Packet packet;
					packet.Allocate(1024); // TODO: Could allocate less.

					PacketStreamWriter packetWriter(packet);
					packetWriter.WriteRaw<DefaultPacketID>(DefaultPacketID::PACKET_SERVER);

					std::string nickName;
					packetReader >> nickName;
					// TODO: Empty check doesn't really work properly.
					if (!nickName.empty() || !std::all_of(nickName.begin(), nickName.end(), isspace)) // String isn't empty and string isn't just spaces.
					{
						bool nickNameExists = false;
						for (auto it = m_connectedClients.begin(); it != m_connectedClients.end(); it++)
						{
							if (it->second.nickName == nickName)
							{
								nickNameExists = true;
								break;
							}
						}

						if (nickNameExists)
						{
							packetWriter.WriteString(std::string("Set Nickname Failed: Another user already has this nickname!"));
						}
						else
						{
							std::string s(itClient->second.nickName + " is now " + nickName);
							std::cout << s << std::endl;
							packetWriter.WriteString(s);

							SetClientNickname(msg->m_conn, nickName);
						}
					}
					else
					{
						packetWriter.WriteString(std::string("Set Nickname Failed: No Nickname Provided."));
					}

					SendPacketToAllClients(packetWriter.GetPacket()); // Tell other clients that their nickname has been changed.
					packet.Release();
				} continue;
				case DefaultPacketID::PACKET_WHOSONLINE:
				{
					std::string s = PrintConnectedUsers();
					Packet packet;
					packet.Allocate(1024);

					PacketStreamWriter packetWriter(packet);
					packetWriter.WriteRaw<DefaultPacketID>(DefaultPacketID::PACKET_SERVER);
					packetWriter.WriteString(s);

					SendPacketToClient(msg->m_conn, packet); // Tell client who's online.
					packet.Release();
				} continue;
			}

			if (m_packetReceivedCallback)
				m_packetReceivedCallback(itClient->second, packet); // Do callback.
		}

		msg->Release(); // No longer needed.
	}
}

void BCNetServer::PollConnectionStateChanges()
{
	m_interface->RunCallbacks();
}

void BCNetServer::HandleUserCommands()
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

		std::cout << "Invalid command entered." << std::endl;
	}
}

// Utility.
bool BCNetServer::GetNextCommand(std::string &result)
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
void BCNetServer::ParseCommand(const std::string &command, std::string *outCommand, std::string *outParams)
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

void BCNetServer::AddCustomCommand(std::string command, ServerCommandCallback callback)
{
	m_commandCallbacks[command] = callback;
}

// Gathers all the commands into a string.
std::string BCNetServer::PrintCommandList()
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

// Gathers all the connected users into a string.
std::string BCNetServer::PrintConnectedUsers()
{
	std::stringstream ss;
	ss << "Current Users [" << m_clientCount << "]: " << std::endl;
	int i = 0;
	for (auto [clientID, clientData] : m_connectedClients)
	{
		ss << clientData.nickName;
		if (m_clientCount > 1 && i < m_clientCount - 1)
			ss << ", ";
		else
			ss << " ";
		i++;
	}
	return ss.str();
}

void BCNetServer::SetClientNickname(uint32 clientID, const std::string &nick)
{
	m_connectedClients[clientID].nickName = nick;
	m_interface->SetConnectionName(clientID, nick.c_str());
}

void BCNetServer::SendPacketToClient(uint32 clientID, const Packet &packet, bool reliable)
{
	EResult result = m_interface->SendMessageToConnection(clientID, packet.data, (uint32)packet.size, reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
}

void BCNetServer::SendPacketToAllClients(const Packet &packet, uint32 excludeID, bool reliable)
{
	for (auto [clientID, clientData] : m_connectedClients)
	{
		if (clientID != excludeID)
			SendPacketToClient(clientID, packet, reliable);
	}
}

void BCNetServer::KickClient(uint32 clientID)
{
	auto it = m_connectedClients.find(clientID);
	if (it == m_connectedClients.end())
	{
		std::cout << "Error: Could not kick client because ID [" << (int)clientID << "] is not connected!" << std::endl;
		return;
	}

	std::cout << "Kicked " << it->second.nickName << " [" << (int)clientID << "]" << std::endl;
	m_interface->CloseConnection(clientID, 0, "Kicked by server", false);

	m_connectedClients.erase(clientID);
	m_clientCount--;
}

void BCNetServer::KickClient(const std::string &nickName)
{
	bool found = false;

	HSteamNetConnection clientID;
	for (auto it = m_connectedClients.begin(); it != m_connectedClients.end(); it++)
	{
		if (it->second.nickName == nickName) // There is a connection with that name.
		{
			found = true;
			clientID = it->first;
			break;
		}
	}

	if (found == false)
	{
		std::cout << "Error: Could not kick client because User [" << nickName << "] is not connected!" << std::endl;
		return;
	}

	std::cout << "Kicked " << nickName << " [" << (int)clientID << "]" << std::endl;
	m_interface->CloseConnection(clientID, 0, "Kicked by server", false);

	m_connectedClients.erase(clientID);
	m_clientCount--;
}

void BCNetServer::Log(std::string message)
{
	std::cout << message << std::endl;

	if ((m_outputLog.size() + 1) > m_maxOutputLog)
		m_outputLog.pop(); // Removes oldest message.
	m_outputLog.push(message); // Adds latest message.

	if (m_outputLogCallback)
		m_outputLogCallback(); // Do callback.
}

void BCNetServer::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo)
{
	switch (pInfo->m_info.m_eState)
	{
		case k_ESteamNetworkingConnectionState_None:
		{
		} break;
		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		{
			// Handle client disconnection or local connection error.
			if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected) // Connection has been severed.
			{
				auto itClient = m_connectedClients.find(pInfo->m_hConn);
				assert(itClient != m_connectedClients.end());

				const char *debugAction;
				if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
				{
					debugAction = "problem detected locally";
				}
				else
				{
					debugAction = "closed by peer";
				}

				std::cout << "Connection " << pInfo->m_info.m_szConnectionDescription << " " << debugAction << ", " <<
					pInfo->m_info.m_eEndReason << ": " << pInfo->m_info.m_szEndDebug << std::endl;

				Packet packet;
				packet.Allocate(1024);

				PacketStreamWriter packetWriter(packet);
				packetWriter.WriteRaw<DefaultPacketID>(DefaultPacketID::PACKET_SERVER);
				packetWriter.WriteString(std::string(itClient->second.nickName + " has left."));

				SendPacketToAllClients(packetWriter.GetPacket(), pInfo->m_hConn); // Tell other clients that they have left.
				packet.Release();

				if (m_disconnectedCallback)
					m_disconnectedCallback(itClient->second); // Do callback.

				m_connectedClients.erase(itClient);
				m_clientCount--;
			}
			else
			{
				// Some problem has occured.
				assert(pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
			}

			m_interface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
		} break;
		case k_ESteamNetworkingConnectionState_Connecting:
		{
			// Handle incoming connections.
			assert(m_connectedClients.find(pInfo->m_hConn) == m_connectedClients.end());

			std::cout << "Incoming connection " << pInfo->m_info.m_szConnectionDescription << std::endl;

			if (m_interface->AcceptConnection(pInfo->m_hConn) != k_EResultOK)
			{
				m_interface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
				std::cout << "Incoming connection failed. (was it already closed?)" << std::endl;
				break;
			}

			if (!m_interface->SetConnectionPollGroup(pInfo->m_hConn, m_pollGroup))
			{
				m_interface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
				std::cout << "Failed to set poll group on incoming connection." << std::endl;
				break;
			}

			auto &client = m_connectedClients[pInfo->m_hConn]; // Add client to map and get reference.

			// Setup client defaults.
			client.id = pInfo->m_hConn;

			std::string nick = ("User " + std::to_string(m_clientCount));
			SetClientNickname(pInfo->m_hConn, nick);

			m_clientCount++;

			if (m_connectedCallback)
				m_connectedCallback(client); // Do callback.

		} break;
		case k_ESteamNetworkingConnectionState_Connected:
		{
			auto itClient = m_connectedClients.find(pInfo->m_hConn);
			assert(itClient != m_connectedClients.end());

			// Handle on client connected.
			std::cout << "Client connected. " << pInfo->m_info.m_szConnectionDescription << std::endl;

			// Relay connection info.
			std::string userList = PrintConnectedUsers();
			std::cout << userList << std::endl;

			std::string peerText(itClient->second.nickName + " has connected!");

			// Send who's currently connected to client.
			Packet packet;
			packet.Allocate(1024); // TODO: allocation could probably be less.
			PacketStreamWriter packetWriter(packet);
			packetWriter.WriteRaw<DefaultPacketID>(DefaultPacketID::PACKET_SERVER);
			packetWriter.WriteString(userList);
			SendPacketToClient(pInfo->m_hConn, packetWriter.GetPacket());

			// Update peers on the new connection.
			packet.Zero(); // Reuse instead of reallocating.
			packetWriter = PacketStreamWriter(packet); // Reinit
			packetWriter.WriteRaw<DefaultPacketID>(DefaultPacketID::PACKET_SERVER);
			packetWriter.WriteString(peerText);
			SendPacketToAllClients(packetWriter.GetPacket(), pInfo->m_hConn);
			packet.Release();
		} break;
		default:
		{
		} break;
	}
}

void BCNetServer::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo)
{
	s_callbackInstance->OnSteamNetConnectionStatusChanged(pInfo); // Handle callback.
}

// Default commands.
void BCNetServer::DoQuitCommand(const std::string parameters) // /quit
{
	if (!parameters.empty())
	{
		std::cout << "Warning: Ignoring parameters." << std::endl;
	}

	m_shouldQuit = true;
}

void BCNetServer::DoKickCommand(const std::string parameters) // /kick {-id [ID]} {-user [User Name]}
{
	if (parameters.empty()) // No parameters, don't do anything.
	{
		std::cout << "Command usage: " << std::endl;
		std::cout << "\t" << "/kick -id [ID]" << std::endl;
		std::cout << "\t" << "/kick -user [User Name]" << std::endl;
		return;
	}

	int count;
	char *params[128];
	ParseCommandParameters(parameters, &count, params); // Get individual parameters.

	// Command input should not have both parameters.
	if (strstr(parameters.c_str(), "-user") != nullptr &&
		strstr(parameters.c_str(), "-id") != nullptr)
	{
		std::cout << "Error: Cannot use both parameters (-user & -id) at once!" << std::endl;
		return;
	}

	// Handle command parameters.
	for (int i = 0; i < count; i++)
	{
		if (strcmp(params[i], "-user") == 0)
		{
			i++;
			if (i >= count)
				continue;

			const char *name = params[i];
			std::cout << "Kicked User: " << name << std::endl;
			KickClient(name);
			continue;
		}
		else if (strcmp(params[i], "-id") == 0)
		{
			i++;
			if (i >= count)
				continue;

			uint32 id = (uint32)std::stoul(params[i]);
			KickClient(id);
			std::cout << "Kicked User (ID: " << id << ")" << std::endl;
			continue;
		}

		std::cout << "Warning: Unknown parameter specified \"" << params[i] << "\"" << std::endl;
	}
}
