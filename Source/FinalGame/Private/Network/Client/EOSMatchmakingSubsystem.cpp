#include "Network/Client/EOSMatchmakingSubsystem.h"
#include "Network/Client/EOSLobbySubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void UEOSMatchmakingSubsystem::StartMatchmaking()
{
	UEOSLobbySubsystem* LobbySub = GetGameInstance()->GetSubsystem<UEOSLobbySubsystem>();
	if (!LobbySub) return;

	const int32 MemberCount = LobbySub->GetCurrentMembers().Num();

	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject());
	JsonObj->SetStringField(TEXT("lobby_id"), TEXT("CurrentLobby"));
	JsonObj->SetNumberField(TEXT("player_count"), MemberCount);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("http://localhost:8080/matchmake"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);
	Request->OnProcessRequestComplete().BindUObject(this, &UEOSMatchmakingSubsystem::OnMatchmakingResponse);

	LobbySub->SyncMatchmakingState(FString::Printf(TEXT("Searching... (%d/2)"), MemberCount));

	UE_LOG(LogTemp, Warning, TEXT("[Matchmaking] Sending request to Go backend with %d players..."), MemberCount);
	Request->ProcessRequest();
}

void UEOSMatchmakingSubsystem::OnMatchmakingResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[Matchmaking] HTTP request failed. Is the Go server running?"));
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		UE_LOG(LogTemp, Error, TEXT("[Matchmaking] Server returned %d"), Response->GetResponseCode());
		return;
	}

	TSharedPtr<FJsonObject> JsonResponse;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[Matchmaking] Failed to parse JSON: %s"), *Response->GetContentAsString());
		return;
	}

	const FString ServerIP = JsonResponse->GetStringField(TEXT("ip"));
	const int32   TeamID = JsonResponse->GetIntegerField(TEXT("team"));

	UE_LOG(LogTemp, Warning, TEXT("[Matchmaking] Match found — Server: %s | Team: %d"), *ServerIP, TeamID);

	if (UEOSLobbySubsystem* LobbySub = GetGameInstance()->GetSubsystem<UEOSLobbySubsystem>())
	{
		if (LobbySub->IsLobbyLeader())
		{
			LobbySub->SyncMatchmakingState(TEXT("Match Found!"));
			LobbySub->BroadcastServerIP(ServerIP);
		}
	}
}