#include "Network/Server/EOSServerSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

void UEOSServerSubsystem::CreateServerSession()
{
	// Fetch the stable OSSv1 subsystem
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("No OnlineSubsystem found!"));
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("No Session Interface found!"));
		return;
	}

	// Configure the Dedicated Server Session Settings
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bIsDedicated = true; 
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.NumPublicConnections = 16;
	SessionSettings.bUseLobbiesIfAvailable = false;
	SessionSettings.bUsesPresence = false; // Servers don't have presence

	SessionSettings.Set(FName("BucketId"), FString("DedicatedServer"), EOnlineDataAdvertisementType::ViaOnlineService);

	SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UEOSServerSubsystem::OnCreateSessionComplete);

	UE_LOG(LogTemp, Log, TEXT("Attempting to Create Dedicated Server Session via OSSv1..."));

	// Create the session (0 is safely ignored because bIsDedicated is true)
	SessionInterface->CreateSession(0, FName("GameSession"), SessionSettings);
}

void UEOSServerSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// Clean up the delegate
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		Subsystem->GetSessionInterface()->ClearOnCreateSessionCompleteDelegates(this);
	}

	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT(">>> DEDICATED SERVER SESSION CREATED SUCCESSFULLY! <<<"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create OSSv1 Session!"));
	}
}