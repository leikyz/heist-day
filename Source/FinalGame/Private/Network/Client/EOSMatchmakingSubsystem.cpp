#include "Network/Client/EOSMatchmakingSubsystem.h"

bool UEOSMatchmakingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only run on clients
	return !IsRunningDedicatedServer();
}

void UEOSMatchmakingSubsystem::StartMatchmaking()
{
	// TODO: Integrate custom Go backend communication here
	// Examples: Send HTTP POST to /matchmake or connect to a Matchmaking WebSocket

	OnMatchmakingStatusChanged.Broadcast(TEXT("Connecting to custom matchmaking server..."));
}