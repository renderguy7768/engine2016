/*
This file manages mesh-related functionality
*/

#ifndef EAE6320_MESH_H
#define EAE6320_MESH_H

#if defined( EAE6320_PLATFORM_D3D )
#include<D3D11.h>
#elif defined( EAE6320_PLATFORM_GL )
#include "OpenGL\Includes.h"
#include "Configuration.h"
#endif	

// Interface
//==========

namespace eae6320
{
	namespace Graphics
	{	
		class Mesh
		{
		public:
			bool Initialize();
			bool CleanUp();
			void RenderMesh();
		private:
#if defined( EAE6320_PLATFORM_D3D )
			// The vertex buffer holds the data for each vertex
			ID3D11Buffer* s_vertexBuffer = NULL;
#elif defined( EAE6320_PLATFORM_GL )
			GLuint s_vertexArrayId = 0;
#ifdef EAE6320_GRAPHICS_ISDEVICEDEBUGINFOENABLED
			// OpenGL debuggers don't seem to support freeing the vertex buffer
			// and letting the vertex array object hold a reference to it,
			// and so if debug info is enabled an explicit reference is held
			GLuint s_vertexBufferId = 0;
#endif
#endif			
		};
	}
}
#endif	// EAE6320_MESH_H



