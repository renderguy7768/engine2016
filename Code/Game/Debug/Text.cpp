#include "Text.h"
#include "../../Engine/Graphics/SpriteData.h"
#include "../../Engine/Graphics/MeshData.h"
#include "../../Engine/UserSettings/UserSettings.h"
#include "../../Engine/Graphics/Font.h"

namespace
{
	const int16_t fixedHeight = 32;
	const uint8_t widthAdvance = 8;
	const uint8_t space = 2;
	const uint8_t typeOfIndexData = 16;
	const uint8_t verticesPerQuad = 4;
	const uint8_t indicesPerQuad = 6;
	const uint8_t offset = 32;
}

eae6320::Debug::UI::Text::Text(const PixelCoordinates i_pixelCoordinates, const std::string i_text, const Color i_color) :
	m_text(i_text),
	m_color(i_color),
	m_screenPositionForEachCharacter(nullptr),
	m_pixelCoordinates(i_pixelCoordinates),
	m_meshData(nullptr)
{
	Text::Initialize();
}

eae6320::Debug::UI::Text::~Text()
{
	if (m_screenPositionForEachCharacter)
	{
		free(m_screenPositionForEachCharacter);
		m_screenPositionForEachCharacter = nullptr;
	}
	if (m_meshData)
	{
		delete m_meshData;
		m_meshData = nullptr;
	}
}

void eae6320::Debug::UI::Text::Draw()
{
	Graphics::Font::RenderText(*m_meshData);
}

void eae6320::Debug::UI::Text::Update(std::string i_string)
{
	m_text = i_string;
	Initialize();
}

void eae6320::Debug::UI::Text::GetColor(float & i_r, float & i_g, float & i_b) const
{
	i_r = m_color.r;
	i_g = m_color.g;
	i_b = m_color.b;
}

void eae6320::Debug::UI::Text::Initialize()
{
	if (m_screenPositionForEachCharacter)
	{
		free(m_screenPositionForEachCharacter);
		m_screenPositionForEachCharacter = nullptr;
	}
	if (m_meshData)
	{
		delete m_meshData;
		m_meshData = nullptr;
	}
	m_numberOfCharacters = m_text.length();
	m_screenPositionForEachCharacter = reinterpret_cast<Graphics::Sprite::ScreenPosition*>(malloc(m_numberOfCharacters * sizeof(Graphics::Sprite::ScreenPosition)));
	const float widthMultiplier = 2.0f / UserSettings::GetResolutionWidth();
	const float heightMultiplier = 2.0f / UserSettings::GetResolutionHeight();
	for (size_t i = 0; i < m_numberOfCharacters; i++)
	{
		m_screenPositionForEachCharacter[i].top = m_pixelCoordinates.y*heightMultiplier;
		m_screenPositionForEachCharacter[i].bottom = (m_pixelCoordinates.y - fixedHeight)*heightMultiplier;
		if (i > 0)
		{
			m_screenPositionForEachCharacter[i].left = m_screenPositionForEachCharacter[i - 1].right + space * widthMultiplier;
		}
		else
		{
			m_screenPositionForEachCharacter[i].left = m_pixelCoordinates.x*widthMultiplier;
		}
		m_screenPositionForEachCharacter[i].right = m_screenPositionForEachCharacter[i].left + ((static_cast<int16_t>(Graphics::Font::widthArray[m_text[i] - 32]) + widthAdvance)*widthMultiplier);
	}
	m_meshData = new Graphics::MeshData(typeOfIndexData, verticesPerQuad*m_numberOfCharacters, indicesPerQuad*m_numberOfCharacters);
	for (size_t i = 0; i < m_numberOfCharacters; i++)
	{
		m_meshData->vertexData[i * verticesPerQuad + 0].AddVertexData(
			m_screenPositionForEachCharacter[i].left,
			m_screenPositionForEachCharacter[i].bottom,
			-1.0f,
			Graphics::Font::texcoordArray[m_text[i] - offset].left,
			Graphics::Font::texcoordArray[m_text[i] - offset].bottom);

		m_meshData->vertexData[i * verticesPerQuad + 1].AddVertexData(
			m_screenPositionForEachCharacter[i].right,
			m_screenPositionForEachCharacter[i].bottom,
			-1.0f,
			Graphics::Font::texcoordArray[m_text[i] - offset].right,
			Graphics::Font::texcoordArray[m_text[i] - offset].bottom);

		m_meshData->vertexData[i * verticesPerQuad + 2].AddVertexData(
			m_screenPositionForEachCharacter[i].left,
			m_screenPositionForEachCharacter[i].top,
			-1.0f,
			Graphics::Font::texcoordArray[m_text[i] - offset].left,
			Graphics::Font::texcoordArray[m_text[i] - offset].top);

		m_meshData->vertexData[i * verticesPerQuad + 3].AddVertexData(
			m_screenPositionForEachCharacter[i].right,
			m_screenPositionForEachCharacter[i].top,
			-1.0f,
			Graphics::Font::texcoordArray[m_text[i] - offset].right,
			Graphics::Font::texcoordArray[m_text[i] - offset].top);
	}
	for (size_t i = 0; i < m_numberOfCharacters; i++)
	{
		reinterpret_cast<uint16_t*>(m_meshData->indexData)[i * indicesPerQuad + 0] = static_cast<uint16_t>(i * verticesPerQuad + 0);
		reinterpret_cast<uint16_t*>(m_meshData->indexData)[i * indicesPerQuad + 1] = static_cast<uint16_t>(i * verticesPerQuad + 1);
		reinterpret_cast<uint16_t*>(m_meshData->indexData)[i * indicesPerQuad + 2] = static_cast<uint16_t>(i * verticesPerQuad + 2);
		reinterpret_cast<uint16_t*>(m_meshData->indexData)[i * indicesPerQuad + 3] = static_cast<uint16_t>(i * verticesPerQuad + 3);
		reinterpret_cast<uint16_t*>(m_meshData->indexData)[i * indicesPerQuad + 4] = static_cast<uint16_t>(i * verticesPerQuad + 2);
		reinterpret_cast<uint16_t*>(m_meshData->indexData)[i * indicesPerQuad + 5] = static_cast<uint16_t>(i * verticesPerQuad + 1);
	}
}