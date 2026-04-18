#include "Network/Server/EOSServerSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "JsonObjectConverter.h"
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
	SessionSettings.NumPublicConnections = 2; // 1v1
	SessionSettings.bUseLobbiesIfAvailable = false;

	SessionSettings.Set(FName("BucketId"), FString("MainMatchmaking"), EOnlineDataAdvertisementType::ViaOnlineService);

	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	UE_LOG(LogTemp, Warning, TEXT("SERVER: Creating OSSv1 Dedicated Server Session..."));

	SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
}
void UEOSServerSubsystem::RegisterServerWithBackend()
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("ip", "127.0.0.1"); 
	JsonObject->SetStringField("port", "7777");

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->OnProcessRequestComplete().BindUObject(this, &UEOSServerSubsystem::OnServerRegistrationComplete);
	Request->SetURL("http://127.0.0.1:8080/api/server/register");
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(JsonString);

	Request->ProcessRequest();
}

void UEOSServerSubsystem::OnServerRegistrationComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (bConnectedSuccessfully && Response->GetResponseCode() == 200)
	{
		UE_LOG(LogTemp, Warning, TEXT("SERVER: Successfully registered with Go Backend!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SERVER: Failed to register with Go Backend."));
	}
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
		UE_LOG(LogTemp, Warning, TEXT("SERVER: OSSv1 Session created successfully!"));
		RegisterServerWithBackend();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SERVER: OSSv1 Session creation FAILED."));
	}
}