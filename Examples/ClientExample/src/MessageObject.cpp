#include "MessageObject.h"

#include <iostream>

#include <raymath.h>

MessageObject::MessageObject()
{
}
MessageObject::~MessageObject()
{
}

void MessageObject::Init(Vector2 position, std::string text, float lifeTime, float decayOffset, unsigned int fontSize, Color color)
{
	data.m_postion = position;
	data.m_startPosition = position;
	data.m_text = text;

	data.m_lifeTime = lifeTime;
	data.m_decayOffset = decayOffset;
	data.m_clock = 0.0f;

	data.m_fontSize = fontSize;
	data.m_color = color;

	m_timeLeft = lifeTime;
}

bool MessageObject::Update(double deltaTime)
{
	if (!InUse())
		return false;

	const Vector2 endPosition = { (float)data.m_postion.x, -(float)data.m_fontSize };
	float percent = 1.0f - (m_timeLeft / data.m_lifeTime);
	data.m_postion = Vector2Lerp(data.m_startPosition, endPosition, percent);

	data.m_clock += (float)deltaTime;
	if (data.m_clock > data.m_decayOffset)
		m_timeLeft -= (float)deltaTime;

	data.m_color.a = (unsigned char)((m_timeLeft / data.m_lifeTime) * 255);

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
