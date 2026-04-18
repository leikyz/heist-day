#include "Network/Client/EOSIdentitySubsystem.h"
#include "OnlineSubsystem.h"
#include "Misc/CommandLine.h"

bool UEOSIdentitySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer();
}

void UEOSIdentitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UEOSIdentitySubsystem::LoginWithDevAuth()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem) return;

	IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
	if (!IdentityInterface.IsValid()) return;

	FString DevAuthToken = TEXT("DevUser");
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), DevAuthToken);

	// Setup OSSv1 Credentials
	FOnlineAccountCredentials Credentials;
	Credentials.Type = TEXT("developer");
	Credentials.Id = TEXT("localhost:9000");
	Credentials.Token = DevAuthToken;

	UE_LOG(LogTemp, Warning, TEXT("CLIENT (Identity OSSv1): Logging in with [%s]..."), *DevAuthToken);

	// Bind the callback and execute login
	IdentityInterface->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateUObject(this, &UEOSIdentitySubsystem::OnLoginComplete));
	IdentityInterface->Login(0, Credentials);
}

void UEOSIdentitySubsystem::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem && Subsystem->GetIdentityInterface().IsValid())
	{
		Subsystem->GetIdentityInterface()->ClearOnLoginCompleteDelegates(0, this);
	}

	bIsLoggedIn = bWasSuccessful;

	if (bIsLoggedIn)
	{
		LocalAccountId = UserId.AsShared();
		UE_LOG(LogTemp, Warning, TEXT("CLIENT (Identity OSSv1): Login SUCCESS. AccountId valid."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CLIENT (Identity OSSv1): Login FAILED. Error: %s"), *Error);
	}

	OnLoginStateChanged.Broadcast(bIsLoggedIn);
}

FString UEOSIdentitySubsystem::GetPlayerName() const
{
	if (!bIsLoggedIn || !LocalAccountId.IsValid()) return TEXT("Non loggé");

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
		if (IdentityInterface.IsValid())
		{
			return IdentityInterface->GetPlayerNickname(0);
		}
	}

	return TEXT("Unknown");
}