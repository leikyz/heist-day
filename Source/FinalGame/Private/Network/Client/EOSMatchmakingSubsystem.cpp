#include "Network/Client/EOSMatchmakingSubsystem.h"
#include "Network/Client/EOSIdentitySubsystem.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "TimerManager.h"

using namespace UE::Online;

bool UEOSMatchmakingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer();
}

IOnlineServicesPtr UEOSMatchmakingSubsystem::GetServices() const
{
	return UE::Online::GetServices();
}

void UEOSMatchmakingSubsystem::StartMatchmaking()
{
	UEOSIdentitySubsystem* Identity = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();
	if (!Identity || !Identity->IsLoggedIn()) return;

	FindAvailableMatch();
}

void UEOSMatchmakingSubsystem::FindAvailableMatch()
{
	IOnlineServicesPtr Services = GetServices();
	if (!Services) return;

	ISessionsPtr Sessions = Services->GetSessionsInterface();
	UEOSIdentitySubsystem* Identity = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>();

	FFindSessions::Params Params;
	Params.LocalAccountId = Identity->GetLocalAccountId();
	Params.MaxResults = 10;

	// Use BucketId from DefaultEngine.ini
	FFindSessionsSearchFilter Filter;
	Filter.Key = FName(TEXT("BucketId"));
	Filter.ComparisonOp = ESchemaAttributeComparisonOp::Equals;
	Filter.Value = FSchemaVariant(TEXT("MainMatchmaking"));
	Params.Filters.Add(Filter);

	OnMatchmakingStatusChanged.Broadcast(TEXT("Searching (1/2)..."));

	Sessions->FindSessions(MoveTemp(Params)).OnComplete([this, Identity](const TOnlineResult<FFindSessions>& Result)
		{
			if (Result.IsOk() && Result.GetOkValue().FoundSessionIds.Num() > 0)
			{
				FJoinSession::Params JoinParams;
				JoinParams.LocalAccountId = Identity->GetLocalAccountId();
				JoinParams.SessionName = FName(TEXT("gamesession"));
				JoinParams.SessionId = Result.GetOkValue().FoundSessionIds[0];

				GetServices()->GetSessionsInterface()->JoinSession(MoveTemp(JoinParams)).OnComplete(
					[this](const TOnlineResult<FJoinSession>& JoinResult) { OnMatchJoined(JoinResult); });
			}
			else
			{
				OnMatchmakingStatusChanged.Broadcast(TEXT("No Match Found."));
			}
		});
}

void UEOSMatchmakingSubsystem::OnMatchJoined(const TOnlineResult<FJoinSession>& Result)
{
	if (Result.IsOk())
	{
		// Logical join successful. Now we start polling the session count.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(MatchTimerHandle, this, &UEOSMatchmakingSubsystem::CheckMatchStatus, 2.0f, true);
		}

		OnMatchmakingStatusChanged.Broadcast(TEXT("Waiting for opponent (1/2)..."));
	}
}

void UEOSMatchmakingSubsystem::CheckMatchStatus()
{
	IOnlineServicesPtr Services = GetServices();
	if (!Services) return;

	ISessionsPtr Sessions = Services->GetSessionsInterface();

	FGetSessionByName::Params Params;
	Params.LocalName = FName(TEXT("GameSession"));

	auto Result = Sessions->GetSessionByName(MoveTemp(Params));
	if (Result.IsOk())
	{
		// Check how many players are logically in this session on the backend
		int32 Count = Result.GetOkValue().Session->GetSessionMembers().Num();

		if (Count >= 2)
		{
			// STOP POLLING
			if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(MatchTimerHandle);

			OnMatchmakingStatusChanged.Broadcast(TEXT("Match Found (2/2)!"));

			// Travel to ServerIP
			auto& Settings = Result.GetOkValue().Session->GetSessionSettings().CustomSettings;
			if (Settings.Contains(FName(TEXT("ServerIP"))))
			{
				FString IP = Settings[FName(TEXT("ServerIP"))].Data.GetString();
				if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
				{
					PC->ClientTravel(IP, TRAVEL_Absolute);
				}
			}
		}
	}
}

void UEOSMatchmakingSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MatchTimerHandle);
	}
	Super::Deinitialize();
}