#include <iostream>
#include <string>

#include <Shared.h>

#include <BCNet/IBCNetServer.h>
#include <BCNet/BCNetUtil.h>

BCNet::IBCNetServer *g_server;

void PacketReceived(const BCNet::ClientInfo &clientData, const BCNet::Packet packet) // Packet received callback.
{
	BCNet::PacketStreamReader packetReader(packet);

	PacketID id;
	packetReader >> id;

	switch (id)
	{
		case PacketID::PACKET_TEXT_MESSAGE:
		{
			// Print packet.
			char temp[1024];

			std::string message;
			packetReader >> message;
			const char *msg = message.c_str();

			sprintf_s(temp, "[%s]: %s", clientData.nickName.c_str(), msg);
			g_server->Log(temp);
			std::string s(temp);

			BCNet::Packet packet;
			packet.Allocate(1024);

			BCNet::PacketStreamWriter packetWriter(packet);
			packetWriter << PacketID::PACKET_TEXT_MESSAGE << s;

			g_server->SendPacketToAllClients(packetWriter.GetPacket());

			packet.Release();
		} break;
		default:
		{
			std::cout << "Warning: Unhandled Packet" << std::endl;
		} return;
	}
}

void DoEchoCommand(const std::string parameters) // Echo command implementation.
{
	if (parameters.empty())
	{
		g_server->Log("Command Usage : ");
		g_server->Log("\t/echo[message]");
		return;
	}

	int count;
	char *params[128];
	BCNet::ParseCommandParameters(parameters, &count, params); // Get individual parameters.

	char temp[1024];
	const char *message = params[0];
	sprintf_s(temp, "[%s]: %s", "Server", message);
	std::string s(temp);

	g_server->Log(s);

	BCNet::Packet packet;
	packet.Allocate(1024);

	BCNet::PacketStreamWriter packetWriter(packet);
	packetWriter << PacketID::PACKET_TEXT_MESSAGE << s;
	
	g_server->SendPacketToAllClients(packetWriter.GetPacket()); // Send message to all connected clients.

	packet.Release();
}

// ----------------- Entry point.
int main()
{
	g_server = BCNet::InitServer();

	// Setup callbacks.
	g_server->SetPacketReceivedCallback(PacketReceived);

	// Add custom commands.
	BCNet::ServerCommandCallback echoCommand = DoEchoCommand;
	g_server->AddCustomCommand("/echo", echoCommand);

	// Set max clients.
	g_server->SetMaxClients(2);

	// Run Server.
	g_server->Start();
	g_server->Stop(); // g_server->Start() calls a loop so we can do this immediately after without really any problems.

	if (g_server != nullptr)
	{
		delete g_server;
		g_server = nullptr;
	}

	return 0;
}
