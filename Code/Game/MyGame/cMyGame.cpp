// Header Files
//=============

#include <map>
#include <vector>
#include <regex>
#include "../Gameplay/GameObject.h"
#include "../Gameplay/GameObject2D.h"
#include "../Debug//DebugObject.h"
#include "../Debug/Configuration.h"
#include "cMyGame.h"
#include "../../Engine/Asserts/Asserts.h"
#include "../../Engine/Logging/Logging.h"
#include "../../Engine/Graphics/Graphics.h"
#include "../../Engine/Camera/cCamera.h"
#include "../../Engine/Graphics/Font.h"

// Interface
//==========

// Initialization / Clean Up
//--------------------------

eae6320::cMyGame::~cMyGame()
{

}
// Helper Function Declarations
//=============================

namespace
{
	void GenerateRelativePaths(std::string prefix);
	std::map<const std::string, eae6320::Gameplay::GameObject*> gameObjects;
	std::vector<eae6320::Debug::Shapes::DebugObject*> debugObjects;
	std::map<const std::string, eae6320::Gameplay::GameObject2D*> gameObjects2D;
	std::vector<std::string> relativePaths;
	std::vector<std::string> fileNames;
	const std::regex pattern_match("(\\.)([[:alpha:]]+)");
	const std::string pattern_replace("");
}
// Inherited Implementation
//=========================

// Initialization / Clean Up
//--------------------------

bool eae6320::cMyGame::Initialize()
{
	bool wereThereErrors = false;
	{
		const std::string prefix = "data/gameobjects/";
		GenerateRelativePaths(prefix);
		const size_t numberOfGameObjects = relativePaths.size();
		for (size_t i = 0; i < numberOfGameObjects; i++)
		{
			gameObjects[fileNames[i]] = (Gameplay::GameObject::LoadGameObject(relativePaths[i].c_str()));
		}
		if (!numberOfGameObjects)
		{
			wereThereErrors = true;
			EAE6320_ASSERT(false);
			Logging::OutputError("No Gameobjects to draw build the assets.");
		}
	}
	{
		const std::string prefix = "data/gameobjects2d/";
		GenerateRelativePaths(prefix);
		const size_t numberOfGameObjects2D = relativePaths.size();
		for (size_t i = 0; i < numberOfGameObjects2D; i++)
		{
			gameObjects2D[fileNames[i]] = (Gameplay::GameObject2D::LoadGameObject2D(relativePaths[i].c_str()));
		}
	}
	{
#if defined(EAE6320_DEBUG_SHAPES_AREENABLED)
		//Debug Shape Lines
		debugObjects.push_back(new Debug::Shapes::DebugObject(Math::cVector(-10.0f, 0.0f, 0.0f), 1.0f, 0.0f, 0.0f));
		debugObjects.back()->CreateLine(Math::cVector(10.0f, 10.0f, 10.0f));
		debugObjects.push_back(new Debug::Shapes::DebugObject(Math::cVector(0.0f, 0.0f, 0.0f), 0.0f, 1.0f, 0.0f));
		debugObjects.back()->CreateLine(Math::cVector(10.0f, 20.0f, 40.0f));
		//Debug Shape Boxes
		debugObjects.push_back(new Debug::Shapes::DebugObject(Math::cVector(-60.0f, 10.0f, -50.0f), 0.0f, 0.0f, 1.0f));
		debugObjects.back()->CreateBox(10.0f, 10.0f, 10.0f);
		debugObjects.push_back(new Debug::Shapes::DebugObject(Math::cVector(60.0f, 10.0f, -50.0f), 1.0f, 1.0f, 0.0f));
		debugObjects.back()->CreateBox(10.0f, 20.0f, 40.0f);
		//Debug Shape Spheres
		debugObjects.push_back(new Debug::Shapes::DebugObject(Math::cVector(-30.0f, -70.0f, -75.0f), 0.0f, 1.0f, 1.0f));
		debugObjects.back()->CreateSphere(10.0f, 20, 20);
		debugObjects.push_back(new Debug::Shapes::DebugObject(Math::cVector(30.0f, -70.0f, -75.0f), 1.0f, 0.0f, 1.0f));
		debugObjects.back()->CreateSphere(20.0f, 20, 20);
		//Debug Shape Cylinders
		debugObjects.push_back(new Debug::Shapes::DebugObject(Math::cVector(-40.0f, -20.0f, -100.0f), 1.0f, 0.5f, 0.0f));
		debugObjects.back()->CreateCylinder(20.0f, 10.0f, 40.0f, 10, 10);
		debugObjects.push_back(new Debug::Shapes::DebugObject(Math::cVector(40.0f, -20.0f, -100.0f), 0.5f, 1.0f, 0.0f));
		debugObjects.back()->CreateCylinder(10.0f, 20.0f, 40.0f, 10, 10);
#endif
	}
	Graphics::Font::LoadFont("data/fonts/myfont.binfont");
	//Make different cameras and pushback in cameras vector
	Camera::cCamera *mainCamera = Camera::cCamera::Initialize(false, Math::cVector(0.0f, 0.0f, 0.0f), Math::cVector(0.0f, 0.0f, 200.0f));
	Camera::cCamera::PushBackToVector(mainCamera);
	Camera::cCamera *frontLeftCamera = Camera::cCamera::Initialize(false, Math::cVector(0.0f, 25.0f, 0.0f), Math::cVector(-5.0f, 5.0f, 50.0f));
	Camera::cCamera::PushBackToVector(frontLeftCamera);
	Camera::cCamera *frontRightCamera = Camera::cCamera::Initialize(false, Math::cVector(0.0f, -25.0f, 0.0f), Math::cVector(5.0f, 5.0f, 50.0f));
	Camera::cCamera::PushBackToVector(frontRightCamera);
	Camera::cCamera *backLeftCamera = Camera::cCamera::Initialize(false, Math::cVector(0.0f, 165.0f, 0.0f), Math::cVector(-5.0f, 5.0f, -50.0f));
	Camera::cCamera::PushBackToVector(backLeftCamera);
	Camera::cCamera *backRightCamera = Camera::cCamera::Initialize(false, Math::cVector(0.0f, -165.0f, 0.0f), Math::cVector(5.0f, 5.0f, -50.0f));
	Camera::cCamera::PushBackToVector(backRightCamera);

	//After adding all cameras, doing this is must
	Camera::cCamera::UpdateMaxCameras();

	return !wereThereErrors;
}

void eae6320::cMyGame::ChangeCamera()
{
	Camera::cCamera::ChangeCurrentCamera();
}

void eae6320::cMyGame::UpdateCameraPostion()
{
	Camera::cCamera::GetCurrentCamera()->UpdateCurrentCameraPosition();
}

void eae6320::cMyGame::UpdateCameraOrientation()
{
	Camera::cCamera::GetCurrentCamera()->UpdateCurrentCameraOrientation();
}


void eae6320::cMyGame::SubmitCamera()
{
	Camera::cCamera *currentCamera = Camera::cCamera::GetCurrentCamera();
	if (currentCamera)
	{
		Graphics::SetCamera(currentCamera);
	}
	else
	{
		EAE6320_ASSERT(false);
		Logging::OutputError("No Current Camera Exists");
	}
}

void eae6320::cMyGame::UpdateGameObjectPosition()
{
	for (auto const& gameObject : gameObjects)
	{
		if (gameObject.second)
		{
			gameObject.second->UpdateGameObjectPosition();
		}
	}
}

void eae6320::cMyGame::UpdateGameObjectOrientation()
{
	for (auto const& gameObject : gameObjects)
	{
		if (gameObject.second)
		{
			gameObject.second->UpdateGameObjectOrientation();
		}
	}
}

void eae6320::cMyGame::SubmitDrawcallData3D()
{
	for (auto const& gameObject : gameObjects)
	{
		if (gameObject.second)
		{
			Graphics::SetGameObject(gameObject.second);
		}
	}
}

void eae6320::cMyGame::SubmitDebugShapeData3D()
{
	for (size_t i = 0; i < debugObjects.size(); i++)
	{
		Graphics::SetDebugObject(debugObjects[i]);
	}
}

void eae6320::cMyGame::SubmitDrawcallData2D()
{
	for (auto const& gameObject2D : gameObjects2D)
	{
		if (gameObject2D.second)
		{
			Graphics::SetGameObject2D(gameObject2D.second);
		}
	}
}

bool eae6320::cMyGame::CleanUp()
{
	bool wereThereErrors = false;

	for (auto const& gameObject : gameObjects)
	{
		if (gameObject.second)
		{
			delete gameObject.second;
		}
	}
	gameObjects.clear();
	for (size_t i = 0; i < debugObjects.size(); i++)
	{
		delete debugObjects[i];
	}
	debugObjects.clear();
	for (auto const& gameObject2D : gameObjects2D)
	{
		if (gameObject2D.second)
		{
			delete gameObject2D.second;
		}
	}
	gameObjects2D.clear();
	if (!Camera::cCamera::CleanUp())
	{
		wereThereErrors = true;
		EAE6320_ASSERT(false);
		Logging::OutputError("Camera Cleanup Failed");
	}
	if (!Graphics::Font::CleanUp())
	{
		wereThereErrors = true;
		EAE6320_ASSERT(false);
		Logging::OutputError("Font Cleanup Failed");
	}
	return !wereThereErrors;
}

// Helper Function Definitions
//=============================

namespace
{
	void GenerateRelativePaths(std::string prefix)
	{
		relativePaths.clear();
		fileNames.clear();
		WIN32_FIND_DATA search_data;
		memset(&search_data, 0, sizeof(WIN32_FIND_DATA));
		HANDLE handle = FindFirstFile((prefix + "*").c_str(), &search_data);
		if (handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!strcmp(search_data.cFileName, "."))continue;
				else if (!strcmp(search_data.cFileName, ".."))continue;
				else
				{
					relativePaths.push_back((prefix + search_data.cFileName).c_str());
					fileNames.push_back(std::regex_replace(static_cast<std::string>(search_data.cFileName), pattern_match, pattern_replace));
				}
			} while (FindNextFile(handle, &search_data));
		}
		FindClose(handle);
	}
}