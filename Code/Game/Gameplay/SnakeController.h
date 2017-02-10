#ifndef EAE6320_SNAKECONTROLLER_H
#define EAE6320_SNAKECONTROLLER_H

#include "IGameObjectController.h"
#include <cstdint>
namespace eae6320
{
	namespace Gameplay
	{
		class SnakeController final : IGameObjectController
		{
		public:
			static const uint32_t classUUID;
			static SnakeController* Initialize();
		private:
			SnakeController();
			Math::cVector UpdatePosition()override;
			Math::cVector UpdateOrientation(RotationAxis axis)override;

			float speedOfSnake;
			char sign;
			Direction direction;
		};
	}
}

#endif // !EAE6320_SNAKECONTROLLER_H
