#ifndef ENTRYPOINT_CPP
#define ENTRYPOINT_CPP

#include "impl/include.hpp"

#define FN_POINTER uintptr_t
#define FN_OFFSET ptrdiff_t
#define FN_STORAGE_I int


namespace Offsets {
	//Offsets
	FN_POINTER UWorld = 0xEA84A78; //0xec90bd8
	FN_POINTER ObjectID = 0x18;

	//UWorld
	FN_POINTER OwningGameInstance = 0x1b8; //UGameInstance*
	FN_POINTER Levels = 0x170; //TArray<ULevel*>
	FN_POINTER GameState = 0x158; //AGameStateBase*
	FN_POINTER PersistentLevel = 0x30; //ULevel*


	//UWorld -> PersistentLevel (ULevel)
	FN_POINTER AActors = 0x98;
	FN_POINTER ActorCount = 0xA0;

	//UGameInstance
	FN_POINTER LocalPlayers = 0x38; //TArray<ULocalPlayer*>

	//UPlayer
	FN_POINTER PlayerController = 0x30; //APlayerController*

	//APlayerController
	FN_POINTER AcknowledgedPawn = 0x330; //APawn*
	FN_POINTER PlayerCameraManager = 0x340; //APlayerCameraManager*


	//AActor
	FN_POINTER bHidden = 0x58; //char
	FN_POINTER RootComponent = 0x190; //USceneComponent*
	FN_POINTER CustomTimeDilation = 0x64; //float

	//ACharacter
	FN_POINTER Mesh = 0x310; //USkeletalMeshComponent*

	//APawn
	FN_POINTER PlayerState = 0x2a8; //APlayerState*


	FN_POINTER TeamIndex = 0x10C8; //AFortPlayerStateAthena
	FN_POINTER ReviveFromDBNOTime = 0x45b0; //AFortPlayerStateAthena
	FN_POINTER LocalActorPos = 0x128;
	FN_POINTER CurrentWeapon = 0x948;

}

namespace pointer {
	FN_POINTER Uworld;
	FN_POINTER GameInstance;
	FN_POINTER LocalPlayers;
	FN_POINTER LocalPlayer;
	FN_POINTER LocalPawn;
	FN_POINTER PlayerState;
	FN_POINTER LocalPlayerState;
	FN_POINTER Mesh;
	FN_POINTER RootComponent;
	//FN_VECTOR LocalActorPos;
	FN_POINTER PlayerController;
	FN_POINTER PersistentLevel;
	FN_POINTER ULevel;
	FN_POINTER CameraManager;
	FN_POINTER MyObjectID;
	FN_POINTER CurrentActor;
	FN_POINTER PlayerCameraManager;
	FN_POINTER CameraLocation;
	FN_POINTER BoneArray;
	FN_POINTER CurrentActorMesh;
	//DWORD_PTR closestPawn = NULL;
	//float closestDistance = FLT_MAX;
	FN_POINTER AimbotMesh;
	int TeamIndex;
	int ActorCount;
	FN_POINTER AActors;
}

auto main( ) -> void
{
	// create handle to our driver
	auto result = request->initialize_handle( );
	if ( !result ) {
		std::printf( "\n [log] -> failed to initialize driver.\n" );
		std::cin.get( );
	}
	std::printf( "\n [log] -> driver initialized.\n" );

	// replace this with the process name to attack.
	const auto pid = request->get_process_pid( L"FortniteClient-Win64-Shipping.exe" );
	if ( !pid ) {
		std::printf( "\n [log] -> failed to get process id.\n" );
		std::cin.get( );
	}
	
	// attach to our process id
	auto ret = request->attach( pid );
	if ( !ret ) {
		std::printf( " [log] -> failed to attach to pid.\n" );
		std::cin.get( );
	}
	std::printf( " [pid] -> %i\n", pid );

	// get our base address of process
	auto base_address = request->get_image_base( nullptr );
	if ( !base_address ) {
		std::printf( " [log] -> failed to get base address.\n" );
		std::cin.get( );
	}
	std::printf( " [base_address] -> %I64d\n", base_address );

	// need to call this to get our dtb for reading
	result = request->get_cr3( base_address );
	if ( !result ) {
		std::printf( " [log] -> failed to get cr3.\n" );
		std::cin.get( );
	}

	// now we can read memory :) 
	auto buffer = request->read<std::uintptr_t>( base_address );
	if ( !buffer ) {
		std::printf( " [log] -> failed to get buffer.\n" );
		std::cin.get( );
	}
	std::printf( " buffer -> 0x%p\n", buffer );


	/*
	showing our read speeds 
	*/

	// rpm speed
	const auto rpm_time = [ & ] ( ) -> float
	{
		auto time_now = std::chrono::high_resolution_clock::now( );
		for ( auto x = 0ull; x < 0x10000; x++ ) {
			request->read<std::uintptr_t>( base_address + x );
		}
		auto time_span =
			std::chrono::duration_cast< std::chrono::duration< float> >(std::chrono::high_resolution_clock::now( ) - time_now);
		return time_span.count( );
	};
	
	std::printf( " [(0x10,000)] -> %fs\n", rpm_time( ) );

	const auto rpm_time2 = [ & ] ( ) -> float
	{
		auto time_now = std::chrono::high_resolution_clock::now( );
		for ( auto x = 0ull; x < 0x100000; x++ ) {
			request->read<std::uintptr_t>( base_address + x );
		}
		auto time_span =
			std::chrono::duration_cast< std::chrono::duration< float> >(std::chrono::high_resolution_clock::now( ) - time_now);
		return time_span.count( );
	};
	
	std::printf( " [(0x100,000)] -> %fs\n", rpm_time2( ) );
	std::cin.get( );

	while (true)
	{
		pointer::Uworld = request->read<uint64_t>(base_address + Offsets::UWorld); //+
		std::cout << "uworld: " << pointer::Uworld << std::endl;
		pointer::GameInstance = request->read<uint64_t>(pointer::Uworld + Offsets::OwningGameInstance); //+
		std::cout << "game instance: " << pointer::GameInstance << std::endl;
		pointer::PersistentLevel = request->read<uint64_t>(pointer::Uworld + Offsets::PersistentLevel);
		std::cout << "level: " << pointer::PersistentLevel << std::endl;
		Sleep(3000);
	}
}

#endif // !ENTRYPOINT_CPP
