#pragma once

#include "Application.h"
#include "Renderer2D.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <string>

#include <Shared.h>

#include <BCNet/IBCNetClient.h>

class Application2D : public aie::Application 
{
public:
	Application2D();
	virtual ~Application2D();

	virtual bool startup() override;
	virtual void shutdown() override;

	virtual void update(float deltaTime) override;
	virtual void draw() override;

	static glm::vec2 ScreenToWorld(glm::vec2 screenPosition);

	static unsigned int GetWindowWidth() { return m_windowWidth; }
	static unsigned int GetWindowHeight() { return m_windowHeight; }

	static float GetExtents() { return m_extents; }

private:
	void PacketReceived(const BCNet::Packet packet);

	void DoEchoCommand(const std::string parameters);

protected:
	aie::Renderer2D *m_2dRenderer = nullptr;
	
	aie::Texture *m_ballTexture = nullptr;

	BCNet::IBCNetClient *m_networkClient = nullptr;

	static unsigned int m_windowWidth;
	static unsigned int m_windowHeight;

	static float m_extents;

};
