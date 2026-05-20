#include "HeistDayGameMode.h"
#include "TimerManager.h"
#include "HeistDayPlayerState.h"
#include "HeistDayGameState.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Kismet/GameplayStatics.h"
#include "Network/Client/EOSMatchmakingSubsystem.h"

#pragma region Lifecycle & Connection

void AHeistDayGameMode::BeginPlay()
{
	Super::BeginPlay();
	CachedGameState = GetGameState<AHeistDayGameState>();

	// Cache all dynamic map objects so we can rebuild the map exactly at halftime
	SaveCarryablesInitialState();

	if (IsRunningDedicatedServer())
	{
		NotifyServerReady();
	}
}

FString AHeistDayGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// Extract the TeamID assigned by the matchmaking backend via the connection URL parameters
	if (AHeistDayPlayerState* PS = NewPlayerController->GetPlayerState<AHeistDayPlayerState>())
	{
		int32 ParsedTeamId = UGameplayStatics::GetIntOption(Options, TEXT("Team"), 0);

		if (ParsedTeamId > 0)
		{
			PS->SetTeamId(ParsedTeamId);
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] Parsed TeamID %d from URL options for %s"), ParsedTeamId, *NewPlayerController->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameMode] Failed to parse TeamID from URL options: %s"), *Options);
		}
	}

	return ErrorMessage;
}

void AHeistDayGameMode::PostLogin(APlayerController* NewPlayer)
{
	ConnectedCount++;

	AHeistDayPlayerState* PS = NewPlayer->GetPlayerState<AHeistDayPlayerState>();
	if (!PS || !CachedGameState)
	{
		Super::PostLogin(NewPlayer);
		return;
	}

	int32 AssignedTeamId = PS->GetTeamId();

	// Fallback logic for local testing without the matchmaking backend
	if (AssignedTeamId <= 0)
	{
		AssignedTeamId = (ConnectedCount % 2 != 0) ? 1 : 2;
		PS->SetTeamId(AssignedTeamId);
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] No Team in URL, fallback to AssignedTeamId %d"), AssignedTeamId);
	}

	if (AssignedTeamId == 1)
	{
		PS->SetTeam(ETeam::Thief);
		ThiefCount++;
		PS->SetPlayerIndex(ThiefCount);
	}
	else
	{
		PS->SetTeam(ETeam::Employee);
		EmployeeCount++;
		PS->SetPlayerIndex(EmployeeCount);
	}

	// Register player in the GameState's MatchData for global score tracking
	FTeamData& TargetTeamData = (AssignedTeamId == 1)
		? CachedGameState->CurrentMatchData.FirstTeam
		: CachedGameState->CurrentMatchData.SecondTeam;

	CachedGameState->CurrentMatchData.FirstTeam.EmployeeScore = CachedGameState->GetMuseumValue() / 2;
	CachedGameState->CurrentMatchData.SecondTeam.EmployeeScore = CachedGameState->GetMuseumValue() / 2;

	if (TargetTeamData.TeamId == 0)
	{
		TargetTeamData.TeamId = AssignedTeamId;
		TargetTeamData.Team = PS->GetTeam();
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Initializing Team %d in MatchData"), AssignedTeamId);
	}

	TargetTeamData.Players.Add(PS);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player %s added to MatchData structure for Team %d"), *PS->GetPlayerName(), AssignedTeamId);

	Super::PostLogin(NewPlayer);

	// Force an initial spawn spot selection before they fully enter the world
	if (AActor* InitialStart = ChoosePlayerStart(NewPlayer))
	{
		NewPlayer->StartSpot = InitialStart;
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] StartSpot initialized to %s for %s"), *InitialStart->GetName(), *NewPlayer->GetName());
	}
}

void AHeistDayGameMode::OnClientReady()
{
	ReadyPlayersCount++;
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] A client is ready! (%d/%d)"), ReadyPlayersCount, ExpectedPlayerCount);

	// Wait for all clients to load before starting the match countdown
	if (ReadyPlayersCount == ExpectedPlayerCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] All players are ready! Starting PreRound..."));

		if (CachedGameState)
		{
			CachedGameState->Server_SetMatchPhase(EMatchPhase::FirstRoundStart);
		}

		FTimerHandle StartDelay;
		GetWorldTimerManager().SetTimer(StartDelay, [this]()
			{
				StartRound(1);
			}, 14.0f, false);
	}
}

#pragma endregion

#pragma region Backend Communication

void AHeistDayGameMode::NotifyServerReady()
{
	if (GetNumValidPlayerStarts() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] PlayerStarts are not detected. Retrying in 1s..."));

		FTimerHandle RetryHandle;
		GetWorldTimerManager().SetTimer(RetryHandle, this, &AHeistDayGameMode::NotifyServerReady, 1.0f, false);
		return;
	}

	int32 ServerPort = GetWorld()->URL.Port;
	FParse::Value(FCommandLine::Get(), TEXT("port="), ServerPort);

	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject());
	JsonObj->SetNumberField(TEXT("server_port"), ServerPort);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("http://104.194.157.137:8080/server_ready"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);
	Request->OnProcessRequestComplete().BindUObject(this, &AHeistDayGameMode::OnServerReadyResponse);
	Request->ProcessRequest();
}

void AHeistDayGameMode::NotifyServerReadyWithRetry()
{
	if (GetNumValidPlayerStarts() > 0)
	{
		NotifyServerReady();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] PlayerStarts are not ready. Retrying in 1s..."));

		FTimerHandle RetryHandle;
		GetWorldTimerManager().SetTimer(RetryHandle, this, &AHeistDayGameMode::NotifyServerReadyWithRetry, 1.0f, false);
	}
}

void AHeistDayGameMode::OnServerReadyResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Backend acknowledged Server Ready. Matchmaking will now send players."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] Failed to notify Backend of Server Ready. Go Server might be down."));
	}
}

void AHeistDayGameMode::NotifyMatchEndAndShutdown()
{
	int32 ServerPort = GetWorld()->URL.Port;
	FParse::Value(FCommandLine::Get(), TEXT("port="), ServerPort);

	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject());
	JsonObj->SetNumberField(TEXT("server_port"), ServerPort);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("http://127.0.0.1:8080/match_end"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);
	Request->OnProcessRequestComplete().BindUObject(this, &AHeistDayGameMode::OnMatchEndResponse);
	Request->ProcessRequest();

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Sending /match_end on port %d. Initiating auto-destruction sequence..."), ServerPort);
}

void AHeistDayGameMode::OnMatchEndResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Backend acknowledged Match End. Shutting down process. Goodbye!"));

	// Force closes the dedicated server instance once the match data is offloaded
	FGenericPlatformMisc::RequestExit(false);
}

int32 AHeistDayGameMode::GetNumValidPlayerStarts() const
{
	int32 Count = 0;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		Count++;
	}
	return Count;
}

#pragma endregion

#pragma region Spawning & Environment Reset

AActor* AHeistDayGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	AHeistDayPlayerState* PS = Player->GetPlayerState<AHeistDayPlayerState>();

	if (!PS)
	{
		UE_LOG(LogTemp, Error, TEXT("[ChoosePlayerStart] Failed : PlayerState is NULL for %s !"), *Player->GetName());
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	// Filter spawn locations based on the assigned Team Role
	FName TargetTag = (PS->GetTeam() == ETeam::Thief) ? FName("Thief") : FName("Employee");

	TArray<APlayerStart*> ValidStarts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Start = *It;
		if (Start->PlayerStartTag == TargetTag)
		{
			ValidStarts.Add(Start);
		}
	}

	ValidStarts.Sort([](const APlayerStart& A, const APlayerStart& B)
		{
			return A.GetName() < B.GetName();
		});

	// Assign the spawn point using modulo to ensure no overlapping index if player counts exceed spawn counts
	if (ValidStarts.Num() > 0)
	{
		int32 SpawnIndex = FMath::Max(0, PS->GetPlayerIndex() - 1);
		SpawnIndex = SpawnIndex % ValidStarts.Num();
		UE_LOG(LogTemp, Warning, TEXT("[ChoosePlayerStart] Success : Spawn %s assigned to %s (PlayerIndex : %d)"),
			*ValidStarts[SpawnIndex]->GetName(), *Player->GetName(), PS->GetPlayerIndex());

		return ValidStarts[SpawnIndex];
	}

	UE_LOG(LogTemp, Error, TEXT("[ChoosePlayerStart] Error: No valid spawn found for team %s !"), *TargetTag.ToString());

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AHeistDayGameMode::TeleportPlayersToNewSpawns()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			AActor* NewStart = ChoosePlayerStart(PC);

			if (NewStart)
			{
				FVector SpawnLocation = NewStart->GetActorLocation();
				FRotator SpawnRotation = NewStart->GetActorRotation();

				// Move the pawn explicitly since we're keeping the same pawn between rounds
				PC->GetPawn()->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
				PC->StartSpot = NewStart;

				if (auto* Movement = PC->GetPawn()->FindComponentByClass<UCharacterMovementComponent>())
				{
					Movement->StopMovementImmediately();
				}
			}
		}
	}
}

void AHeistDayGameMode::SaveCarryablesInitialState()
{
	InitialCarryablesData.Empty();

	// Cache Transforms for all dynamic physics/interactable classes
	if (CarryableBaseClass)
	{
		for (TActorIterator<AActor> It(GetWorld(), CarryableBaseClass); It; ++It)
		{
			if (AActor* Item = *It)
			{
				FCarryableSpawnData Data;
				Data.CarryableClass = Item->GetClass();
				Data.InitialTransform = Item->GetActorTransform();
				InitialCarryablesData.Add(Data);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] CarryableBaseClass is not assigned! Set BP_CarryableBase in the GameMode Blueprint."));
	}

	if (KeycardBaseClass)
	{
		for (TActorIterator<AActor> It(GetWorld(), KeycardBaseClass); It; ++It)
		{
			if (AActor* Item = *It)
			{
				FCarryableSpawnData Data;
				Data.CarryableClass = Item->GetClass();
				Data.InitialTransform = Item->GetActorTransform();
				InitialCarryablesData.Add(Data);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] KeycardBaseClass is not assigned! Set BP_Keycard_Pickup in the GameMode Blueprint."));
	}

	if (FracturableBaseClass)
	{
		for (TActorIterator<AActor> It(GetWorld(), FracturableBaseClass); It; ++It)
		{
			if (AActor* Item = *It)
			{
				FCarryableSpawnData Data;
				Data.CarryableClass = Item->GetClass();
				Data.InitialTransform = Item->GetActorTransform();
				InitialCarryablesData.Add(Data);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] FracturableBaseClass is not assigned!"));
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] %d objects (Carryables + Keycards + Fracturables) saved at startup."), InitialCarryablesData.Num());
}

void AHeistDayGameMode::ResetCarryables()
{
	// Destroy any existing interactables that might have been moved or modified during Round 1
	if (CarryableBaseClass)
	{
		for (TActorIterator<AActor> It(GetWorld(), CarryableBaseClass); It; ++It)
		{
			if (AActor* Item = *It)
			{
				Item->Destroy();
			}
		}
	}

	if (KeycardBaseClass)
	{
		for (TActorIterator<AActor> It(GetWorld(), KeycardBaseClass); It; ++It)
		{
			if (AActor* Item = *It)
			{
				Item->Destroy();
			}
		}
	}

	if (FracturableBaseClass)
	{
		for (TActorIterator<AActor> It(GetWorld(), FracturableBaseClass); It; ++It)
		{
			if (AActor* Item = *It)
			{
				Item->Destroy();
			}
		}
	}

	// Respawn everything perfectly using the initial cached transforms
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FCarryableSpawnData& Data : InitialCarryablesData)
	{
		if (Data.CarryableClass)
		{
			GetWorld()->SpawnActor<AActor>(Data.CarryableClass, Data.InitialTransform, SpawnParams);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] %d objects (Carryables + Keycards + Fracturables) reset!"), InitialCarryablesData.Num());
}

#pragma endregion

#pragma region Combat & Stats Tracking

void AHeistDayGameMode::HandlePlayerDamage(AController* Victim, AController* Attacker, float DamageAmount)
{
	AHeistDayPlayerState* VictimPS = Victim->GetPlayerState<AHeistDayPlayerState>();
	AHeistDayPlayerState* AttackerPS = Attacker->GetPlayerState<AHeistDayPlayerState>();
	AHeistDayGameState* GS = GetGameState<AHeistDayGameState>();

	if (!VictimPS) return;

	float NewHealth = VictimPS->GetCurrentHealth() - DamageAmount;
	VictimPS->SetCurrentHealth(FMath::Max(0.f, NewHealth));

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s took %.1f damage from %s, new health: %d"),
		*Victim->GetName(), DamageAmount, Attacker ? *Attacker->GetName() : TEXT("Environment"), VictimPS->GetCurrentHealth());

	// If player health reaches 0, award kill/death stats based on the active round
	if (VictimPS->IsDead())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s has died!"), *Victim->GetName());

		if (GS->MatchPhase == EMatchPhase::FirstRound)
		{
			VictimPS->AddDeath(1);

			if (AttackerPS)
			{
				AttackerPS->AddKill(1);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s killed by environment or unknown source."), *Victim->GetName());
			}
		}
		else if (GS->MatchPhase == EMatchPhase::SecondRound)
		{
			VictimPS->AddDeath(2);

			if (AttackerPS)
			{
				AttackerPS->AddKill(2);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s killed by environment or unknown source."), *Victim->GetName());
			}
		}
	}
}

void AHeistDayGameMode::HandleChangePlayerHealthValue(AController* Victim, int32 NewHealth)
{
	AHeistDayPlayerState* PS = Victim->GetPlayerState<AHeistDayPlayerState>();
	if (!PS) return;

	if (NewHealth < 0 || NewHealth > AHeistDayPlayerState::MaxHealth)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleChangePlayerHealthValue] Try to set invalid health value (%d) for %s"), NewHealth, *Victim->GetName());
		return;
	}

	PS->SetCurrentHealth(NewHealth);
}

void AHeistDayGameMode::ResetAllPlayersHealth()
{
	if (!CachedGameState) return;

	for (APlayerState* PS : CachedGameState->PlayerArray)
	{
		AHeistDayPlayerState* HeistPS = Cast<AHeistDayPlayerState>(PS);
		if (HeistPS)
		{
			HeistPS->SetCurrentHealth(AHeistDayPlayerState::MaxHealth);
			UE_LOG(LogTemp, Log, TEXT("[GameMode] Health reset for: %s"), *HeistPS->GetPlayerName());
		}
	}
}

void AHeistDayGameMode::HandlePlayerInterception(AController* Interceptor)
{
	if (!Interceptor) return;

	AHeistDayPlayerState* PS = Interceptor->GetPlayerState<AHeistDayPlayerState>();
	AHeistDayGameState* GS = GetGameState<AHeistDayGameState>();

	if (PS && GS)
	{
		if (GS->GetMatchPhase() == EMatchPhase::FirstRound)
		{
			PS->AddInterception(1);
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s made an interception in Round 1!"), *Interceptor->GetName());
		}
		else if (GS->GetMatchPhase() == EMatchPhase::SecondRound)
		{
			PS->AddInterception(2);
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s made an interception in Round 2!"), *Interceptor->GetName());
		}
	}
}

void AHeistDayGameMode::HandlePlayerRobbed(AController* Robber)
{
	if (!Robber) return;

	AHeistDayPlayerState* PS = Robber->GetPlayerState<AHeistDayPlayerState>();
	AHeistDayGameState* GS = GetGameState<AHeistDayGameState>();

	if (PS && GS)
	{
		if (GS->GetMatchPhase() == EMatchPhase::FirstRound)
		{
			PS->AddRobbed(1);
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s stole loot in Round 1!"), *Robber->GetName());
		}
		else if (GS->GetMatchPhase() == EMatchPhase::SecondRound)
		{
			PS->AddRobbed(2);
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s stole loot in Round 2!"), *Robber->GetName());
		}
	}
}

bool AHeistDayGameMode::CheckAllThiefDead()
{
	if (!CachedGameState) return false;

	for (APlayerState* PS : CachedGameState->PlayerArray)
	{
		AHeistDayPlayerState* HeistPS = Cast<AHeistDayPlayerState>(PS);
		if (HeistPS && HeistPS->GetTeam() == ETeam::Thief && !HeistPS->IsDead())
		{
			return false;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] All thiefs are dead!"));
	return true;
}

#pragma endregion

#pragma region Scoring

void AHeistDayGameMode::AwardThiefScore(int32 TeamId, int32 ScoreToAdd)
{
	if (CachedGameState)
	{
		CachedGameState->Server_SetThiefScore(TeamId, ScoreToAdd);
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Referee awarded %d Thief points to team %d"), ScoreToAdd, TeamId);

		// Scoring for Thieves dynamically updates the opposing team's Employee score (subtracts from their total possible defense points)
		AwardEmployeeScore(CachedGameState->GetOpposingTeamDataById(TeamId).TeamId, ScoreToAdd);
	}
}

void AHeistDayGameMode::AwardEmployeeScore(int32 TeamId, int32 ScoreToAdd)
{
	if (CachedGameState)
	{
		CachedGameState->Server_SetEmployeeScore(TeamId, ScoreToAdd);
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Referee awarded %d Employee points to team %d"), ScoreToAdd, TeamId);
	}
}

#pragma endregion

#pragma region Round Management

void AHeistDayGameMode::StartRound(int roundNumber)
{
	UsedPlayerStarts.Empty();

	if (!CachedGameState) return;

	CachedGameState->Server_SetRemainingTime(RoundDuration);

	switch (roundNumber)
	{
	case 1:
		CachedGameState->Server_SetMatchPhase(EMatchPhase::FirstRound);
		break;
	case 2:
		CachedGameState->Server_SetMatchPhase(EMatchPhase::SecondRound);
		break;
	default:
		CachedGameState->Server_SetMatchPhase(EMatchPhase::FirstRound);
		break;
	}

	GetWorldTimerManager().SetTimer(RoundTimerHandle,
		this, &AHeistDayGameMode::OnRoundTimerExpired,
		RoundDuration, false);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Round #%d started — %.0f s"), roundNumber, RoundDuration);
}

void AHeistDayGameMode::SetAlarmTimer(float NewTime)
{
	GetWorldTimerManager().SetTimer(
		RoundTimerHandle,
		this,
		&AHeistDayGameMode::OnRoundTimerExpired,
		NewTime,
		false
	);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Timer updated following alarm: %.0f s"), NewTime);
}

void AHeistDayGameMode::SwapAllTeamsRoles()
{
	if (!CachedGameState) return;

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] SwapAllTeamsRoles: Starting role swap."));

	FMatchData NewMatchData = CachedGameState->CurrentMatchData;

	// Lambda helper to flip the Enum value for an entire team group
	auto SwapLogic = [](FTeamData& TeamData)
		{
			FString OldRoleStr = (TeamData.Team == ETeam::Thief) ? TEXT("Thief") : TEXT("Employee");

			TeamData.Team = (TeamData.Team == ETeam::Thief) ? ETeam::Employee : ETeam::Thief;

			FString NewRoleStr = (TeamData.Team == ETeam::Thief) ? TEXT("Thief") : TEXT("Employee");

			UE_LOG(LogTemp, Warning, TEXT("[GameMode] Team %d Swap: %s -> %s"), TeamData.TeamId, *OldRoleStr, *NewRoleStr);

			int32 PlayerCount = 0;
			for (AHeistDayPlayerState* PS : TeamData.Players)
			{
				if (IsValid(PS))
				{
					PS->SetTeam(TeamData.Team);
					PlayerCount++;
					UE_LOG(LogTemp, Log, TEXT("[GameMode] PlayerState %s updated with role %s"), *PS->GetName(), *NewRoleStr);
				}
			}

			UE_LOG(LogTemp, Log, TEXT("[GameMode] %d players updated for Team %d"), PlayerCount, TeamData.TeamId);
		};

	SwapLogic(NewMatchData.FirstTeam);
	SwapLogic(NewMatchData.SecondTeam);

	int32 TempCount = ThiefCount;
	ThiefCount = EmployeeCount;
	EmployeeCount = TempCount;

	CachedGameState->CurrentMatchData = NewMatchData;

	// Broadcast data updates to clients so their UI reflects the new roles immediately
	CachedGameState->OnTeamValueChanged.Broadcast(CachedGameState->CurrentMatchData.FirstTeam);
	CachedGameState->OnTeamValueChanged.Broadcast(CachedGameState->CurrentMatchData.SecondTeam);

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] SwapAllTeamsRoles: Finished and broadcasted locally."));
}

void AHeistDayGameMode::OnRoundTimerExpired()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Round over."));

	if (CachedGameState == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] CachedGameState is null in OnRoundTimerExpired!"));
		return;
	}

	// This state machine drives the entire match progression
	switch (CachedGameState->GetMatchPhase())
	{
	case EMatchPhase::FirstRound:
	{
		// First Round Ends -> Trigger Intermission
		CachedGameState->Server_SetRemainingTime(6.0f);
		CachedGameState->Server_SetMatchPhase(EMatchPhase::FirstRoundEnd);

		SwapAllTeamsRoles();
		CachedGameState->Server_SetIsAlarming(false);

		GetWorldTimerManager().SetTimer(RoundTimerHandle, [this]()
			{
				// Physically reset the map and player stats for Round 2
				TeleportPlayersToNewSpawns();
				ResetAllPlayersHealth();
				ResetCarryables();

				CachedGameState->Server_SetRemainingTime(14.0f);
				CachedGameState->Server_SetMatchPhase(EMatchPhase::SecondRoundStart);

				GetWorldTimerManager().SetTimer(RoundTimerHandle, [this]()
					{
						StartRound(2);
					}, 14.0f, false);

			}, 6.0f, false);
		break;
	}
	case EMatchPhase::SecondRound:
	{
		// Second Round Ends -> Trigger Final Match Screen
		CachedGameState->Server_SetRemainingTime(6.0f);
		CachedGameState->Server_SetMatchPhase(EMatchPhase::SecondRoundEnd);
		CachedGameState->Server_SetIsAlarming(false);

		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Second round ended. Match should end or restart after delay."));

		GetWorldTimerManager().SetTimer(RoundTimerHandle, [this]()
			{
				CachedGameState->Server_SetMatchPhase(EMatchPhase::MatchEnd);

				// Wait enough time for clients to view the scoreboard before auto-destroying the server instance
				FTimerHandle ShutdownTimerHandle;
				GetWorldTimerManager().SetTimer(ShutdownTimerHandle, this, &AHeistDayGameMode::NotifyMatchEndAndShutdown, 21.0f, false);

			}, 6.0f, false);

		break;
	}

	default:
		break;
	}
}

#pragma endregion