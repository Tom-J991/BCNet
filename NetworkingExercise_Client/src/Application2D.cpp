#include "Application2D.h"

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <random>

#include "Common.h"

#include "Gizmos.h"
#include "Texture.h"
#include "Font.h"
#include "Input.h"

#include <BCNet/BCNetUtil.h>

static bool toggleInfoText = true;

static float aspectRatio;

unsigned int Application2D::m_windowWidth = 0;
unsigned int Application2D::m_windowHeight = 0;

float Application2D::m_extents = 0;

Application2D::Application2D()
{ 
	m_windowWidth = SCREEN_WIDTH;
	m_windowHeight = SCREEN_HEIGHT;

	m_extents = 100;

	m_networkClient = BCNet::InitClient();
}
Application2D::~Application2D() 
{ 
	if (m_networkClient != nullptr)
	{
		delete m_networkClient;
		m_networkClient = nullptr;
	}
}

bool Application2D::startup() 
{
	// Setup application.
	setVSync(false);
	srand((unsigned int)time(nullptr)); // Set random seed.

	// Create renderer and scene.
	aspectRatio = getWindowWidth() / (float)getWindowHeight();
	aie::Gizmos::create(255U, 255U, 65535U, 65535U);
	m_2dRenderer = new aie::Renderer2D();

	GLOBALS::g_font = new aie::Font("./font/consolas.ttf", 18);

	m_ballTexture = new aie::Texture("./textures/ball.png");

	m_networkClient->SetPacketReceivedCallback(BIND_CLIENT_PACKET_RECEIVED_CALLBACK(Application2D::PacketReceived));

	BCNet::ClientCommandCallback echoCommand = BIND_COMMAND(Application2D::DoEchoCommand);
	m_networkClient->AddCustomCommand("/echo", echoCommand);
	m_networkClient->Start();

	return true;
}

void Application2D::shutdown() 
{
	delete m_ballTexture;

	delete GLOBALS::g_font;
	GLOBALS::g_font = nullptr;

	delete m_2dRenderer;
	m_2dRenderer = nullptr;

	m_networkClient->Stop();
}

void Application2D::update(float deltaTime) 
{
	aie::Input* input = aie::Input::getInstance();

	aie::Gizmos::clear(); // Clear Gizmos on start of frame.

	// Toggle info text.
	if (input->wasKeyReleased(aie::INPUT_KEY_G))
		toggleInfoText = !toggleInfoText;

	// Toggle debug.
	if (input->wasKeyReleased(aie::INPUT_KEY_P))
		GLOBALS::g_DEBUG = !GLOBALS::g_DEBUG;

	// exit the application
	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE) || m_networkClient->IsRunning() == false)
	{
		quit();
		m_networkClient->Stop();
	}
}

void Application2D::draw() 
{
	m_windowWidth = getWindowWidth();
	m_windowHeight = getWindowHeight();
	aspectRatio = m_windowWidth / (float)m_windowHeight;

	// wipe the screen to the background colour
	clearScreen();

	// begin drawing sprites on foreground
	m_2dRenderer->begin();

	m_2dRenderer->drawSprite(m_ballTexture, 100.0f, 100.0f);

	aie::Gizmos::draw2D(glm::ortho<float>(-m_extents, m_extents, -m_extents / aspectRatio, m_extents / aspectRatio, -1.0f, 1.0f)); // Draw gizmos.

	if (toggleInfoText)
	{
		// output some text, uses the last used colour
		char fps[32];
		sprintf_s(fps, 32, "Application FPS: %i", getFPS());
		m_2dRenderer->setRenderColour(0xFFFFFFFF);
		m_2dRenderer->drawText(GLOBALS::g_font, fps, 0, (float)m_windowHeight - 32);

		m_2dRenderer->drawText(GLOBALS::g_font, "Press ESC to quit!", 0, (float)m_windowHeight - 64);

		if (GLOBALS::g_DEBUG == true)
			m_2dRenderer->drawText(GLOBALS::g_font, "DEBUG VIEW!", 0, (float)m_windowHeight - (64+32));
	}

	// done drawing sprites
	m_2dRenderer->end();
}

glm::vec2 Application2D::ScreenToWorld(glm::vec2 screenPosition)
{
	// Screen space -> World space transformation.
	glm::vec2 worldPosition = screenPosition;
	worldPosition.x -= m_windowWidth / 2;
	worldPosition.y -= m_windowHeight / 2;
	worldPosition.x *= 2.0f * m_extents / m_windowWidth;
	worldPosition.y *= 2.0f * m_extents / (aspectRatio * m_windowHeight);
	return worldPosition;
}

void Application2D::PacketReceived(const BCNet::Packet packet)
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

void Application2D::DoEchoCommand(const std::string parameters)
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
