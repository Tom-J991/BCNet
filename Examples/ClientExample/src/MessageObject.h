#pragma once

#include <string>

#include <raylib.h>

class MessageObject
{
public:
	MessageObject();
	~MessageObject();

	void Init(Vector2 position, std::string text, float lifeTime, float decayOffset, unsigned int fontSize = 12, Color color = BLACK);

	bool Update(double deltaTime);
	void Draw(bool centered = false);

	MessageObject *GetNext() const { return this->next; }
	void SetNext(MessageObject *next) { this->next = next; }

	inline bool InUse() { return m_timeLeft > 0.0f; }

private:
	float m_timeLeft = 0.0f;

	//https://gameprogrammingpatterns.com/object-pool.html#a-free-list
	union
	{
		struct
		{
			Vector2 m_postion = { 0, 0 };
			Vector2 m_startPosition = { 0, 0 };

			std::string m_text = "";

			unsigned int m_fontSize = 12;
			Color m_color = BLACK;

			float m_lifeTime = 1.0f;
			float m_decayOffset = 1.0f;
			float m_clock = 0.0f;
		} data;
		MessageObject *next;
	}; 

};
