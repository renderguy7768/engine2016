// Header Files
//=============

#include<iostream>

#include "../Mesh.h"
#include "../CommonData.h"
#include "../../Asserts/Asserts.h"
#include "../../Logging/Logging.h"


bool eae6320::Graphics::Mesh::Initialize(void* meshData)
{
	CommonData *commonData = CommonData::GetCommonData();
	MeshData<uint16_t> *meshData_16 = NULL;
	MeshData<uint32_t> *meshData_32 = NULL;
	if (is16bit)
	{
		meshData_16 = reinterpret_cast<MeshData<uint16_t>*>(meshData);
		numberOfIndices_16 = meshData_16->numberOfIndices;
	}
	else
	{
		meshData_32 = reinterpret_cast<MeshData<uint32_t>*>(meshData);
		numberOfIndices_32 = meshData_32->numberOfIndices;
	}

	//Vertex Buffer Init
	unsigned int vertexBufferSize = 0;
	if (is16bit)
	{
		vertexBufferSize = meshData_16->numberOfVertices * sizeof(MeshData<uint16_t>::Vertex);
	}
	else
	{
		vertexBufferSize = meshData_32->numberOfVertices * sizeof(MeshData<uint32_t>::Vertex);
	}
	
	D3D11_BUFFER_DESC bufferDescriptionVertexBuffer = { 0 };
	{
		bufferDescriptionVertexBuffer.ByteWidth = vertexBufferSize;
		bufferDescriptionVertexBuffer.Usage = D3D11_USAGE_IMMUTABLE;	// In our class the buffer will never change after it's been created
		bufferDescriptionVertexBuffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDescriptionVertexBuffer.CPUAccessFlags = 0;	// No CPU access is necessary
		bufferDescriptionVertexBuffer.MiscFlags = 0;
		bufferDescriptionVertexBuffer.StructureByteStride = 0;	// Not used
	}
	D3D11_SUBRESOURCE_DATA initialDataVertexBuffer = { 0 };
	{
		if (is16bit)
		{
			if (meshData_16->vertexData)
			{
				initialDataVertexBuffer.pSysMem = meshData_16->vertexData;
			}
			else
			{
				EAE6320_ASSERT(false);
				eae6320::Logging::OutputError("Direct3D failed to create the vertex buffer because there is no Vertex Data");
				return false;
			}
		}	
		else
		{
			if (meshData_32->vertexData)
			{
				initialDataVertexBuffer.pSysMem = meshData_32->vertexData;
			}
			else
			{
				EAE6320_ASSERT(false);
				eae6320::Logging::OutputError("Direct3D failed to create the vertex buffer because there is no Vertex Data");
				return false;
			}
		}
		// (The other data members are ignored for non-texture buffers)
	}

	const HRESULT resultVertexBuffer = commonData->s_direct3dDevice->CreateBuffer(&bufferDescriptionVertexBuffer, &initialDataVertexBuffer, &s_vertexBuffer);
	if (FAILED(resultVertexBuffer))
	{
		EAE6320_ASSERT(false);
		eae6320::Logging::OutputError("Direct3D failed to create the vertex buffer with HRESULT %#010x", resultVertexBuffer);
		return false;
	}

	//Index Buffer Init
	unsigned int indexBufferSize = 0;
	if (is16bit)
	{
		indexBufferSize = meshData_16->numberOfIndices * sizeof(uint16_t);
	}
	else
	{
		indexBufferSize = meshData_32->numberOfIndices * sizeof(uint32_t);
	}
	D3D11_BUFFER_DESC bufferDescriptionIndexBuffer = { 0 };
	{
		bufferDescriptionIndexBuffer.ByteWidth = indexBufferSize;
		bufferDescriptionIndexBuffer.Usage = D3D11_USAGE_IMMUTABLE;	// In our class the buffer will never change after it's been created
		bufferDescriptionIndexBuffer.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDescriptionIndexBuffer.CPUAccessFlags = 0;	// No CPU access is necessary
		bufferDescriptionIndexBuffer.MiscFlags = 0;
		bufferDescriptionIndexBuffer.StructureByteStride = 0;	// Not used
	}
	D3D11_SUBRESOURCE_DATA initialDataIndexBuffer = { 0 };
	{
		if (is16bit)
		{
			if (meshData_16->indexData)
			{
				initialDataIndexBuffer.pSysMem = meshData_16->indexData;
			}
			else
			{
				EAE6320_ASSERT(false);
				eae6320::Logging::OutputError("Direct3D failed to create the index buffer because there is no Index Data");
				return false;
			}
		}
		else
		{
			if (meshData_32->indexData)
			{
				initialDataIndexBuffer.pSysMem = meshData_32->indexData;
			}
			else
			{
				EAE6320_ASSERT(false);
				eae6320::Logging::OutputError("Direct3D failed to create the index buffer because there is no Index Data");
				return false;
			}
		}	
		// (The other data members are ignored for non-texture buffers)
	}

	const HRESULT resultIndexBuffer = commonData->s_direct3dDevice->CreateBuffer(&bufferDescriptionIndexBuffer, &initialDataIndexBuffer, &s_indexBuffer);
	if (FAILED(resultIndexBuffer))
	{
		EAE6320_ASSERT(false);
		eae6320::Logging::OutputError("Direct3D failed to create the index buffer with HRESULT %#010x", resultIndexBuffer);
		return false;
	}

	return true;

}

bool eae6320::Graphics::Mesh::CleanUp()
{
	bool wereThereErrors = false;
	if (s_vertexBuffer)
	{
		s_vertexBuffer->Release();
		s_vertexBuffer = NULL;
	}
	if (s_indexBuffer)
	{
		s_indexBuffer->Release();
		s_indexBuffer = NULL;
	}
	return !wereThereErrors;
}

void eae6320::Graphics::Mesh::RenderMesh()
{
	CommonData *commonData = CommonData::GetCommonData();
	// Bind a specific vertex buffer to the device as a data source
	{
		const unsigned int startingSlot = 0;
		const unsigned int vertexBufferCount = 1;
		// The "stride" defines how large a single vertex is in the stream of data
		unsigned int bufferStride = 0;
		if (is16bit)
		{
			bufferStride = sizeof(MeshData<uint16_t>::Vertex);
		}
		else
		{
			bufferStride = sizeof(MeshData<uint32_t>::Vertex);
		}
		
		// It's possible to start streaming data in the middle of a vertex buffer
		const unsigned int bufferOffset = 0;
		commonData->s_direct3dImmediateContext->IASetVertexBuffers(startingSlot, vertexBufferCount, &s_vertexBuffer, &bufferStride, &bufferOffset);	
	}
	// Specify what kind of data the vertex buffer holds
	{
		// Set the layout (which defines how to interpret a single vertex)
		//commonData->s_direct3dImmediateContext->IASetInputLayout(commonData->s_vertexLayout);
		// Set the topology (which defines how to interpret multiple vertices as a single "primitive";
		// we have defined the vertex buffer as a triangle list
		// (meaning that every primitive is a triangle and will be defined by three vertices)
		//commonData->s_direct3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	{
		EAE6320_ASSERT(s_indexBuffer != NULL);
		// Every index is a 16 bit unsigned integer
		DXGI_FORMAT format;
		if (is16bit)
		{
			format = DXGI_FORMAT_R16_UINT;
		}
		else
		{
			format = DXGI_FORMAT_R32_UINT;
		}
		
		// The indices start at the beginning of the buffer
		const unsigned int offset = 0;
		commonData->s_direct3dImmediateContext->IASetIndexBuffer(s_indexBuffer, format, offset);
	}
	// Render triangles from the currently-bound vertex buffer
	{
		
		// It's possible to start rendering primitives in the middle of the stream
		const unsigned int indexOfFirstIndexToUse = 0;
		const unsigned int offsetToAddToEachIndex = 0;
		if (is16bit)
		{
			commonData->s_direct3dImmediateContext->DrawIndexed(numberOfIndices_16, indexOfFirstIndexToUse, offsetToAddToEachIndex);
		}
		else
		{
			commonData->s_direct3dImmediateContext->DrawIndexed(numberOfIndices_32, indexOfFirstIndexToUse, offsetToAddToEachIndex);
		}
	}
}
