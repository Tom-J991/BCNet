#include <iostream>
#include <string>

#include <Shared.h>

#include <BCNet/IBCNetServer.h>
#include <BCNet/BCNetUtil.h>

BCNet::IBCNetServer *g_server;

void PacketReceived(const BCNet::ClientInfo &clientData, const BCNet::Packet packet)
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
			std::cout << temp << std::endl;
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

void DoEchoCommand(const std::string parameters)
{
	if (parameters.empty())
	{
		std::cout << "Command Usage: " << std::endl;
		std::cout << "\t" << "/echo [message]" << std::endl;
		return;
	}

	int count;
	char *params[128];
	BCNet::ParseCommandParameters(parameters, &count, params); // Get individual parameters.

	char temp[1024];
	const char *message = params[0];
	sprintf_s(temp, "[%s]: %s", "Server", message);
	std::string s(temp);

	std::cout << s << std::endl;

	BCNet::Packet packet;
	packet.Allocate(1024);

	BCNet::PacketStreamWriter packetWriter(packet);
	packetWriter << PacketID::PACKET_TEXT_MESSAGE << s;
	
	g_server->SendPacketToAllClients(packetWriter.GetPacket());

	packet.Release();
}

int main()
{
	g_server = BCNet::InitServer();

	g_server->SetPacketReceivedCallback(PacketReceived);

	BCNet::ServerCommandCallback echoCommand = DoEchoCommand;
	g_server->AddCustomCommand("/echo", echoCommand);

	g_server->Start();
	g_server->Stop();

	if (g_server != nullptr)
	{
		delete g_server;
		g_server = nullptr;
	}

	return 0;
}
