#include "../../Time/Time.h"
#include "../../Logging/Logging.h"
#include "../ConstantBuffer.h"
#include "../CommonData.h"
#include "../../Math/Functions.h"
#include "../ConstantBufferData.h"

namespace eae6320
{
	namespace Graphics
	{
		bool ConstantBuffer::CreateConstantBuffer(void* initialBufferData)
		{
			D3D11_BUFFER_DESC bufferDescription = { 0 };
			{
				// The byte width must be rounded up to a multiple of 16
				bufferDescription.ByteWidth = eae6320::Math::RoundUpToMultiple_powerOf2(static_cast<unsigned int>(sizeOfConstantBuffer), 16u);
				bufferDescription.Usage = D3D11_USAGE_DYNAMIC;	// The CPU must be able to update the buffer
				bufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;	// The CPU must write, but doesn't read
				bufferDescription.MiscFlags = 0;
				bufferDescription.StructureByteStride = 0;	// Not used
			}
			D3D11_SUBRESOURCE_DATA *initialData = NULL;
			if (constantBufferType == ConstantBufferType::FRAME)
			{
				initialData = reinterpret_cast<D3D11_SUBRESOURCE_DATA*>(malloc(sizeof(D3D11_SUBRESOURCE_DATA)));
				{
					// Fill in the constant buffer

					reinterpret_cast<ConstantBufferData::sFrame*>(initialBufferData)->g_elapsedSecondCount_total = eae6320::Time::GetElapsedSecondCount_total();
					initialData->pSysMem = initialBufferData;

					// (The other data members are ignored for non-texture buffers)
				}
			}
			else if (constantBufferType == ConstantBufferType::MATERIAL)
			{
				initialData = reinterpret_cast<D3D11_SUBRESOURCE_DATA*>(malloc(sizeof(D3D11_SUBRESOURCE_DATA)));
				initialData->pSysMem = initialBufferData;
			}
			const HRESULT result = CommonData::GetCommonData()->s_direct3dDevice->CreateBuffer(&bufferDescription, initialData, &this->s_constantBuffer);
			if (SUCCEEDED(result))
			{
				if (initialData)
				{
					free(initialData);
				}
				return true;
			}
			else
			{
				EAE6320_ASSERT(false);
				eae6320::Logging::OutputError("Direct3D failed to create a constant buffer with HRESULT %#010x", result);
				return false;
			}
			return false;
		}

		void ConstantBuffer::BindingConstantBuffer(BindMode bindMode)
		{
			CommonData* commonData = CommonData::GetCommonData();
			const unsigned int registerAssignedInShader = static_cast<int>(constantBufferType);
			const unsigned int bufferCount = 1;
			if (bindMode == BindMode::VS_ONLY || bindMode == BindMode::VS_PS_BOTH)
			{
				commonData->s_direct3dImmediateContext->VSSetConstantBuffers(registerAssignedInShader, bufferCount, &s_constantBuffer);
			}
			if (bindMode == BindMode::PS_ONLY || bindMode == BindMode::VS_PS_BOTH)
			{
				commonData->s_direct3dImmediateContext->PSSetConstantBuffers(registerAssignedInShader, bufferCount, &s_constantBuffer);
			}

		}

		void ConstantBuffer::UpdateConstantBuffer(void* bufferData, size_t sizeOfConstantBuffer)
		{
			CommonData* commonData = CommonData::GetCommonData();
			// Update the struct (i.e. the memory that we own)
			/*if (constantBufferType == ConstantBufferType::FRAME)
			{
				reinterpret_cast<ConstantBufferData::sFrame*>(bufferData)->g_elapsedSecondCount_total = eae6320::Time::GetElapsedSecondCount_total();
			}*/
			// Get a pointer from Direct3D that can be written to
			void* memoryToWriteTo = NULL;
			{
				D3D11_MAPPED_SUBRESOURCE mappedSubResource;
				{
					// Discard previous contents when writing
					const unsigned int noSubResources = 0;
					const D3D11_MAP mapType = D3D11_MAP_WRITE_DISCARD;
					const unsigned int noFlags = 0;
					const HRESULT result = commonData->s_direct3dImmediateContext->Map(s_constantBuffer, noSubResources, mapType, noFlags, &mappedSubResource);
					if (SUCCEEDED(result))
					{
						memoryToWriteTo = mappedSubResource.pData;
					}
					else
					{
						EAE6320_ASSERT(false);
						eae6320::Logging::OutputError("Direct3D failed to map a constant buffer with HRESULT %#010x", result);
					}
				}
			}
			if (memoryToWriteTo)
			{
				// Copy the new data to the memory that Direct3D has provided
				if (sizeOfConstantBuffer <= this->sizeOfConstantBuffer)
				{
					memcpy(memoryToWriteTo, bufferData, sizeOfConstantBuffer);
				}
				else
				{
					EAE6320_ASSERT(false);
					eae6320::Logging::OutputError("Size of the buffer is greater than initial size of the buffer");
				}
				// Let Direct3D know that the memory contains the data
				// (the pointer will be invalid after this call)
				const unsigned int noSubResources = 0;
				commonData->s_direct3dImmediateContext->Unmap(s_constantBuffer, noSubResources);
				memoryToWriteTo = NULL;
			}
		}

		bool eae6320::Graphics::ConstantBuffer::CleanUpConstantBuffer()
		{
			if (s_constantBuffer)
			{
				s_constantBuffer->Release();
				s_constantBuffer = NULL;
				return true;
			}
			return false;
		}
	}
}