#include "Network/Client/EOSIdentitySubsystem.h"
#include "Network/Client/EOSLobbySubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Engine/GameInstance.h"

void UEOSIdentitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Ensure LobbySubsystem is initialized first
	Collection.InitializeDependency<UEOSLobbySubsystem>();

	UE_LOG(LogTemp, Log, TEXT("[EOSIdentity] Subsystem initialized"));
}

void UEOSIdentitySubsystem::Deinitialize()
{
	if (IOnlineIdentityPtr Identity = GetIdentityInterface())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginDelegateHandle);
	}
	Super::Deinitialize();
}

void UEOSIdentitySubsystem::LoginWithDevAuthTool()
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity)
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSIdentity] Could not get Identity interface."));
		return;
	}

	FString CredentialToUse;
	if (FParse::Value(FCommandLine::Get(), TEXT("-AUTH_LOGIN="), CredentialToUse))
	{
		UE_LOG(LogTemp, Log, TEXT("[EOSIdentity] Using command line credential: '%s'"), *CredentialToUse);
	}
	else
	{
		// Fallback just in case we run from the editor without a .bat file
		CredentialToUse = TEXT("DevUser");
		UE_LOG(LogTemp, Warning, TEXT("[EOSIdentity] No -AUTH_LOGIN found. Falling back to default: '%s'"), *CredentialToUse);
	}

	LoginDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(
		0,
		FOnLoginCompleteDelegate::CreateUObject(this, &UEOSIdentitySubsystem::HandleLoginComplete)
	);

	FOnlineAccountCredentials Credentials;
	Credentials.Type = TEXT("developer");
	Credentials.Id = TEXT("localhost:9000");
	Credentials.Token = CredentialToUse; // Use the parsed string here

	UE_LOG(LogTemp, Log, TEXT("[EOSIdentity] Attempting EOS login as '%s'..."), *CredentialToUse);
	Identity->Login(0, Credentials);
}

void UEOSIdentitySubsystem::HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (Identity)
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);
	}

	if (bWasSuccessful)
	{
		if (Identity)
		{
			TSharedPtr<FUserOnlineAccount> UserAccount = Identity->GetUserAccount(UserId);
			if (UserAccount.IsValid())
			{
				CachedDisplayName = UserAccount->GetDisplayName();
			}

			if (CachedDisplayName.IsEmpty())
			{
				CachedDisplayName = Identity->GetPlayerNickname(LocalUserNum);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[EOSIdentity] Login successful! Display name: '%s'  PUID: '%s'"),
			*CachedDisplayName,
			*UserId.ToString());

		OnLoginComplete.Broadcast(true, CachedDisplayName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSIdentity] Login FAILED. Error: %s"), *Error);
		OnLoginComplete.Broadcast(false, FString());
	}
}

bool UEOSIdentitySubsystem::IsLoggedIn() const
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity) return false;
	return Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

FUniqueNetIdRepl UEOSIdentitySubsystem::GetLocalUserId() const
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity) return FUniqueNetIdRepl();
	TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(0);
	if (!UserId.IsValid()) return FUniqueNetIdRepl();
	return FUniqueNetIdRepl(*UserId);
}

IOnlineSubsystem* UEOSIdentitySubsystem::GetOSS() const
{
	return IOnlineSubsystem::Get();
}

IOnlineIdentityPtr UEOSIdentitySubsystem::GetIdentityInterface() const
{
	IOnlineSubsystem* OSS = GetOSS();
	if (!OSS) return nullptr;
	return OSS->GetIdentityInterface();
}