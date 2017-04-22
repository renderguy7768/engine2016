#include "NetworkManager.h"
#include "NetworkScene.h"
#include "../Asserts/Asserts.h"
#include "../Logging/Logging.h"
#include "../../External/RakNet/Includes.h"
#include "../../Game/Gameplay/GameObject.h"
#include "../Graphics/Graphics.h"

namespace
{
	enum GameMessages
	{
		ID_CREATE_PLAYER = ID_USER_PACKET_ENUM + 1,
		ID_UPDATE_OTHER_PLAYER = ID_USER_PACKET_ENUM + 2
	};

	char const * const hardCodedPlayerPath = "data/gameobjects/network/playerthirdpersonremote.bingobj";
	char const * const localIPV4 = "10.0.0.117";//"127.0.0.1";

	bool notDone = true;

	const char *StartupResultTable[static_cast<int>(RakNet::STARTUP_OTHER_FAILURE) + 1] =
	{
		"RAKNET_STARTED",
		"RAKNET_ALREADY_STARTED",
		"INVALID_SOCKET_DESCRIPTORS",
		"INVALID_MAX_CONNECTIONS",
		"SOCKET_FAMILY_NOT_SUPPORTED",
		"SOCKET_PORT_ALREADY_IN_USE",
		"SOCKET_FAILED_TO_BIND",
		"SOCKET_FAILED_TEST_SEND",
		"PORT_CANNOT_BE_ZERO",
		"FAILED_TO_CREATE_NETWORK_THREAD",
		"COULD_NOT_GENERATE_GUID",
		"STARTUP_OTHER_FAILURE"
	};
	const char *ConnectionAttemptResultTable[static_cast<int>(RakNet::SECURITY_INITIALIZATION_FAILED) + 1] =
	{
		"CONNECTION_ATTEMPT_STARTED",
		"INVALID_PARAMETER",
		"CANNOT_RESOLVE_DOMAIN_NAME",
		"ALREADY_CONNECTED_TO_ENDPOINT",
		"CONNECTION_ATTEMPT_ALREADY_IN_PROGRESS",
		"SECURITY_INITIALIZATION_FAILED"
	};
}

eae6320::Network::NetworkManager* eae6320::Network::NetworkManager::singleton(nullptr);
eae6320::Gameplay::GameObject* eae6320::Network::NetworkManager::nativePlayer(nullptr);

eae6320::Network::NetworkManager::NetworkManager(bool i_isServer, uint16_t i_serverPort, uint32_t i_maxClients)
	:
	m_remotePlayer(nullptr),
	m_rakPeerInterface(nullptr),
	m_maxClients(i_maxClients),
	m_serverPort(i_serverPort),
	m_isServer(i_isServer)
{}


bool eae6320::Network::NetworkManager::Initialize(bool i_isServer, uint16_t i_serverPort, uint32_t i_maxClients)
{
	bool wereThereErrors = false;
	singleton = new NetworkManager(i_isServer, i_serverPort, i_maxClients);

	// Getting the RakNet Instance
	{
		singleton->m_rakPeerInterface = RakNet::RakPeerInterface::GetInstance();
		if (!singleton->m_rakPeerInterface)
		{
			wereThereErrors = true;
			EAE6320_ASSERT(false);
			Logging::OutputError("Failed to get the RakNet instance");
			goto OnExit;
		}
	}
	if (singleton->m_isServer)
	{
		// Starting as Server
		{
			RakNet::SocketDescriptor socketDescriptor(i_serverPort, nullptr);
			const RakNet::StartupResult result = singleton->m_rakPeerInterface->Startup(singleton->m_maxClients, &socketDescriptor, 1);
			if (result != RakNet::RAKNET_STARTED)
			{
				wereThereErrors = true;
				EAE6320_ASSERT(false);
				Logging::OutputError("Failed to start RakNet: %s", StartupResultTable[result]);
				goto OnExit;
			}
		}

		// Setting Max Number of Clients
		singleton->m_rakPeerInterface->SetMaximumIncomingConnections(singleton->m_maxClients);
	}
	else
	{
		// Starting as Client	
		{
			RakNet::SocketDescriptor socketDescriptor;
			const RakNet::StartupResult result = singleton->m_rakPeerInterface->Startup(1, &socketDescriptor, 1);
			if (result != RakNet::RAKNET_STARTED)
			{
				wereThereErrors = true;
				EAE6320_ASSERT(false);
				Logging::OutputError("Failed to start RakNet: %s", StartupResultTable[result]);
				goto OnExit;
			}
		}

		// Attempeting to Connect to Server
		{
			const RakNet::ConnectionAttemptResult result = singleton->m_rakPeerInterface->Connect(localIPV4, singleton->m_serverPort, nullptr, 0);
			if (result != RakNet::CONNECTION_ATTEMPT_STARTED)
			{
				wereThereErrors = true;
				EAE6320_ASSERT(false);
				Logging::OutputError("Failed to start connection attempt to server: %s", ConnectionAttemptResultTable[result]);
				goto OnExit;
			}
		}
	}
OnExit:
	return !wereThereErrors;
}

bool eae6320::Network::NetworkManager::CleanUp()
{
	if (singleton)
	{
		if (singleton->m_remotePlayer)
		{
			delete singleton->m_remotePlayer;
			singleton->m_remotePlayer = nullptr;
		}
		RakNet::RakPeerInterface::DestroyInstance(singleton->m_rakPeerInterface);
		singleton->m_rakPeerInterface = nullptr;
		delete singleton;
		singleton = nullptr;
	}
	return true;
}

eae6320::Network::NetworkManager* eae6320::Network::NetworkManager::GetSingleton()
{
	if (singleton)return singleton;
	EAE6320_ASSERTF(false, "Initialize Network Manager");
	Logging::OutputError("Network Manager not initialized");
	return nullptr;
}

void eae6320::Network::NetworkManager::ProcessIncomingPackets()
{
	RakNet::Packet* packet;
	for (packet = m_rakPeerInterface->Receive(); packet; m_rakPeerInterface->DeallocatePacket(packet), packet = m_rakPeerInterface->Receive())
	{
		switch (packet->data[0])
		{
		case ID_CONNECTION_REQUEST_ACCEPTED:
			// Send ID_CREATE_PLAYER alongwith initial player transform to server
		{
			RakNet::BitStream bsOut;
			bsOut.Write(static_cast<RakNet::MessageID>(ID_CREATE_PLAYER));
			m_rakPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		}
		break;
		case ID_CONNECTION_ATTEMPT_FAILED:
			// Try to reconnect to server
			if (!m_isServer)
			{
				const RakNet::ConnectionAttemptResult result = m_rakPeerInterface->Connect(localIPV4, singleton->m_serverPort, nullptr, 0);
				if (result != RakNet::CONNECTION_ATTEMPT_STARTED)
				{
					EAE6320_ASSERT(false);
					Logging::OutputError("Failed to start connection attempt to server: %s", ConnectionAttemptResultTable[result]);
				}
			}
			break;
		case ID_ALREADY_CONNECTED:
			// Do nothing
			break;
		case ID_NEW_INCOMING_CONNECTION:
			// Send ID_CREATE_PLAYER alongwith initial player transform to client	
		{
			RakNet::BitStream bsOut;
			bsOut.Write(static_cast<RakNet::MessageID>(ID_CREATE_PLAYER));
			m_rakPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		}
		break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			// Should not happen in our case. Assert false and log an error
			EAE6320_ASSERT(false);
			Logging::OutputError("Number of max connections to the sever exceeded");
			break;
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			// Delete remote player on both
			if (m_remotePlayer)
			{
				delete m_remotePlayer;
				m_remotePlayer = nullptr;
			}
			// Try to reconnect to server
			if (!m_isServer)
			{
				const RakNet::ConnectionAttemptResult result = m_rakPeerInterface->Connect(localIPV4, singleton->m_serverPort, nullptr, 0);
				if (result != RakNet::CONNECTION_ATTEMPT_STARTED)
				{
					EAE6320_ASSERT(false);
					Logging::OutputError("Failed to start connection attempt to server: %s", ConnectionAttemptResultTable[result]);
				}
			}
			break;
		case ID_CREATE_PLAYER:
			if (!m_remotePlayer)
			{
				m_remotePlayer = Gameplay::GameObject::LoadGameObject(hardCodedPlayerPath);
			}
			else
			{
				delete m_remotePlayer;
				m_remotePlayer = nullptr;
				m_remotePlayer = Gameplay::GameObject::LoadGameObject(hardCodedPlayerPath);
			}

			if (notDone && !m_isServer)
			{
				Math::cVector position, orientationEular;
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				bsIn.ReadVector(position.x, position.y, position.z);
				bsIn.IgnoreBytes(sizeof(Math::cVector));
				bsIn.ReadVector(orientationEular.x, orientationEular.y, orientationEular.z);
				position += Math::cVector(0.0f, 0.0f, 0.0f);
				m_remotePlayer->SetTransformSpecial(position, orientationEular);
				nativePlayer->SetTransformSpecial(position, orientationEular);
				notDone = false;
			}

			{
				RakNet::BitStream bsOut;
				bsOut.Write(static_cast<RakNet::MessageID>(ID_UPDATE_OTHER_PLAYER));
				Gameplay::Transform nativePlayerTransform = nativePlayer->GetTransform();
				bsOut.WriteVector(nativePlayerTransform.m_position.x, nativePlayerTransform.m_position.y, nativePlayerTransform.m_position.z);
				bsOut.WriteVector(nativePlayerTransform.GetOrientationEular().x, nativePlayerTransform.GetOrientationEular().y, nativePlayerTransform.GetOrientationEular().z);
				m_rakPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
			}
			NetworkScene::currentGameState = NetworkScene::RunMultiplayer;
			break;
		case ID_UPDATE_OTHER_PLAYER:
			// For both update remote player
		{
			Math::cVector position, orientationEular;
			// Read the packet and write value to remote player transform
			{
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				bsIn.ReadVector(position.x, position.y, position.z);
				bsIn.IgnoreBytes(sizeof(Math::cVector));
				bsIn.ReadVector(orientationEular.x, orientationEular.y, orientationEular.z);
				m_remotePlayer->SetTransformSpecial(position, orientationEular);
			}

			{
				RakNet::BitStream bsOut;
				bsOut.Write(static_cast<RakNet::MessageID>(ID_UPDATE_OTHER_PLAYER));
				Gameplay::Transform nativePlayerTransform = nativePlayer->GetTransform();
				bsOut.WriteVector(nativePlayerTransform.m_position.x, nativePlayerTransform.m_position.y, nativePlayerTransform.m_position.z);
				bsOut.WriteVector(nativePlayerTransform.GetOrientationEular().x, nativePlayerTransform.GetOrientationEular().y, nativePlayerTransform.GetOrientationEular().z);
				m_rakPeerInterface->Send(&bsOut, HIGH_PRIORITY, UNRELIABLE, 0, packet->systemAddress, false);
			}
		}
		break;
		default:;
		}
	}
}

void eae6320::Network::NetworkManager::Draw()const
{
	if (m_remotePlayer)Graphics::SetGameObject(m_remotePlayer);
}
