#include "../Effect.h"
#include "../MeshData.h"
#include "../CommonData.h"
#include "../../Asserts/Asserts.h"
#include "../../Logging/Logging.h"
#include "../../Platform/Platform.h"

#include <string>

namespace
{
	eae6320::Graphics::CommonData *commonData = eae6320::Graphics::CommonData::GetCommonData();
}

namespace eae6320
{
	namespace Graphics
	{
		bool Effect::LoadEffect()
		{
			//CommonData *commonData = CommonData::GetCommonData();

			Platform::sDataFromFile compiledFragmentShader;
			Platform::sDataFromFile compiledVertexShader;

			bool wereThereErrors = false;

			// Load the compiled fragment shader and create the shader object
			{
				const char* const path = "data/shaders/fragment.binshader";
				std::string errorMessage;
				if (!Platform::LoadBinaryFile(path, compiledFragmentShader, &errorMessage))
				{
					wereThereErrors = true;
					EAE6320_ASSERTF(false, errorMessage.c_str());
					Logging::OutputError("Failed to load the shader file \"%s\": %s", path, errorMessage.c_str());
					goto OnExit;
				}

				ID3D11ClassLinkage* const noInterfaces = NULL;
				const HRESULT result = commonData->s_direct3dDevice->CreatePixelShader(
					compiledFragmentShader.data, compiledFragmentShader.size, noInterfaces, &s_fragmentShader);
				if (FAILED(result))
				{
					wereThereErrors = true;
					EAE6320_ASSERT(false);
					Logging::OutputError("Direct3D failed to create the shader %s with HRESULT %#010x", path, result);
					goto OnExit;

				}
			}


			// Load the compiled vertex shader and create the shader object	
			{
				const char* const path = "data/shaders/vertex.binshader";
				std::string errorMessage;
				if (!Platform::LoadBinaryFile(path, compiledVertexShader, &errorMessage))
				{
					wereThereErrors = true;
					EAE6320_ASSERTF(false, errorMessage.c_str());
					Logging::OutputError("Failed to load the shader file \"%s\": %s", path, errorMessage.c_str());
					goto OnExit;
				}

				ID3D11ClassLinkage* const noInterfaces = NULL;
				const HRESULT result = commonData->s_direct3dDevice->CreateVertexShader(compiledVertexShader.data, compiledVertexShader.size, noInterfaces, &s_vertexShader);
				if (FAILED(result))
				{
					wereThereErrors = true;
					EAE6320_ASSERT(false);
					Logging::OutputError("Direct3D failed to create the shader %s with HRESULT %#010x", path, result);
					goto OnExit;
				}
			}

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
					compiledVertexShader.data, compiledVertexShader.size, &s_vertexLayout);
				if (FAILED(result))
				{
					wereThereErrors = true;
					EAE6320_ASSERT(false);
					Logging::OutputError("Direct3D failed to create a vertex input layout with HRESULT %#010x", result);
					goto OnExit;
				}
			}
		OnExit:
			compiledFragmentShader.Free();
			compiledVertexShader.Free();
			return !wereThereErrors;
		}
		bool Effect::CleanUpEffect()
		{
			bool wereThereErrors = false;
			if (commonData->s_direct3dDevice)
			{
				if (s_vertexLayout)
				{
					s_vertexLayout->Release();
					s_vertexLayout = NULL;
				}

				if (s_vertexShader)
				{
					s_vertexShader->Release();
					s_vertexShader = NULL;
				}

				if (s_fragmentShader)
				{
					s_fragmentShader->Release();
					s_fragmentShader = NULL;
				}
			}
			return !wereThereErrors;
		}

		void Effect::BindEffect()
		{
			ID3D11ClassInstance** const noInterfaces = NULL;
			const unsigned int interfaceCount = 0;

			commonData->s_direct3dImmediateContext->VSSetShader(s_vertexShader, noInterfaces, interfaceCount);
			commonData->s_direct3dImmediateContext->PSSetShader(s_fragmentShader, noInterfaces, interfaceCount);

			commonData->s_direct3dImmediateContext->IASetInputLayout(s_vertexLayout);
			commonData->s_direct3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
	}
}
