#include "Network/Client/EOSIdentitySubsystem.h"
#include "Network/Client/EOSLobbySubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Engine/GameInstance.h"

void UEOSIdentitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UEOSLobbySubsystem>();
}

void UEOSIdentitySubsystem::Deinitialize()
{
	if (IOnlineIdentityPtr Identity = GetIdentityInterface())
		Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginDelegateHandle);

	Super::Deinitialize();
}

void UEOSIdentitySubsystem::LoginWithDevAuthTool()
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid()) return;

	FString Credential;
	if (!FParse::Value(FCommandLine::Get(), TEXT("-AUTH_LOGIN="), Credential))
		Credential = TEXT("DevUser");

	LoginDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(0,
		FOnLoginCompleteDelegate::CreateUObject(this, &UEOSIdentitySubsystem::HandleLoginComplete));

	FOnlineAccountCredentials Creds;
	Creds.Type = TEXT("developer");
	Creds.Id = TEXT("localhost:9000");
	Creds.Token = Credential;
	Identity->Login(0, Creds);
}

void UEOSIdentitySubsystem::HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (IOnlineIdentityPtr Identity = GetIdentityInterface())
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);

	if (bWasSuccessful)
	{
		if (IOnlineIdentityPtr Identity = GetIdentityInterface())
		{
			TSharedPtr<FUserOnlineAccount> Account = Identity->GetUserAccount(UserId);
			CachedDisplayName = Account.IsValid() ? Account->GetDisplayName() : Identity->GetPlayerNickname(LocalUserNum);
		}
		OnLoginComplete.Broadcast(true, CachedDisplayName);
	}
	else
	{
		OnLoginComplete.Broadcast(false, TEXT(""));
	}
}

bool UEOSIdentitySubsystem::IsLoggedIn() const
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	return Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

FUniqueNetIdRepl UEOSIdentitySubsystem::GetLocalUserId() const
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid()) return FUniqueNetIdRepl();
	TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(0);
	return UserId.IsValid() ? FUniqueNetIdRepl(*UserId) : FUniqueNetIdRepl();
}

IOnlineSubsystem* UEOSIdentitySubsystem::GetOSS()              const { return IOnlineSubsystem::Get(); }
IOnlineIdentityPtr UEOSIdentitySubsystem::GetIdentityInterface() const { IOnlineSubsystem* OSS = GetOSS(); return OSS ? OSS->GetIdentityInterface() : nullptr; }