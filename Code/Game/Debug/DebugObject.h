#ifndef EAE6320_DEBUGOBJECT_H
#define EAE6320_DEBUGOBJECT_H

#include "ConfigurationShapes.h"


#if defined(EAE6320_DEBUG_SHAPES_AREENABLED)

#include "../../Engine/Math/cVector.h"
#include "Color.h"
#include <cstdint>
namespace eae6320
{
	namespace Graphics
	{
		class Material;
		class Mesh;
		struct MeshData;
	}
}

namespace eae6320
{
	namespace Debug
	{
		namespace Shapes
		{		
			class DebugObject
			{
			public:
				static Graphics::Material* GetMaterial();
				Graphics::Mesh* GetMesh()const;
				Math::cVector GetPosition()const;
				void GetColor(float& o_r, float& o_g, float& o_b)const;

				explicit DebugObject(const Math::cVector i_position = Math::cVector::zero, const Color i_color = { 1.0f,1.0f,1.0f });
				~DebugObject();

				void CreateLine(const Math::cVector i_end);
				void CreateBox(const float i_width, const float i_height, const float i_depth);
				void CreateSphere(const float i_radius, const uint32_t i_sliceCount, const uint32_t i_stackCount);
				void CreateCylinder(const float i_bottomRadius, const float i_topRadius, const float i_height, const uint32_t i_sliceCount, const uint32_t i_stackCount);

				void UpdateSphere(const float i_radius);

			private:
				Math::cVector m_position;
				Color m_color;
				static Graphics::Material* ms_material;
				Graphics::Mesh* m_mesh;				
				Graphics::MeshData* m_meshData;
			};		
		}
	}
}
#endif

#endif // !EAE6320_DEBUGOBJECT_H

