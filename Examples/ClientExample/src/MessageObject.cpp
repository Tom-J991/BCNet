#include "MessageObject.h"

#include <iostream>

#include <raymath.h>

MessageObject::MessageObject()
{
}
MessageObject::~MessageObject()
{
}

void MessageObject::Init(Vector2 position, std::string text, float lifeTime, float decayOffset, unsigned int fontSize, Color color, unsigned int index)
{
	data.m_postion = position;
	data.m_startPosition = position;
	data.m_text = text;

	data.m_lifeTime = lifeTime;
	data.m_decayOffset = decayOffset;
	data.m_clock = 0.0f;

	data.m_fontSize = fontSize;
	data.m_color = color;

	data.m_index = index;

	m_timeLeft = data.m_lifeTime;
}

bool MessageObject::Update(double deltaTime)
{
	if (!InUse())
		return false;

	float percent = (m_timeLeft / data.m_lifeTime);
	float invPercent = 1.0f - percent;

	data.m_postion = { data.m_startPosition.x, data.m_startPosition.y + (data.m_index * 16.0f) };

	data.m_clock += (float)deltaTime;
	if (data.m_clock > data.m_decayOffset)
		m_timeLeft -= (float)deltaTime;

	data.m_color.a = (unsigned char)(percent * 255);

	return m_timeLeft <= 0.0f;
}

void MessageObject::Draw(bool centered)
{
	if (!InUse())
		return;

	const int textWidth = (MeasureText(data.m_text.c_str(), data.m_fontSize));
	Vector2 offset = { 0, 0 };
	if (centered)
		offset.x = (textWidth/2.0f);

	DrawText(data.m_text.c_str(), (int)data.m_postion.x - (int)offset.x, (int)data.m_postion.y - (int)offset.y, data.m_fontSize, data.m_color);
}
