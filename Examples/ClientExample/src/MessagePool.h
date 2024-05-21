#pragma once

#include "MessageObject.h"

#include <string>

#include <raylib.h>

class MessagePool
{
public:
	// Basic object pool.
	MessagePool();
	~MessagePool();

	void Create(Vector2 position, std::string text, float lifeTime, float decayOffset, unsigned int fontSize = 12, Color color = BLACK);

	void Update(double deltaTime);
	void Draw(bool centered = false);

	unsigned int GetActiveCount() { return m_activeMessages; }

private:
	static const unsigned int MAX_SIZE = 12;
	MessageObject m_pool[MAX_SIZE];

	MessageObject *m_firstAvailable = nullptr;

	unsigned int m_activeMessages = 0;

};
