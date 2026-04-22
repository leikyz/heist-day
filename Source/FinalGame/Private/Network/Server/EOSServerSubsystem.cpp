#include "Network/Server/EOSServerSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

bool UEOSServerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return IsRunningDedicatedServer();
}

void UEOSServerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Bind the delegate
	CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UEOSServerSubsystem::OnCreateSessionComplete);

	CreateServerSession();
}

void UEOSServerSubsystem::CreateServerSession()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem) return;

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bIsDedicated = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.NumPublicConnections = 2; // 1v1 setup
	SessionSettings.bUseLobbiesIfAvailable = false;

	SessionSettings.Set(FName("BucketId"), FString("MainMatchmaking"), EOnlineDataAdvertisementType::ViaOnlineService);

	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	UE_LOG(LogTemp, Warning, TEXT("SERVER: Creating OSSv1 Dedicated Server Session..."));

	SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
}

void UEOSServerSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
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