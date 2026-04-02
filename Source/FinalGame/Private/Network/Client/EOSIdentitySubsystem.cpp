#include "Network/Client/EOSIdentitySubsystem.h"

using namespace UE::Online;

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
	IOnlineServicesPtr Services = GetServices();
	if (!Services) return;

	IAuthPtr Auth = Services->GetAuthInterface();
	if (!Auth) return;

	FString DevAuthToken = TEXT("DevUser");
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), DevAuthToken);

	FAuthLogin::Params LoginParams;

	// Mandatory OSSv2 binding
	LoginParams.PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(0);

	LoginParams.CredentialsType = LoginCredentialsType::Developer;
	LoginParams.CredentialsId = TEXT("localhost:9000");
	LoginParams.CredentialsToken.Set<FString>(DevAuthToken);

	UE_LOG(LogTemp, Warning, TEXT("CLIENT (Identity OSSv2): Logging in with [%s]..."), *DevAuthToken);

	Auth->Login(MoveTemp(LoginParams)).OnComplete(
		[this](const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result)
		{
			OnLoginComplete(Result);
		}
	);
}
void UEOSIdentitySubsystem::OnLoginComplete(const TOnlineResult<FAuthLogin>& Result)
{
	bIsLoggedIn = Result.IsOk();

	if (bIsLoggedIn)
	{
		LocalAccountId = Result.GetOkValue().AccountInfo->AccountId;
		UE_LOG(LogTemp, Warning, TEXT("CLIENT (Identity OSSv2): Login SUCCESS. AccountId valid."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CLIENT (Identity OSSv2): Login FAILED. Error: %s"), *Result.GetErrorValue().GetLogString());
	}

	OnLoginStateChanged.Broadcast(bIsLoggedIn);
}

FString UEOSIdentitySubsystem::GetPlayerName() const
{
	if (!bIsLoggedIn || !LocalAccountId.IsValid()) return TEXT("Non loggé");

	return UE::Online::ToLogString(LocalAccountId);
}