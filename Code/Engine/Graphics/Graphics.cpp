#include "Graphics.h"

#include "ConstantBuffer.h"
#include "ConstantBufferData.h"
#include "Material.h"
#include "Mesh.h"
#include "Effect.h"
#include "../Asserts/Asserts.h"
#include "../Logging/Logging.h"
#include "../Platform/Platform.h"
#include "../Time/Time.h"
#include "../../Game/Gameplay/GameObject.h"
#include "../Camera/cCamera.h"
#include "../Graphics/CommonData.h"

#if defined( EAE6320_PLATFORM_GL )
#include "../Windows/Functions.h"
#include <sstream>
#endif

#include <list>
#include <vector>
namespace
{
	using namespace eae6320::Graphics;
	CommonData *commonData = CommonData::GetCommonData();
	//std::map<uint32_t, std::vector < eae6320::Gameplay::GameObject* >>gameObjects;
	std::vector<eae6320::Gameplay::GameObject*>unsortedGameObjects;
	std::list<eae6320::Gameplay::GameObject*>sortedGameObjects;
	uint32_t currentMaterialUUID = 0;
	ConstantBufferData::sFrame frameBufferData;
	ConstantBuffer frameBuffer;
	ConstantBuffer drawCallBuffer;
	eae6320::Camera::cCamera* camera;
	HWND s_renderingWindow = NULL;

	void SortGameObjects();
	void ClearScreen();
	void SwapBuffers();
	bool InitializeInternal(const sInitializationParameters& i_initializationParameters);
	bool CleanUpInternal();

#if defined( EAE6320_PLATFORM_D3D )
	IDXGISwapChain* s_swapChain = NULL;
	ID3D11RenderTargetView* s_renderTargetView = NULL;
	ID3D11DepthStencilView* s_depthStencilView = NULL;
	bool CreateDevice(const unsigned int i_resolutionWidth, const unsigned int i_resolutionHeight);
	bool CreateViews(const unsigned int i_resolutionWidth, const unsigned int i_resolutionHeight);
#elif defined( EAE6320_PLATFORM_GL )
	HDC s_deviceContext = NULL;
	HGLRC s_openGlRenderingContext = NULL;
	bool CreateRenderingContext();
	bool EnableBackFaceCulling();
	bool EnableDepthTesting();
	bool EnableDepthWriting();
#endif

}

void eae6320::Graphics::SetCamera(Camera::cCamera * Camera)
{
	camera = Camera;
}

void eae6320::Graphics::SetGameObject(Gameplay::GameObject*gameObject)
{
	if (gameObject)
	{
		unsortedGameObjects.push_back(gameObject);
	}
	else
	{
		EAE6320_ASSERT(false);
		Logging::OutputError("Trying to draw a non existent gameobject. Check gameobject name");
	}
}

void eae6320::Graphics::RenderFrame()
{
	ClearScreen();
	// Update the constant buffer
	{
		frameBufferData.g_transform_worldToCamera = Math::cMatrix_transformation::CreateWorldToCameraTransform(camera->GetOrientation(), camera->GetPosition());
		frameBufferData.g_transform_cameraToScreen = Math::cMatrix_transformation::CreateCameraToScreenTransform_perspectiveProjection(camera->GetFieldOfView(), camera->GetAspectRatio(), camera->GetNearPlaneDistance(), camera->GetFarPlaneDistance());
		frameBufferData.g_elapsedSecondCount_total = eae6320::Time::GetElapsedSecondCount_total();
		frameBuffer.UpdateConstantBuffer(&frameBufferData, sizeof(frameBufferData));
	}

	// Draw Submitted Gameobjects
	{
		SortGameObjects();
		ConstantBufferData::sDrawCall drawCallBufferData;
		for (auto it = sortedGameObjects.begin(); it != sortedGameObjects.end(); it++)
		{
			Material*material = (*it)->GetMaterial();
			if (currentMaterialUUID != material->GetMaterialUUID())
			{
				material->BindMaterial();
				currentMaterialUUID = material->GetMaterialUUID();
			}
			drawCallBufferData.g_transform_localToWorld = Math::cMatrix_transformation((*it)->GetOrientation(), (*it)->GetPosition());
			drawCallBuffer.UpdateConstantBuffer(&drawCallBufferData, sizeof(drawCallBufferData));
			(*it)->GetMesh()->RenderMesh();
		}
		sortedGameObjects.clear();
	}
	SwapBuffers();
}

bool eae6320::Graphics::Initialize(const sInitializationParameters& i_initializationParameters)
{
	s_renderingWindow = i_initializationParameters.mainWindow;
	if (!InitializeInternal(i_initializationParameters))
	{
		EAE6320_ASSERT(false);
		return false;
	}
	if (!frameBuffer.InitializeConstantBuffer(ConstantBufferType::FRAME, sizeof(frameBufferData), &frameBufferData))
	{
		EAE6320_ASSERT(false);
		return false;
	}
	else
	{
		frameBuffer.BindingConstantBuffer();
	}

	if (!drawCallBuffer.InitializeConstantBuffer(ConstantBufferType::DRAWCALL, sizeof(ConstantBufferData::sDrawCall)))
	{
		EAE6320_ASSERT(false);
		return false;
	}
	else
	{
		drawCallBuffer.BindingConstantBuffer(BindMode::VS_ONLY);
	}
	return true;
}

bool eae6320::Graphics::CleanUp()
{
	frameBuffer.CleanUpConstantBuffer();
	drawCallBuffer.CleanUpConstantBuffer();

	bool wereThereErrors = false;

	if (!CleanUpInternal())
	{
		wereThereErrors = true;
	}

	s_renderingWindow = NULL;
	return !wereThereErrors;
}

namespace
{
	void SortGameObjects()
	{
		const size_t lengthcheck = unsortedGameObjects.size();
		sortedGameObjects.push_front(unsortedGameObjects[lengthcheck - 1]);
		unsortedGameObjects.pop_back();
		while (lengthcheck != sortedGameObjects.size())
		{
			std::list<eae6320::Gameplay::GameObject*>::iterator itList = sortedGameObjects.begin();
			uint32_t currentMaterialUUID = (*itList)->GetMaterial()->GetMaterialUUID();
			uint32_t currentEffectUUID = (*itList)->GetMaterial()->GetEffect()->GetEffectUUID();	
			std::vector<eae6320::Gameplay::GameObject*>::iterator itVector = unsortedGameObjects.begin();
			uint32_t materialUUIDToBeChecked = (*itVector)->GetMaterial()->GetMaterialUUID();
			uint32_t effectUUIDToBeChecked = (*itVector)->GetMaterial()->GetEffect()->GetEffectUUID();
			while (itList != sortedGameObjects.end() && currentMaterialUUID != materialUUIDToBeChecked && currentEffectUUID != effectUUIDToBeChecked)
			{
				itList++;
				if (itList != sortedGameObjects.end())
				{
					currentMaterialUUID = (*itList)->GetMaterial()->GetMaterialUUID();
					currentEffectUUID = (*itList)->GetMaterial()->GetEffect()->GetEffectUUID();
				}
			}
			if (itList == sortedGameObjects.end())
			{
				sortedGameObjects.push_back((*itVector));
				unsortedGameObjects.erase(itVector);
			}
			else
			{
				if (currentMaterialUUID == materialUUIDToBeChecked && currentEffectUUID == effectUUIDToBeChecked)
				{
					sortedGameObjects.insert(itList, (*itVector));
					unsortedGameObjects.erase(itVector);
				}
				else
				{
					bool itemNotAdded = true;
					while (itList != sortedGameObjects.end())
					{
						itList++;
						if (itList != sortedGameObjects.end())
						{
							currentEffectUUID = (*itList)->GetMaterial()->GetEffect()->GetEffectUUID();
							if (currentEffectUUID == effectUUIDToBeChecked)
							{
								continue;
							}
							else
							{
								itemNotAdded = false;
								sortedGameObjects.insert(itList, (*itVector));
								unsortedGameObjects.erase(itVector);
								break;
							}
						}
					}
					if (itemNotAdded)
					{
						sortedGameObjects.insert(itList, (*itVector));
						unsortedGameObjects.erase(itVector);
					}
				}
			}
		}
		unsortedGameObjects.clear();
	}
	void ClearScreen()
	{
#if defined( EAE6320_PLATFORM_D3D )
		float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		commonData->s_direct3dImmediateContext->ClearRenderTargetView(s_renderTargetView, clearColor);
		const float depthToClear = 1.0f;
		const uint8_t stencilToClear = 0;
		commonData->s_direct3dImmediateContext->ClearDepthStencilView(s_depthStencilView, D3D11_CLEAR_DEPTH,
			depthToClear, stencilToClear);
#elif defined( EAE6320_PLATFORM_GL )
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		EAE6320_ASSERT(glGetError() == GL_NO_ERROR);
		glClearDepth(1.0f);
		EAE6320_ASSERT(glGetError() == GL_NO_ERROR);
		const GLbitfield clearColorAndDepth = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		glClear(clearColorAndDepth);
		EAE6320_ASSERT(glGetError() == GL_NO_ERROR);
#endif
	}

	void SwapBuffers()
	{
#if defined( EAE6320_PLATFORM_D3D )
		const unsigned int swapImmediately = 0;
		const unsigned int presentNextFrame = 0;
		const HRESULT result = s_swapChain->Present(swapImmediately, presentNextFrame);
		EAE6320_ASSERT(SUCCEEDED(result));
#elif defined( EAE6320_PLATFORM_GL )
		BOOL result = SwapBuffers(s_deviceContext);
		EAE6320_ASSERT(result != FALSE);
#endif
	}

	bool InitializeInternal(const sInitializationParameters& i_initializationParameters)
	{
#if defined( EAE6320_PLATFORM_D3D )
		// Create an interface to a Direct3D device
		if (!CreateDevice(i_initializationParameters.resolutionWidth, i_initializationParameters.resolutionHeight))
		{
			EAE6320_ASSERT(false);
			return false;
		}
		// Initialize the viewport of the device
		if (!CreateViews(i_initializationParameters.resolutionWidth, i_initializationParameters.resolutionHeight))
		{
			EAE6320_ASSERT(false);
			return false;
		}
#elif defined( EAE6320_PLATFORM_GL )
		std::string errorMessage;
		// Load any required OpenGL extensions
		if (!eae6320::OpenGlExtensions::Load(&errorMessage))
		{
			EAE6320_ASSERTF(false, errorMessage.c_str());
			eae6320::Logging::OutputError(errorMessage.c_str());
			return false;
		}
		// Create an OpenGL rendering context
		if (!CreateRenderingContext())
		{
			EAE6320_ASSERT(false);
			return false;
		}

		if (!EnableBackFaceCulling())
		{
			EAE6320_ASSERT(false);
			return false;
		}

		if (!EnableDepthTesting())
		{
			EAE6320_ASSERT(false);
			return false;
		}

		if (!EnableDepthWriting())
		{
			EAE6320_ASSERT(false);
			return false;
		}
#endif
		return true;
	}
	bool CleanUpInternal()
	{
		bool wereThereErrors = false;
#if defined( EAE6320_PLATFORM_D3D )
		if (commonData->s_direct3dDevice)
		{
			if (s_renderTargetView)
			{
				s_renderTargetView->Release();
				s_renderTargetView = NULL;
			}
			if (s_depthStencilView)
			{
				s_depthStencilView->Release();
				s_depthStencilView = NULL;
			}

			commonData->s_direct3dDevice->Release();
			commonData->s_direct3dDevice = NULL;
		}
		if (commonData->s_direct3dImmediateContext)
		{
			commonData->s_direct3dImmediateContext->Release();
			commonData->s_direct3dImmediateContext = NULL;
		}
		if (s_swapChain)
		{
			s_swapChain->Release();
			s_swapChain = NULL;
		}
#elif defined( EAE6320_PLATFORM_GL )
		if (s_openGlRenderingContext != NULL)
		{
			if (wglMakeCurrent(s_deviceContext, NULL) != FALSE)
			{
				if (wglDeleteContext(s_openGlRenderingContext) == FALSE)
				{
					wereThereErrors = true;
					const std::string windowsErrorMessage = eae6320::Windows::GetLastSystemError();
					EAE6320_ASSERTF(false, windowsErrorMessage.c_str());
					eae6320::Logging::OutputError("Windows failed to delete the OpenGL rendering context: %s", windowsErrorMessage.c_str());
				}
			}
			else
			{
				wereThereErrors = true;
				const std::string windowsErrorMessage = eae6320::Windows::GetLastSystemError();
				EAE6320_ASSERTF(false, windowsErrorMessage.c_str());
				eae6320::Logging::OutputError("Windows failed to unset the current OpenGL rendering context: %s", windowsErrorMessage.c_str());
			}
			s_openGlRenderingContext = NULL;
		}

		if (s_deviceContext != NULL)
		{
			// The documentation says that this call isn't necessary when CS_OWNDC is used
			ReleaseDC(s_renderingWindow, s_deviceContext);
			s_deviceContext = NULL;
	}
#endif
		return !wereThereErrors;
}

#if defined( EAE6320_PLATFORM_D3D )
	bool CreateDevice(const unsigned int i_resolutionWidth, const unsigned int i_resolutionHeight)
	{
		IDXGIAdapter* const useDefaultAdapter = NULL;
		const D3D_DRIVER_TYPE useHardwareRendering = D3D_DRIVER_TYPE_HARDWARE;
		const HMODULE dontUseSoftwareRendering = NULL;
		unsigned int flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
		{
#ifdef EAE6320_GRAPHICS_ISDEVICEDEBUGINFOENABLED
			flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		}
		D3D_FEATURE_LEVEL* const useDefaultFeatureLevels = NULL;
		const unsigned int requestedFeatureLevelCount = 0;
		const unsigned int sdkVersion = D3D11_SDK_VERSION;
		DXGI_SWAP_CHAIN_DESC swapChainDescription = { 0 };
		{
			{
				DXGI_MODE_DESC& bufferDescription = swapChainDescription.BufferDesc;

				bufferDescription.Width = i_resolutionWidth;
				bufferDescription.Height = i_resolutionHeight;
				{
					DXGI_RATIONAL& refreshRate = bufferDescription.RefreshRate;

					refreshRate.Numerator = 0;	// Refresh as fast as possible
					refreshRate.Denominator = 1;
				}
				bufferDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				bufferDescription.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				bufferDescription.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			}
			{
				DXGI_SAMPLE_DESC& multiSamplingDescription = swapChainDescription.SampleDesc;

				multiSamplingDescription.Count = 1;
				multiSamplingDescription.Quality = 0;	// Anti-aliasing is disabled
			}
			swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDescription.BufferCount = 1;
			swapChainDescription.OutputWindow = s_renderingWindow;
			swapChainDescription.Windowed = TRUE;
			swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			swapChainDescription.Flags = 0;
		}
		D3D_FEATURE_LEVEL highestSupportedFeatureLevel;
		const HRESULT result = D3D11CreateDeviceAndSwapChain(useDefaultAdapter, useHardwareRendering, dontUseSoftwareRendering,
			flags, useDefaultFeatureLevels, requestedFeatureLevelCount, sdkVersion, &swapChainDescription,
			&s_swapChain, &commonData->s_direct3dDevice, &highestSupportedFeatureLevel, &commonData->s_direct3dImmediateContext);
		if (SUCCEEDED(result))
		{
			return true;
		}
		else
		{
			EAE6320_ASSERT(false);
			eae6320::Logging::OutputError("Direct3D failed to create a Direct3D11 device with HRESULT %#010x", result);
			return false;
		}
	}

	bool CreateViews(const unsigned int i_resolutionWidth, const unsigned int i_resolutionHeight)
	{
		bool wereThereErrors = false;

		ID3D11Texture2D* backBuffer = NULL;
		ID3D11Texture2D* depthBuffer = NULL;

		// Create a "render target view" of the back buffer
		// (the back buffer was already created by the call to D3D11CreateDeviceAndSwapChain(),
		// but we need a "view" of it to use as a "render target",
		// meaning a texture that the GPU can render to)
		{
			// Get the back buffer from the swap chain
			{
				const unsigned int bufferIndex = 0; // This must be 0 since the swap chain is discarded
				const HRESULT result = s_swapChain->GetBuffer(bufferIndex, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
				if (FAILED(result))
				{
					EAE6320_ASSERT(false);
					eae6320::Logging::OutputError("Direct3D failed to get the back buffer from the swap chain with HRESULT %#010x", result);
					goto OnExit;
				}
			}
			// Create the view
			{
				const D3D11_RENDER_TARGET_VIEW_DESC* const accessAllSubResources = NULL;
				const HRESULT result = commonData->s_direct3dDevice->CreateRenderTargetView(backBuffer, accessAllSubResources, &s_renderTargetView);
				if (FAILED(result))
				{
					EAE6320_ASSERT(false);
					eae6320::Logging::OutputError("Direct3D failed to create the render target view with HRESULT %#010x", result);
					goto OnExit;
				}
			}
		}
		// Create a depth/stencil buffer and a view of it
		{
			// Unlike the back buffer no depth/stencil buffer exists until and unless we create it
			{
				D3D11_TEXTURE2D_DESC textureDescription = { 0 };
				{
					textureDescription.Width = i_resolutionWidth;
					textureDescription.Height = i_resolutionHeight;
					textureDescription.MipLevels = 1; // A depth buffer has no MIP maps
					textureDescription.ArraySize = 1;
					textureDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24 bits for depth and 8 bits for stencil
					{
						DXGI_SAMPLE_DESC& sampleDescription = textureDescription.SampleDesc;
						sampleDescription.Count = 1; // No multisampling
						sampleDescription.Quality = 0; // Doesn't matter when Count is 1
					}
					textureDescription.Usage = D3D11_USAGE_DEFAULT; // Allows the GPU to write to it
					textureDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;
					textureDescription.CPUAccessFlags = 0; // CPU doesn't need access
					textureDescription.MiscFlags = 0;
				}
				// The GPU renders to the depth/stencil buffer and so there is no initial data
				// (like there would be with a traditional texture loaded from disk)
				const D3D11_SUBRESOURCE_DATA* const noInitialData = NULL;
				const HRESULT result = commonData->s_direct3dDevice->CreateTexture2D(&textureDescription, noInitialData, &depthBuffer);
				if (FAILED(result))
				{
					EAE6320_ASSERT(false);
					eae6320::Logging::OutputError("Direct3D failed to create the depth buffer resource with HRESULT %#010x", result);
					goto OnExit;
				}
			}
			// Create the view
			{
				const D3D11_DEPTH_STENCIL_VIEW_DESC* const noSubResources = NULL;
				const HRESULT result = commonData->s_direct3dDevice->CreateDepthStencilView(depthBuffer, noSubResources, &s_depthStencilView);
				if (FAILED(result))
				{
					EAE6320_ASSERT(false);
					eae6320::Logging::OutputError("Direct3D failed to create the depth stencil view with HRESULT %#010x", result);
					goto OnExit;
				}
			}
		}

		// Bind the views
		{
			const unsigned int renderTargetCount = 1;
			commonData->s_direct3dImmediateContext->OMSetRenderTargets(renderTargetCount, &s_renderTargetView, s_depthStencilView);
		}
		// Specify that the entire render target should be visible
		{
			D3D11_VIEWPORT viewPort = { 0 };
			viewPort.TopLeftX = viewPort.TopLeftY = 0.0f;
			viewPort.Width = static_cast<float>(i_resolutionWidth);
			viewPort.Height = static_cast<float>(i_resolutionHeight);
			viewPort.MinDepth = 0.0f;
			viewPort.MaxDepth = 1.0f;
			const unsigned int viewPortCount = 1;
			commonData->s_direct3dImmediateContext->RSSetViewports(viewPortCount, &viewPort);
		}

	OnExit:

		if (backBuffer)
		{
			backBuffer->Release();
			backBuffer = NULL;
		}
		if (depthBuffer)
		{
			depthBuffer->Release();
			depthBuffer = NULL;
		}

		return !wereThereErrors;
	}
#elif defined( EAE6320_PLATFORM_GL )
	bool CreateRenderingContext()
	{
		// Get the device context
		{
			s_deviceContext = GetDC(s_renderingWindow);
			if (s_deviceContext == NULL)
			{
				EAE6320_ASSERT(false);
				eae6320::Logging::OutputError("Windows failed to get the device context");
				return false;
			}
		}
		// Set the pixel format for the window
		// (This can only be done _once_ for a given window)
		{
			// Get the ID of the desired pixel format
			int pixelFormatId;
			{
				// Create a key/value list of attributes that the pixel format should have
				const int desiredAttributes[] =
				{
					WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
					WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
					WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
					WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
					WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
					WGL_COLOR_BITS_ARB, 24,
					WGL_RED_BITS_ARB, 8,
					WGL_GREEN_BITS_ARB, 8,
					WGL_BLUE_BITS_ARB, 8,
					WGL_DEPTH_BITS_ARB, 24,
					WGL_STENCIL_BITS_ARB, 8,
					// NULL terminator
					NULL
				};
				const float* const noFloatAttributes = NULL;
				const unsigned int onlyReturnBestMatch = 1;
				unsigned int returnedFormatCount;
				if (wglChoosePixelFormatARB(s_deviceContext, desiredAttributes, noFloatAttributes, onlyReturnBestMatch,
					&pixelFormatId, &returnedFormatCount) != FALSE)
				{
					if (returnedFormatCount == 0)
					{
						EAE6320_ASSERT(false);
						eae6320::Logging::OutputError("Windows couldn't find a pixel format that satisfied the desired attributes");
						return false;
					}
				}
				else
				{
					const std::string windowsErrorMessage = eae6320::Windows::GetLastSystemError();
					EAE6320_ASSERTF(false, windowsErrorMessage.c_str());
					eae6320::Logging::OutputError("Windows failed to choose the closest pixel format: %s", windowsErrorMessage.c_str());
					return false;
				}
			}
			// Set it
			{
				PIXELFORMATDESCRIPTOR pixelFormatDescriptor = { 0 };
				{
					// I think that the values of this struct are ignored
					// and unnecessary when using wglChoosePixelFormatARB() instead of ChoosePixelFormat(),
					// but the documentation is very unclear and so filling it in seems the safest bet
					pixelFormatDescriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
					pixelFormatDescriptor.nVersion = 1;
					pixelFormatDescriptor.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
					pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
					pixelFormatDescriptor.cColorBits = 24;
					pixelFormatDescriptor.iLayerType = PFD_MAIN_PLANE;
					pixelFormatDescriptor.cDepthBits = 24;
					pixelFormatDescriptor.cStencilBits = 8;
				}
				if (SetPixelFormat(s_deviceContext, pixelFormatId, &pixelFormatDescriptor) == FALSE)
				{
					const std::string windowsErrorMessage = eae6320::Windows::GetLastSystemError();
					EAE6320_ASSERTF(false, windowsErrorMessage.c_str());
					eae6320::Logging::OutputError("Windows couldn't set the desired pixel format: %s", windowsErrorMessage.c_str());
					return false;
				}
			}
		}
		// Create an OpenGL rendering context and make it current
		{
			// Create the context
			{
				// Create a key/value list of attributes that the context should have
				const int desiredAttributes[] =
				{
					// Request at least version 4.2
					WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
					WGL_CONTEXT_MINOR_VERSION_ARB, 2,
					// Request only "core" functionality and not "compatibility"
					// (i.e. only use modern features of version 4.2)
					WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
	#ifdef EAE6320_GRAPHICS_ISDEVICEDEBUGINFOENABLED
					WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
	#endif
					// NULL terminator
					NULL
				};
				const HGLRC noSharedContexts = NULL;
				s_openGlRenderingContext = wglCreateContextAttribsARB(s_deviceContext, noSharedContexts, desiredAttributes);
				if (s_openGlRenderingContext == NULL)
				{
					DWORD errorCode;
					const std::string windowsErrorMessage = eae6320::Windows::GetLastSystemError(&errorCode);
					std::ostringstream errorMessage;
					errorMessage << "Windows failed to create an OpenGL rendering context: ";
					if ((errorCode == ERROR_INVALID_VERSION_ARB)
						|| (HRESULT_CODE(errorCode) == ERROR_INVALID_VERSION_ARB))
					{
						errorMessage << "The requested version number is invalid";
					}
					else if ((errorCode == ERROR_INVALID_PROFILE_ARB)
						|| (HRESULT_CODE(errorCode) == ERROR_INVALID_PROFILE_ARB))
					{
						errorMessage << "The requested profile is invalid";
					}
					else
					{
						errorMessage << windowsErrorMessage;
					}
					EAE6320_ASSERTF(false, errorMessage.str().c_str());
					eae6320::Logging::OutputError(errorMessage.str().c_str());

					return false;
				}
			}
			// Set it as the rendering context of this thread
			if (wglMakeCurrent(s_deviceContext, s_openGlRenderingContext) == FALSE)
			{
				const std::string windowsErrorMessage = eae6320::Windows::GetLastSystemError();
				EAE6320_ASSERTF(false, windowsErrorMessage.c_str());
				eae6320::Logging::OutputError("Windows failed to set the current OpenGL rendering context: %s",
					windowsErrorMessage.c_str());
				return false;
			}
		}

		return true;
	}

	bool EnableBackFaceCulling()
	{
		glEnable(GL_CULL_FACE);
		const GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
			eae6320::Logging::OutputError("OpenGL failed to enable backface culling: %s",
				reinterpret_cast<const char*>(gluErrorString(errorCode)));
			return false;
		}
		return true;
	}

	bool EnableDepthTesting()
	{
		{
			glDepthFunc(GL_LESS);
			const GLenum errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
				eae6320::Logging::OutputError("OpenGL failed to enable depth function: %s",
					reinterpret_cast<const char*>(gluErrorString(errorCode)));
				return false;
			}
		}
		{
			glEnable(GL_DEPTH_TEST);
			const GLenum errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
				eae6320::Logging::OutputError("OpenGL failed to enable depth testing: %s",
					reinterpret_cast<const char*>(gluErrorString(errorCode)));
				return false;
			}
		}
		return true;
	}

	bool EnableDepthWriting()
	{
		glDepthMask(GL_TRUE);
		const GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
			eae6320::Logging::OutputError("OpenGL failed to enable depth writing: %s",
				reinterpret_cast<const char*>(gluErrorString(errorCode)));
			return false;
		}

		return true;
	}
#endif
}