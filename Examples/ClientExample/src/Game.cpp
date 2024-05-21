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
	// Raylib Boilerplate.
	InitAudioDevice();
	InitWindow(windowWidth, windowHeight, windowTitle.c_str());
	SetTargetFPS(60);
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
	m_networkClient->SetOutputLogCallback(BIND_CLIENT_OUTPUT_LOG_CALLBACK(Game::OutputLog));

	// Add a custom command.
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
	m_outputPool.Update(deltaTime);
	//std::cout << "Active " << m_outputPool.GetActiveCount() << std::endl;

	// Handle user input.
	int key = GetCharPressed();
	while (key > 0)
	{
		// Limit key range so that the user can only input what they should be.
		if ((key >= 32) && (key <= 125) && (m_inputCount < MAX_INPUT))
		{
			m_inputArr[m_inputCount] = (char)key;
			m_inputArr[m_inputCount+1] = '\0'; // Null terminator.
			m_inputCount++;
		}

		key = GetCharPressed();
	}

	if (IsKeyPressed(KEY_BACKSPACE)) // Erase input.
	{
		m_inputCount--;
		if (m_inputCount < 0)
			m_inputCount = 0;
		m_inputArr[m_inputCount] = '\0'; // Null terminator.
	}

	if (IsKeyPressed(KEY_ENTER)) // Enter input as command.
	{
		m_lastInputCount = m_inputCount;
		strncpy_s(m_lastInputArr, m_inputArr, m_inputCount);

		m_networkClient->PushInputAsCommand(std::string(m_inputArr));
		m_inputCount = 0;
		m_inputArr[m_inputCount] = '\0'; // Null terminator.
	}

	if (IsKeyPressed(KEY_UP)) // Get last input.
	{
		m_inputCount = m_lastInputCount;
		strncpy_s(m_inputArr, m_lastInputArr, m_lastInputCount);
	}

	m_frameCount++;
}

void Game::Draw()
{
	ClearBackground(RAYWHITE);

	m_outputPool.Draw(false);

	// Draw whatever the user is typing in.
	DrawText(m_inputArr, 12, SCREEN_HEIGHT-24-12, 24, BLACK);
	if (m_inputCount < MAX_INPUT)
	{
		// Draw flashing underscore.
		if ((m_frameCount / 24) % 2) DrawText("_", 12 + MeasureText(m_inputArr, 24) + 2, SCREEN_HEIGHT-24-12, 24, BLACK);
	}
}

void Game::PacketReceived(const BCNet::Packet packet)
{
	BCNet::PacketStreamReader packetReader(packet);

	PacketID id; 
	packetReader >> id;

	switch (id)
	{
		case (PacketID)BCNet::DefaultPacketID::PACKET_SERVER: // Need this to see messages from server.
		{
			// Print packet.
			std::string message;
			packetReader >> message;

			m_networkClient->Log(message);
		} break;
		case PacketID::PACKET_TEXT_MESSAGE: // Chat message.
		{
			// Print packet.
			std::string message;
			packetReader >> message;

			m_networkClient->Log(message);
		} break;
		default:
		{
			std::cout << "Warning: Unhandled Packet" << std::endl;
		} return;
	}
}

void Game::OutputLog()
{
	// Get whatever the latest output is from the network client and display it.
	m_outputPool.Create({ 12.0f, 12.0f }, m_networkClient->GetLatestOutput(), 1.0f, 1.0f, 16);
}

void Game::DoEchoCommand(const std::string parameters)
{
	if (m_networkClient->GetConnectionStatus() != BCNet::IBCNetClient::ConnectionStatus::CONNECTED)
	{
		m_networkClient->Log("Warning: Client is not connected to a server.");
		return;
	}

	if (parameters.empty())
	{
		m_networkClient->Log("Command Usage: ");
		m_networkClient->Log("\t/echo [message]");
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
