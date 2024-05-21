#include "MessagePool.h"

#include <iostream>

MessagePool::MessagePool()
{
	m_firstAvailable = &m_pool[0];

	for (int i = 0; i < MAX_SIZE - 1; i++)
	{
		m_pool[i].SetNext(&m_pool[i + 1]);
	}
	m_pool[MAX_SIZE-1].SetNext(nullptr);
}
MessagePool::~MessagePool()
{
}

void MessagePool::Create(Vector2 position, std::string text, float lifeTime, float decayOffset, unsigned int fontSize, Color color)
{
	if (m_firstAvailable == nullptr)
		return;

	MessageObject *newMessage = m_firstAvailable;
	m_firstAvailable = newMessage->GetNext();

	newMessage->Init(position, text, lifeTime, decayOffset, fontSize, color, m_activeMessages);

	m_activeMessages++;
}

void MessagePool::Update(double deltaTime)
{
	for (int i = 0; i < MAX_SIZE; i++)
	{
		if (m_pool[i].Update(deltaTime))
		{
			m_pool[i].SetNext(m_firstAvailable);
			m_firstAvailable = &m_pool[i];
			m_activeMessages--;

			for (int j = 0; j < MAX_SIZE; j++)
			{
				if (j != i)
					m_pool[j].DecrementIndex();
			}
		}
	}
}

void MessagePool::Draw(bool centered)
{
	for (int i = 0; i < MAX_SIZE; i++)
	{
		m_pool[i].Draw(centered);
	}
}
