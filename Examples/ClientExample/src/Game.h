#pragma once

#include <Shared.h>
#include "MessagePool.h"

#include <string>

#include <BCNet/IBCNetClient.h>
#include <raylib.h>

class Game
{
public:
	Game();
	virtual ~Game();

	bool Run(std::string windowTitle, unsigned int windowWidth, unsigned int windowHeight, bool fullscreen = false);

protected:
	virtual bool Start();
	virtual void Shutdown();

	virtual void Update(double deltaTime);
	virtual void Draw();

	static unsigned int GetWindowWidth() { return m_windowWidth; }
	static unsigned int GetWindowHeight() { return m_windowHeight; }

private:
	void PacketReceived(const BCNet::Packet packet); // Packet received callback.
	void OutputLog(); // Output log callback.

	void DoEchoCommand(const std::string parameters); // Echo command implementation.

protected:
	BCNet::IBCNetClient *m_networkClient = nullptr;

	static unsigned int m_windowWidth;
	static unsigned int m_windowHeight;

	MessagePool m_outputPool;

	constexpr static unsigned int MAX_INPUT = 1024;
	char m_inputArr[MAX_INPUT];
	char m_lastInputArr[MAX_INPUT];
	int m_inputCount = 0;
	int m_lastInputCount = 0;

	int m_frameCount = 0;

};
