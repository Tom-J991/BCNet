#include "Game.h"

#include "Common.h"

#include <BCNet/BCNetUtil.h>

unsigned int Game::m_windowWidth = 0;
unsigned int Game::m_windowHeight = 0;

Game::Game()
{ 
	m_windowWidth = SCREEN_WIDTH;
	m_windowHeight = SCREEN_HEIGHT;

	m_networkClient = BCNet::InitClient();
}
Game::~Game()
{ 
	if (m_networkClient != nullptr)
	{
		delete m_networkClient;
		m_networkClient = nullptr;
	}
}

bool Game::Run(std::string windowTitle, unsigned int windowWidth, unsigned int windowHeight, bool fullscreen)
{
	InitAudioDevice();
	InitWindow(windowWidth, windowHeight, windowTitle.c_str());
	if (fullscreen)
		ToggleFullscreen();
	
	if (!Start())
		return false;

	double lastTime = GetTime();
	while (!WindowShouldClose())
	{
		double nowTime = GetTime();
		double deltaTime = nowTime - lastTime;

		Update(deltaTime);

		BeginDrawing();
		Draw();
		EndDrawing();

		lastTime = nowTime;
	}

	Shutdown();

	CloseWindow();
	CloseAudioDevice();

	return true;
}

bool Game::Start()
{
	// Setup application.
	srand((unsigned int)time(nullptr)); // Set random seed.

	m_networkClient->SetPacketReceivedCallback(BIND_CLIENT_PACKET_RECEIVED_CALLBACK(Game::PacketReceived));

	BCNet::ClientCommandCallback echoCommand = BIND_COMMAND(Game::DoEchoCommand);
	m_networkClient->AddCustomCommand("/echo", echoCommand);
	m_networkClient->Start();

	return true;
}

void Game::Shutdown()
{
	m_networkClient->Stop();
}

void Game::Update(double deltaTime)
{

}

void Game::Draw()
{
	ClearBackground(RAYWHITE);
}

void Game::PacketReceived(const BCNet::Packet packet)
{
	BCNet::PacketStreamReader packetReader(packet);

	PacketID id; 
	packetReader >> id;

	switch (id)
	{
		case (PacketID)BCNet::DefaultPacketID::PACKET_SERVER:
		case PacketID::PACKET_TEXT_MESSAGE:
		{
			// Print packet.
			std::string message;
			packetReader >> message;

			std::cout << message << std::endl;
		} break;
		default:
		{
			std::cout << "Warning: Unhandled Packet" << std::endl;
		} return;
	}
}

void Game::DoEchoCommand(const std::string parameters)
{
	if (m_networkClient->GetConnectionStatus() != BCNet::IBCNetClient::ConnectionStatus::CONNECTED)
	{
		std::cout << "Warning: Client is not connected to a server." << std::endl;
		return;
	}

	if (parameters.empty())
	{
		std::cout << "Command Usage: " << std::endl;
		std::cout << "\t" << "/echo [message]" << std::endl;
		return;
	}

	int count;
	char *params[128];
	BCNet::ParseCommandParameters(parameters, &count, params); // Get individual parameters.

	const char *message = params[0];
	std::string s(message);

	BCNet::Packet packet;
	packet.Allocate(1024);

	BCNet::PacketStreamWriter packetWriter(packet);
	packetWriter << PacketID::PACKET_TEXT_MESSAGE << s;

	m_networkClient->SendPacketToServer(packetWriter.GetPacket());
}
