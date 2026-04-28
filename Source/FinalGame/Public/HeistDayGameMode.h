#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HeistDayGameMode.generated.h"

/**
 * Base GameMode for the dedicated-server gameplay map.
 * Reads "?Team=N" from the connection URL during InitNewPlayer
 * and writes it onto the player's AFinalGamePlayerState.
 *
 * Reparent your gameplay BP GameMode to this class.
 */
UCLASS()
class FINALGAME_API AHeistDayGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AHeistDayGameMode();

    virtual FString InitNewPlayer(APlayerController* NewPlayerController,
        const FUniqueNetIdRepl& UniqueId,
        const FString& Options,
        const FString& Portal) override;
};