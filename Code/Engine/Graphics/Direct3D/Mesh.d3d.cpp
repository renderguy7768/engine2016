// Header Files
//=============

#include<iostream>

#include "../Mesh.h"
#include "../CommonData.h"
#include "../../Asserts/Asserts.h"
#include "../../Logging/Logging.h"


bool eae6320::Graphics::Mesh::Initialize(eae6320::Graphics::MeshData&meshData)
{
	CommonData *commonData = CommonData::GetCommonData();
	numberOfIndices = meshData.numberOfIndices;
	if (!commonData->s_vertexLayout)
	{
		// Create the vertex layout
		{
			// These elements must match the VertexFormat::Vertex layout struct exactly.
			// They instruct Direct3D how to match the binary data in the vertex buffer
			// to the input elements in a vertex shader
			// (by using so-called "semantic" names so that, for example,
			// "POSITION" here matches with "POSITION" in shader code).
			// Note that OpenGL uses arbitrarily assignable number IDs to do the same thing.
			const unsigned int vertexElementCount = 2;
			D3D11_INPUT_ELEMENT_DESC layoutDescription[vertexElementCount] = { 0 , 0 };
			{
				// Slot 0

				// POSITION
				// 2 floats == 8 bytes
				// Offset = 0
				{
					D3D11_INPUT_ELEMENT_DESC& positionElement = layoutDescription[0];

					positionElement.SemanticName = "POSITION";
					positionElement.SemanticIndex = 0;	// (Semantics without modifying indices at the end can always use zero)
					positionElement.Format = DXGI_FORMAT_R32G32B32_FLOAT;
					positionElement.InputSlot = 0;
					positionElement.AlignedByteOffset = offsetof(MeshData::Vertex, x);
					positionElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
					positionElement.InstanceDataStepRate = 0;	// (Must be zero for per-vertex data)
				}

				{
					D3D11_INPUT_ELEMENT_DESC& colorElement = layoutDescription[1];

					colorElement.SemanticName = "COLOR";
					colorElement.SemanticIndex = 0;	// (Semantics without modifying indices at the end can always use zero)
					colorElement.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					colorElement.InputSlot = 0;
					colorElement.AlignedByteOffset = offsetof(MeshData::Vertex, r);
					colorElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
					colorElement.InstanceDataStepRate = 0;	// (Must be zero for per-vertex data)
				}
			}
			const HRESULT result = commonData->s_direct3dDevice->CreateInputLayout(layoutDescription, vertexElementCount,
				commonData->compiledVertexShader->data, commonData->compiledVertexShader->size, &commonData->s_vertexLayout);
			if (FAILED(result))
			{
				EAE6320_ASSERT(false);
				eae6320::Logging::OutputError("Direct3D failed to create a vertex input layout with HRESULT %#010x", result);
				return false;
			}
			if (commonData->compiledVertexShader)
			{
				commonData->compiledVertexShader->Free();
			}
		}
	}

	//Vertex Buffer Init
	const unsigned int vertexBufferSize = meshData.numberOfVertices * sizeof(MeshData::Vertex);
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
		if (meshData.vertexData)
		{
			initialDataVertexBuffer.pSysMem = meshData.vertexData;
		}
		else
		{
			EAE6320_ASSERT(false);
			eae6320::Logging::OutputError("Direct3D failed to create the vertex buffer because there is no Vertex Data");
			return false;
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
	const unsigned int indexBufferSize = meshData.numberOfIndices * sizeof(uint16_t);
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
		if (meshData.indexData)
		{
			initialDataIndexBuffer.pSysMem = meshData.indexData;
		}
		else
		{
			EAE6320_ASSERT(false);
			eae6320::Logging::OutputError("Direct3D failed to create the index buffer because there is no Index Data");
			return false;
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
		const unsigned int bufferStride = sizeof(MeshData::Vertex);
		// It's possible to start streaming data in the middle of a vertex buffer
		const unsigned int bufferOffset = 0;
		commonData->s_direct3dImmediateContext->IASetVertexBuffers(startingSlot, vertexBufferCount, &s_vertexBuffer, &bufferStride, &bufferOffset);	
	}
	// Specify what kind of data the vertex buffer holds
	{
		// Set the layout (which defines how to interpret a single vertex)
		commonData->s_direct3dImmediateContext->IASetInputLayout(commonData->s_vertexLayout);
		// Set the topology (which defines how to interpret multiple vertices as a single "primitive";
		// we have defined the vertex buffer as a triangle list
		// (meaning that every primitive is a triangle and will be defined by three vertices)
		commonData->s_direct3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	{
		EAE6320_ASSERT(s_indexBuffer != NULL);
		// Every index is a 16 bit unsigned integer
		const DXGI_FORMAT format = DXGI_FORMAT_R16_UINT;
		// The indices start at the beginning of the buffer
		const unsigned int offset = 0;
		commonData->s_direct3dImmediateContext->IASetIndexBuffer(s_indexBuffer, format, offset);
	}
	// Render triangles from the currently-bound vertex buffer
	{
		
		// It's possible to start rendering primitives in the middle of the stream
		const unsigned int indexOfFirstIndexToUse = 0;
		const unsigned int offsetToAddToEachIndex = 0;
		commonData->s_direct3dImmediateContext->DrawIndexed(numberOfIndices, indexOfFirstIndexToUse, offsetToAddToEachIndex);
	}
}
