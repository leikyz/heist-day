#include "EOS/EOSServerSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

#pragma region Lifecycle

bool UEOSServerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return IsRunningDedicatedServer();
}

void UEOSServerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Bind the session creation delegate before attempting to create the session
	CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UEOSServerSubsystem::OnCreateSessionComplete);

	CreateServerSession();
}

#pragma endregion

#pragma region Server Session

void UEOSServerSubsystem::CreateServerSession()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem) return;

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	// Configure settings specifically for a 2v2 public dedicated server match
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bIsDedicated = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.NumPublicConnections = 2; // 2v2 setup
	SessionSettings.bUseLobbiesIfAvailable = false;

	SessionSettings.Set(FName("BucketId"), FString("MainMatchmaking"), EOnlineDataAdvertisementType::ViaOnlineService);

	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	UE_LOG(LogTemp, Warning, TEXT("SERVER: Creating OSSv1 Dedicated Server Session..."));

	SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
}

#pragma endregion

#pragma region Session Callbacks

void UEOSServerSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			// Clean up the delegate handle immediately to prevent memory leaks or duplicate calls
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("SERVER: OSSv1 Session created successfully! Now listening for matchmaking clients."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SERVER: OSSv1 Session creation FAILED."));
	}
}

#pragma endregion