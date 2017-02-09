#ifndef EAE6320_CUBECONTROLLER_H
#define EAE6320_CUBECONTROLLER_H

#include "IGameObjectController.h"
#include <cstdint>
namespace eae6320
{
	namespace Gameplay
	{
		class CubeController final : private IGameObjectController
		{	
		public:
			static const uint32_t classUUID;
			static CubeController* Initialize();
		private:
			CubeController();
			Math::cVector UpdatePosition()override;
			Math::cVector UpdateOrientation(RotationAxis axis)override;

			float rotationSpeedOfCube;
			RotationAxis rotationAxis;
			char sign;
			Direction direction;
		};
	}
}



#endif // !EAE6320_CUBECONTROLLER_H