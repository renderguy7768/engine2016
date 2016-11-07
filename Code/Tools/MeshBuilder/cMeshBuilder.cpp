// Header Files
//=============

#include "cMeshBuilder.h"

#include <sstream>
#include <iostream>
#include <climits>

#include "../../Engine/Asserts/Asserts.h"
#include "../AssetBuildLibrary/UtilityFunctions.h"
#include "../../Engine/Platform/Platform.h"
#include "../../External/Lua/Includes.h"
#include "../../Engine/Graphics/MeshData.h"


// Inherited Implementation
//=========================

// Build
//------

// Helper Function Declarations
//=============================

namespace
{
	using namespace eae6320::Graphics;

	template<typename T> MeshData<T> *meshData = NULL;
	const std::type_info* type;

	void InitializeMeshData(bool is16bit);

	bool LoadInitialTable(lua_State& io_luaState);
	bool LoadVerticesTable(lua_State& io_luaState);
	bool LoadPositionsTable(lua_State& io_luaState);
	bool LoadXYZTable(lua_State& io_luaState, int index);
	bool LoadColorsTable(lua_State& io_luaState);
	bool LoadRGBATable(lua_State& io_luaState, int index);
	bool LoadIndicesTable(lua_State& io_luaState);

	bool CheckIfColorIsInCorrectFormat(float *rgba);
}

bool eae6320::AssetBuild::cMeshBuilder::Build(const std::vector<std::string>& i_arguments)
{
	/*bool wereThereErrors = false;

	// Copy the source to the target
	{
		std::string errorMessage;
		//EAE6320_TODO	// Copy m_path_source to m_path_target
		// There are many reasons that a source should be rebuilt,
		// and so even if the target already exists it should just be written over
		const bool noErrorIfTargetAlreadyExists = false;
		// Since we rely on timestamps to determine when a target was built
		// its file time should be updated when the source gets copied
		const bool updateTheTargetFileTime = true;
		if (!(eae6320::Platform::CopyFile(m_path_source, m_path_target, noErrorIfTargetAlreadyExists, updateTheTargetFileTime, &errorMessage)))
		{
			wereThereErrors = true;
			std::ostringstream errorMessage;
			errorMessage << "Failed to copy \"" << m_path_source << "\" to \"" << m_path_target << "\": " << errorMessage.str();
			OutputErrorMessage(errorMessage.str().c_str(), __FILE__);
		}
	}

	return !wereThereErrors;*/

	bool wereThereErrors = false;
	std::string errorMessage;

	FILE * outputFile = NULL;
	// Create a new Lua state
	lua_State* luaState = NULL;
	{
		luaState = luaL_newstate();
		if (!luaState)
		{
			wereThereErrors = true;
			OutputErrorMessage("Failed to create a new Lua state", __FILE__);
			goto OnExit;
		}
	}

	// Creating/Opening file to write in binary mode
	outputFile = fopen(m_path_target, "wb");
	if (!outputFile)
	{
		perror("The target file cannot be opened for writing out data");
		wereThereErrors = true;
		goto OnExit;
	}

	// Open the standard libraries
	luaL_openlibs(luaState);

	// Load and execute it
	{
		if (Platform::DoesFileExist(m_path_source, &errorMessage))
		{

			// Load the asset file as a "chunk",
			// meaning there will be a callable function at the top of the stack
			const int stackTopBeforeLoad = lua_gettop(luaState);
			{
				const int luaResult = luaL_loadfile(luaState, m_path_source);
				if (luaResult != LUA_OK)
				{
					wereThereErrors = true;
					std::cerr << lua_tostring(luaState, -1) << std::endl;
					// Pop the error message
					lua_pop(luaState, 1);
					goto OnExit;
				}
			}
			// Execute the "chunk", which should load the asset
			// into a table at the top of the stack
			{
				const int argumentCount = 0;
				const int returnValueCount = LUA_MULTRET;	// Return _everything_ that the file returns
				const int noErrorHandler = 0;
				const int luaResult = lua_pcall(luaState, argumentCount, returnValueCount, noErrorHandler);
				if (luaResult == LUA_OK)
				{
					// A well-behaved asset file will only return a single value
					const int returnedValueCount = lua_gettop(luaState) - stackTopBeforeLoad;
					if (returnedValueCount == 1)
					{
						// A correct asset file _must_ return a table
						if (!lua_istable(luaState, -1))
						{
							wereThereErrors = true;
							std::cerr << "Asset files must return a table (instead of a "
								<< luaL_typename(luaState, -1) << ")" << std::endl;
							// Pop the returned non-table value
							lua_pop(luaState, 1);
							goto OnExit;
						}
					}
					else
					{
						wereThereErrors = true;
						std::cerr << "Asset files must return a single table (instead of "
							<< returnedValueCount << " values)" << std::endl;
						// Pop every value that was returned
						lua_pop(luaState, returnedValueCount);
						goto OnExit;
					}
				}
				else
				{
					wereThereErrors = true;
					std::cerr << lua_tostring(luaState, -1) << std::endl;
					// Pop the error message
					lua_pop(luaState, 1);
					goto OnExit;
				}
			}
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage(errorMessage.c_str(), m_path_source);
			goto OnExit;
		}
	}

	//If this code is reached the asset file was loaded successfully,
	//and its table is now at index -1

	//meshData = reinterpret_cast<Graphics::MeshData*>(malloc(sizeof(Graphics::MeshData)));
	if (!LoadInitialTable(*luaState, *meshData))
	{
		wereThereErrors = true;
		OutputErrorMessage("Failed to load initial table", __FILE__);
		goto OnExit;
	}
	lua_pop(luaState, 1);



	// Writing data to file 
	{
		// Writing Number Of Vertices
		fwrite(&meshData->numberOfVertices, sizeof(uint16_t), 1, outputFile);
		if (ferror(outputFile))
		{
			fprintf_s(stderr, "Error writing number of vertices to %s \n", m_path_target);
			wereThereErrors = true;
			goto OnExit;
		}

		// Writing Number Of Indices
		fwrite(&meshData->numberOfIndices, sizeof(uint16_t), 1, outputFile);
		if (ferror(outputFile))
		{
			fprintf_s(stderr, "Error writing number of indices to %s \n", m_path_target);
			wereThereErrors = true;
			goto OnExit;
		}

		// Writing Vertex Array
		fwrite(meshData->vertexData, sizeof(Graphics::MeshData::Vertex), meshData->numberOfVertices, outputFile);
		if (ferror(outputFile))
		{
			fprintf_s(stderr, "Error writing vertex array to %s \n", m_path_target);
			wereThereErrors = true;
			goto OnExit;
		}

		// Writing Index Array
		fwrite(meshData->indexData, sizeof(uint16_t), meshData->numberOfIndices, outputFile);
		if (ferror(outputFile))
		{
			fprintf_s(stderr, "Error writing index array to %s \n", m_path_target);
			wereThereErrors = true;
			goto OnExit;
		}
	}

OnExit:
	// Closing file only if it got opened
	if(outputFile)
	{
		if (fclose(outputFile))
		{
			wereThereErrors = true;
			OutputErrorMessage("Failed to close target file", __FILE__);
		}
	}
	
	if (meshData)
	{
		if (meshData->indexData)
		{
			free(meshData_uint16_t->indexData);
		}
		if (meshData_uint16_t->vertexData)
		{
			free(meshData_uint16_t->vertexData);
		}
		free(meshData_uint16_t);
	}

	if (luaState)
	{
		// If I haven't made any mistakes
		// there shouldn't be anything on the stack
		// regardless of any errors
		EAE6320_ASSERT(lua_gettop(luaState) == 0);

		lua_close(luaState);
		luaState = NULL;
	}

	return !wereThereErrors;
}

// Helper Function Definitions
//=============================

namespace
{
	void InitializeMeshData(bool is16bit)
	{
		if (is16bit)
		{
			meshData<uint16_t> = reinterpret_cast<MeshData<uint16_t>*>(malloc(sizeof(MeshData<uint16_t>)));
			type = &typeid(meshData<uint16_t>);
		}
		else
		{
			meshData<uint32_t> = reinterpret_cast<MeshData<uint32_t>*>(malloc(sizeof(MeshData<uint32_t>)));
			type = &typeid(meshData<uint32_t>);
		}
	}

	bool LoadInitialTable(lua_State& io_luaState)
	{
		if (!LoadIndicesTable(io_luaState))
		{
			return false;
		}

		if (!LoadVerticesTable(io_luaState))
		{
			return false;
		}

		return true;
	}
	bool LoadVerticesTable(lua_State& io_luaState, MeshData&meshData)
	{
		bool wereThereErrors = false;
		const char* const key = "vertices";
		lua_pushstring(&io_luaState, key);
		lua_gettable(&io_luaState, -2);
		if (lua_isnil(&io_luaState, -1))
		{
			wereThereErrors = true;
			fprintf_s(stderr, "No value for key:\"%s\" was found in the table", key);
			goto OnExit;
		}
		if (lua_type(&io_luaState, -1) == LUA_TTABLE)
		{
			if (!LoadPositionsTable(io_luaState, meshData))
			{
				wereThereErrors = true;
				goto OnExit;
			}
			if (!LoadColorsTable(io_luaState, meshData))
			{
				wereThereErrors = true;
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			fprintf_s(stderr, "The value at \"%s\" must be a table (instead of a %s) ", key, luaL_typename(&io_luaState, -1));
			goto OnExit;
		}

	OnExit:

		// Pop the Vertices table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;

	}
	bool LoadPositionsTable(lua_State& io_luaState, MeshData&meshData)
	{
		bool wereThereErrors = false;
		const char* const key = "positions";
		lua_pushstring(&io_luaState, key);
		lua_gettable(&io_luaState, -2);
		if (lua_isnil(&io_luaState, -1))
		{
			wereThereErrors = true;
			fprintf_s(stderr, "No value for key:\"%s\" was found in the table", key);
			goto OnExit;
		}
		if (lua_type(&io_luaState, -1) == LUA_TTABLE)
		{
			const int positions = luaL_len(&io_luaState, -1);
			meshData.numberOfVertices = positions;
			meshData.vertexData = reinterpret_cast<MeshData::Vertex*>(malloc(meshData.numberOfVertices * sizeof(MeshData::Vertex)));
			if (!meshData.vertexData)
			{
				wereThereErrors = true;
				fprintf_s(stderr, "Could not initialize the Vertex Data");
				goto OnExit;
			}
			for (int i = 1; i <= meshData.numberOfVertices; ++i)
			{
				if (!LoadXYZTable(io_luaState, meshData, (i - 1)))
				{
					wereThereErrors = true;
					goto OnExit;
				}
			}
		}
		else
		{
			wereThereErrors = true;
			fprintf_s(stderr, "The value at \"%s\" must be a table (instead of a %s) ", key, luaL_typename(&io_luaState, -1));
			goto OnExit;
		}

	OnExit:

		// Pop the Positions table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}
	bool LoadXYZTable(lua_State& io_luaState, MeshData&meshData, int index)
	{
		bool wereThereErrors = false;
		lua_pushinteger(&io_luaState, index + 1);
		lua_gettable(&io_luaState, -2);
		if (lua_isnil(&io_luaState, -1))
		{
			wereThereErrors = true;
			fprintf_s(stderr, "No value for key:%d was found in the table", (index + 1));
			goto OnExit;
		}
		if (lua_type(&io_luaState, -1) == LUA_TTABLE)
		{
			const int arrayLength = luaL_len(&io_luaState, -1);
			float xyz[3] = { 0.0f,0.0f,0.0f };
			if (arrayLength == 3)
			{
				for (int i = 1; i <= arrayLength; ++i)
				{
					lua_pushinteger(&io_luaState, i);
					lua_gettable(&io_luaState, -2);
					if (lua_isnil(&io_luaState, -1))
					{
						wereThereErrors = true;
						fprintf_s(stderr, "No value for key:%d was found in the table", i);
						lua_pop(&io_luaState, 1);
						goto OnExit;
					}
					if (lua_type(&io_luaState, -1) == LUA_TNUMBER)
					{
						xyz[i - 1] = static_cast<float>(lua_tonumber(&io_luaState, -1));
						lua_pop(&io_luaState, 1);
					}
					else
					{
						wereThereErrors = true;
						fprintf_s(stderr, "The value isn't a number!");
						lua_pop(&io_luaState, 1);
						goto OnExit;
					}
				}
				meshData.vertexData[index].x = xyz[0];
				meshData.vertexData[index].y = xyz[1];
				meshData.vertexData[index].z = xyz[2];
			}
			else
			{
				wereThereErrors = true;
				fprintf_s(stderr, "There are %d coordinates instead of 3", arrayLength);
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			fprintf_s(stderr, "The value at \"%d\" must be a table (instead of a %s) ", (index + 1), luaL_typename(&io_luaState, -1));
			goto OnExit;
		}
	OnExit:
		// Pop the XY table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}

	bool LoadColorsTable(lua_State& io_luaState, MeshData&meshData)
	{
		bool wereThereErrors = false;
		const char* const key = "colors";
		lua_pushstring(&io_luaState, key);
		lua_gettable(&io_luaState, -2);
		if (lua_isnil(&io_luaState, -1))
		{
			wereThereErrors = true;
			fprintf_s(stderr, "No value for key:\"%s\" was found in the table", key);
			goto OnExit;
		}
		if (lua_type(&io_luaState, -1) == LUA_TTABLE)
		{
			const int colors = luaL_len(&io_luaState, -1);
			if (colors != meshData.numberOfVertices)
			{
				wereThereErrors = true;
				fprintf_s(stderr, "The number of color tables and position tables do not match");
				goto OnExit;
			}
			else
			{
				for (int i = 1; i <= meshData.numberOfVertices; ++i)
				{
					if (!LoadRGBATable(io_luaState, meshData, (i - 1)))
					{
						wereThereErrors = true;
						goto OnExit;
					}
				}
			}
		}
		else
		{
			wereThereErrors = true;
			fprintf_s(stderr, "The value at \"%s\" must be a table (instead of a %s)", key, luaL_typename(&io_luaState, -1));
			goto OnExit;
		}
	OnExit:
		// Pop the Colors table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}
	bool LoadRGBATable(lua_State& io_luaState, MeshData&meshData, int index)
	{
		bool wereThereErrors = false;
		lua_pushinteger(&io_luaState, index + 1);
		lua_gettable(&io_luaState, -2);
		if (lua_isnil(&io_luaState, -1))
		{
			wereThereErrors = true;
			fprintf_s(stderr, "No value for key:%d was found in the table", (index + 1));
			goto OnExit;
		}
		if (lua_type(&io_luaState, -1) == LUA_TTABLE)
		{
			const int arrayLength = luaL_len(&io_luaState, -1);
			float rgba[4] = { 0.0f,0.0f,0.0f,1.0f };
			if ((arrayLength == 3) || (arrayLength == 4))
			{
				for (int i = 1; i <= arrayLength; ++i)
				{
					lua_pushinteger(&io_luaState, i);
					lua_gettable(&io_luaState, -2);
					if (lua_isnil(&io_luaState, -1))
					{
						wereThereErrors = true;
						fprintf_s(stderr, "No value for key:%d was found in the table", i);
						lua_pop(&io_luaState, 1);
						goto OnExit;
					}
					if (lua_type(&io_luaState, -1) == LUA_TNUMBER)
					{
						rgba[i - 1] = static_cast<float>(lua_tonumber(&io_luaState, -1));
						lua_pop(&io_luaState, 1);
					}
					else
					{
						wereThereErrors = true;
						fprintf_s(stderr, "The value isn't a number!");
						lua_pop(&io_luaState, 1);
						goto OnExit;
					}
				}

				if (CheckIfColorIsInCorrectFormat(rgba))
				{
					meshData.vertexData[index].r = static_cast<uint8_t>(roundf(rgba[0] * 255.0f));
					meshData.vertexData[index].g = static_cast<uint8_t>(roundf(rgba[1] * 255.0f));
					meshData.vertexData[index].b = static_cast<uint8_t>(roundf(rgba[2] * 255.0f));
					meshData.vertexData[index].a = static_cast<uint8_t>(roundf(rgba[3] * 255.0f));
				}
				else
				{
					wereThereErrors = true;
					fprintf_s(stderr, "The color values were not normalized in range [0,1]");
					goto OnExit;
				}

			}
			else
			{
				wereThereErrors = true;
				fprintf_s(stderr, "There are %d channels instead of 3 or 4", arrayLength);
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			fprintf_s(stderr, "The value at \"%d\" must be a table (instead of a %s)", (index + 1), luaL_typename(&io_luaState, -1));
			goto OnExit;
		}
	OnExit:
		// Pop the RGBA table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}

	bool LoadIndicesTable(lua_State& io_luaState)
	{
		bool wereThereErrors = false;
		const char* const key = "indices";
		lua_pushstring(&io_luaState, key);
		lua_gettable(&io_luaState, -2);
		if (lua_isnil(&io_luaState, -1))
		{
			wereThereErrors = true;
			fprintf_s(stderr, "No value for key:\"%s\" was found in the table", key);
			goto OnExit;
		}
		if (lua_type(&io_luaState, -1) == LUA_TTABLE)
		{
			const int arrayLength = luaL_len(&io_luaState, -1);
			if (arrayLength > USHRT_MAX)
			{
				InitializeMeshData(false);
			}
			else
			{
				InitializeMeshData(true);
			}
			if (arrayLength % 3 == 0)
			{
				meshData<type>.numberOfIndices = arrayLength;
				meshData.indexData = reinterpret_cast<uint16_t*>(malloc(meshData.numberOfIndices * sizeof(uint16_t)));
				// Remember that Lua arrays are 1-based and not 0-based!
				for (int i = 1; i <= arrayLength; ++i)
				{
					lua_pushinteger(&io_luaState, i);
					lua_gettable(&io_luaState, -2);
					if (lua_isnil(&io_luaState, -1))
					{
						wereThereErrors = true;
						fprintf_s(stderr, "No value for key:%d was found in the table", i);
						lua_pop(&io_luaState, 1);
						goto OnExit;
					}
					if (lua_type(&io_luaState, -1) == LUA_TNUMBER)
					{
#if defined( EAE6320_PLATFORM_D3D )
						meshData.indexData[meshData.numberOfIndices - i] = static_cast<uint16_t>(lua_tonumber(&io_luaState, -1));
#elif defined( EAE6320_PLATFORM_GL )
						meshData.indexData[i - 1] = static_cast<uint16_t>(lua_tonumber(&io_luaState, -1));
#endif
						lua_pop(&io_luaState, 1);
					}
					else
					{
						wereThereErrors = true;
						fprintf_s(stderr, "The value isn't a number!");
						lua_pop(&io_luaState, 1);
						goto OnExit;
					}
				}
			}
			else
			{
				wereThereErrors = true;
				fprintf_s(stderr, "There are %d indices which is incorrect as we are drawing triangles", arrayLength);
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			fprintf_s(stderr, "The value at \"%s\" must be a table (instead of a %s)", key, luaL_typename(&io_luaState, -1));
			goto OnExit;
		}
	OnExit:
		// Pop the RGBA table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}

	bool CheckIfColorIsInCorrectFormat(float *rgba)
	{
		for (size_t i = 0; i < 4; i++)
		{
			if (rgba[i]<0.0f || rgba[i]>1.0f)
			{
				return false;
			}
		}
		return true;
	}
}