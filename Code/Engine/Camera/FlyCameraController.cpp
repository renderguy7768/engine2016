#include "FlyCameraController.h"
#include "../UserInput/UserInput.h"
#include "../Time/Time.h"
#include "../StringHandler/HashedString.h"
#include "../../Game/Gameplay/Transform.h"

uint32_t const eae6320::Camera::FlyCameraController::classUUID = StringHandler::HashedString("FlyCameraController").GetHash();

void eae6320::Camera::FlyCameraController::UpdatePosition(Gameplay::Transform& io_transform)
{
	Math::cVector localOffset = Math::cVector::zero;

	if (UserInput::IsKeyPressed(VK_UP))
		localOffset += io_transform.m_localAxes.m_forward;
	if (UserInput::IsKeyPressed(VK_DOWN))
		localOffset -= io_transform.m_localAxes.m_forward;
	if (UserInput::IsKeyPressed(VK_RIGHT))
		localOffset += io_transform.m_localAxes.m_right;
	if (UserInput::IsKeyPressed(VK_LEFT))
		localOffset -= io_transform.m_localAxes.m_right;

	const float speed_unitsPerSecond = 100.0f;
	const float offsetModifier = speed_unitsPerSecond * Time::GetElapsedSecondCount_duringPreviousFrame();
	localOffset *= offsetModifier;
	io_transform.m_position += localOffset;
}

void eae6320::Camera::FlyCameraController::UpdateOrientation(Gameplay::Transform& io_transform)const
{
	Math::cVector localOffset = Math::cVector::zero;
	if (UserInput::IsKeyPressed('H'))
		localOffset.y += 1.0f;
	if (UserInput::IsKeyPressed('F'))
		localOffset.y -= 1.0f;
	if (UserInput::IsKeyPressed('G'))
		localOffset.x += 1.0f;
	if (UserInput::IsKeyPressed('T'))
		localOffset.x -= 1.0f;

	const float speed_unitsPerSecond = 100.0f;
	const float offsetModifier = speed_unitsPerSecond * Time::GetElapsedSecondCount_duringPreviousFrame();
	localOffset *= offsetModifier;
	io_transform.m_orientationEular += localOffset;
}
