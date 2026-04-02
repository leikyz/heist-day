#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

// --- NOUVEAUX INCLUDES OSSv2 ---
#include "Online/OnlineServices.h"
#include "Online/Auth.h"
#include "Online/Lobbies.h"
#include "Online/Presence.h"
#include "Online/OnlineAsyncOpHandle.h"

#include "EOSNetworkSubsystem.generated.h"

UCLASS()
class FINALGAME_API UEOSNetworkSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Base vide pour l'instant, comme ton code d'origine
};