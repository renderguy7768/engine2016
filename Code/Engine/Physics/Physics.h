#ifndef EAE6320_PHYSICS_H
#define EAE6320_PHYSICS_H
#include "../Math/cVector.h"
#include "../Camera/LocalAxes.h"


namespace eae6320
{
	namespace Graphics
	{
		struct MeshData;
	}
}

namespace eae6320
{
	namespace Physics
	{
		bool Initialize();
		bool CleanUp();
		void CheckCollision(const Math::cVector i_newPosition, const Camera::LocalAxes i_localAxes, Math::cVector& o_localOffset);

		extern bool isPlayerOnGround;
		extern Graphics::MeshData* collisionData;
	}
}

#endif // !EAE6320_PHYSICS_H