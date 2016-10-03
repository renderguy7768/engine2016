// Header Files
//=============


#include <vector>
#include "../Gameplay/GameObject.h"
#include "cMyGame.h"
#include "../../Engine/Asserts/Asserts.h"
#include "../../Engine/Logging/Logging.h"
#include "../../Engine/Graphics/Graphics.h"

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
	void GenerateRelativePaths();
	std::vector<eae6320::Gameplay::GameObject*> gameObjects;
	std::vector<std::string> relativePaths;
	size_t numberOfMeshes;
	
}
// Inherited Implementation
//=========================

// Initialization / Clean Up
//--------------------------

bool eae6320::cMyGame::Initialize()
{
	GenerateRelativePaths();
	for (size_t i = 0; i < numberOfMeshes; i++)
	{
		gameObjects.push_back(Gameplay::GameObject::Initilaize(relativePaths[i].c_str()));
	}
	gameObjects[0]->SetIsStatic(false);
	camera = Camera::cCamera::Initialize();
	if (camera)
	{
		Graphics::SetCamera(camera);
	}
	else
	{
		EAE6320_ASSERT(false);
		Logging::OutputError("Camera Initilization Failed");
	}
	bool wereThereErrors = false;
	//if (!mesh1)
	//{
	//	wereThereErrors = true;
	//	EAE6320_ASSERT(false);
	//	Logging::OutputError("Mesh was not created %s", path_mesh1);
	//}
	//if (!mesh2)
	//{
	//	wereThereErrors = true;
	//	EAE6320_ASSERT(false);
	//	Logging::OutputError("Mesh was not created %s", path_mesh2);
	//}
	return !wereThereErrors;
}

void eae6320::cMyGame::UpdateGameObjectPosition()
{
	for (size_t i = 0; i < numberOfMeshes; i++)
	{
		gameObjects[i]->UpdateGameObjectPosition();
	}
}

void eae6320::cMyGame::SubmitMesh()
{
	Graphics::SetGameObject(gameObjects[0], 0.0f, 0.0f);
	//Graphics::SetGameObject(gameObjects[1], 0.0f, 0.0f);
}

bool eae6320::cMyGame::CleanUp()
{
	for (size_t i = 0; i < numberOfMeshes; i++)
	{
		delete gameObjects[i];
	}
	gameObjects._Pop_back_n(numberOfMeshes);
	if (camera)
	{
		delete camera;
	}
	bool wereThereErrors = false;
	/*if (mesh1 && !mesh1->CleanUp())
	{
		wereThereErrors = true;
		EAE6320_ASSERT(false);
		Logging::OutputError("Mesh1 cleanup failed");
	}
	if (mesh2 && !mesh2->CleanUp())
	{
		wereThereErrors = true;
		EAE6320_ASSERT(false);
		Logging::OutputError("Mesh2 cleanup failed");
	}
	if (mesh1)
	{
		delete mesh1;
	}
	if (mesh2)
	{
		delete mesh2;
	}*/
	return !wereThereErrors;
}

// Helper Function Definitions
//=============================

namespace
{
	void GenerateRelativePaths()
	{
		std::string prefix = "data/Meshes/";
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
					relativePaths.push_back((prefix + search_data.cFileName).c_str());
			} while (FindNextFile(handle, &search_data));
		}
		FindClose(handle);
		numberOfMeshes = relativePaths.size();
	}
}