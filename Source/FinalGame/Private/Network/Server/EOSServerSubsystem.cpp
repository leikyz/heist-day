#include "Network/Server/EOSServerSubsystem.h"
#include "Online/OnlineAsyncOpHandle.h"

using namespace UE::Online;

bool UEOSServerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return IsRunningDedicatedServer();
}

void UEOSServerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreateServerSession();
}

void UEOSServerSubsystem::CreateServerSession()
{
	IOnlineServicesPtr Services = UE::Online::GetServices();
	if (!Services) return;

	ISessionsPtr Sessions = Services->GetSessionsInterface();
	if (!Sessions.IsValid()) return;

	FCreateSession::Params Params;
	Params.SessionName = FName(TEXT("gamesession"));

	Params.SessionSettings.SchemaName = FName(TEXT("gamesession"));
	Params.SessionSettings.NumMaxConnections = 2; // 1v1
	Params.SessionSettings.JoinPolicy = ESessionJoinPolicy::Public;

	// Tag for Matchmaking
	FCustomSessionSetting BucketSetting;
	BucketSetting.Data = FSchemaVariant(TEXT("MainMatchmaking"));
	BucketSetting.Visibility = ESchemaAttributeVisibility::Public;
	Params.SessionSettings.CustomSettings.Add(FName(TEXT("BucketId")), BucketSetting);

	// Tag with Server IP for ClientTravel
	FCustomSessionSetting IPSetting;
	IPSetting.Data = FSchemaVariant(TEXT("127.0.0.1")); // Replace with logic to get actual Server IP
	IPSetting.Visibility = ESchemaAttributeVisibility::Public;
	Params.SessionSettings.CustomSettings.Add(FName(TEXT("ServerIP")), IPSetting);

	UE_LOG(LogTemp, Warning, TEXT("SERVER: Registering 1v1 Session for Matchmaking..."));

	Sessions->CreateSession(MoveTemp(Params)).OnComplete(
		[this](const TOnlineResult<FCreateSession>& Result)
		{
			OnSessionCreated(Result);
		}
	);
}

void UEOSServerSubsystem::OnSessionCreated(const TOnlineResult<FCreateSession>& Result)
{
	if (Result.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("SERVER: Session registered successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SERVER: Session creation FAILED: %s"), *Result.GetErrorValue().GetLogString());
	}
}