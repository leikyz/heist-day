#include "HeistDayGameMode.h"
#include "Network/Player/NetworkPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

AHeistDayGameMode::AHeistDayGameMode()
{
    // Make sure new players get OUR PlayerState class so the cast below works.
    PlayerStateClass = ANetworkPlayerState::StaticClass();
}

FString AHeistDayGameMode::InitNewPlayer(APlayerController* NewPlayerController,
    const FUniqueNetIdRepl& UniqueId,
    const FString& Options,
    const FString& Portal)
{
    const FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

    // Pull "?Team=N" out of the URL the client connected with.
    const int32 TeamInt = UGameplayStatics::GetIntOption(Options, TEXT("Team"), -1);

    UE_LOG(LogTemp, Warning,
        TEXT("[Server GM] InitNewPlayer Id=%s Options=\"%s\" parsed Team=%d"),
        *UniqueId.ToString(), *Options, TeamInt);

    if (TeamInt < 0)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Server GM] No 'Team' option in URL for %s — leaving team as None."),
            *UniqueId.ToString());
        return Result;
    }

    if (auto* PS = NewPlayerController ? NewPlayerController->GetPlayerState<ANetworkPlayerState>() : nullptr)
    {
        PS->SetTeam(static_cast<ETeam>(TeamInt));
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Server GM] PlayerState was not AFinalGamePlayerState — check PlayerStateClass."));
    }

    return Result;
}