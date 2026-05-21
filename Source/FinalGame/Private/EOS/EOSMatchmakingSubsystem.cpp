#include "EOS/EOSMatchmakingSubsystem.h"
#include "EOS/EOSLobbySubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EOS/EOSIdentitySubsystem.h"

#pragma region Matchmaking Flow

void UEOSMatchmakingSubsystem::StartMatchmaking()
{
    UEOSLobbySubsystem* LobbySub = GetGameInstance()->GetSubsystem<UEOSLobbySubsystem>();
    if (!LobbySub) return;

    const TArray<FLobbyMemberInfo> Members = LobbySub->GetCurrentMembers();
    if (Members.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[Matchmaking] No members in lobby — aborting."));
        return;
    }

    TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject());

    // Generate a unique lobby ID per matchmaking call to track the session
    JsonObj->SetStringField(TEXT("lobby_id"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));
    JsonObj->SetNumberField(TEXT("player_count"), Members.Num());

    // Send the EOS UniqueNetId of every lobby member so the Go backend can build the teams map
    TArray<TSharedPtr<FJsonValue>> PlayerArray;
    PlayerArray.Reserve(Members.Num());

    for (const FLobbyMemberInfo& M : Members)
    {
        PlayerArray.Add(MakeShared<FJsonValueString>(M.UniqueIdString));
    }

    JsonObj->SetArrayField(TEXT("players"), PlayerArray);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

    UE_LOG(LogTemp, Warning, TEXT("[Matchmaking] Request body: %s"), *JsonString);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    // Custom Go Matchmaker endpoint
    Request->SetURL(TEXT("http://104.194.157.137:8080/matchmake"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonString);
    Request->OnProcessRequestComplete().BindUObject(this, &UEOSMatchmakingSubsystem::OnMatchmakingResponse);

    // Allow up to 2 minutes for the matchmaking ticket to resolve
    Request->SetTimeout(120.0f);

    LobbySub->SyncMatchmakingState(FString::Printf(TEXT("Searching... (%d/2)"), Members.Num()));

    UE_LOG(LogTemp, Warning, TEXT("[Matchmaking] Sending request to Go backend with %d players..."), Members.Num());
    Request->ProcessRequest();
}

#pragma endregion

#pragma region HTTP Callbacks

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

    // Cache the dedicated server IP returned by the backend
    PendingServerIP = JsonResponse->GetStringField(TEXT("ip"));

    FString TeamAssignmentsJson;
    const TSharedPtr<FJsonObject>* TeamsObj = nullptr;

    // Parse team assignments if provided by the backend
    if (JsonResponse->TryGetObjectField(TEXT("teams"), TeamsObj) && TeamsObj && TeamsObj->IsValid())
    {
        TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&TeamAssignmentsJson);
        FJsonSerializer::Serialize((*TeamsObj).ToSharedRef(), Writer);

        if (UEOSIdentitySubsystem* ID = GetGameInstance()->GetSubsystem<UEOSIdentitySubsystem>())
        {
            TSharedPtr<const FUniqueNetId> LocalId = ID->GetLocalUserId().GetUniqueNetId();
            if (LocalId.IsValid())
            {
                FString MyNetIdString = LocalId->ToString();
                FString LeftPart, RightPart;

                // EOS IDs often contain a provider prefix, s plit to get the clean ID
                if (MyNetIdString.Split(TEXT("|"), &LeftPart, &RightPart))
                {
                    MyNetIdString = RightPart;
                }

                int32 MyTeam = -1;

                UE_LOG(LogTemp, Warning, TEXT("[Matchmaking DEBUG] Looking for Local ID: '%s' in Teams JSON: %s"), *MyNetIdString, *TeamAssignmentsJson);

                // Safely extract the team ID whether it's formatted as a number or a string in the JSON
                if (!(*TeamsObj)->TryGetNumberField(MyNetIdString, MyTeam))
                {
                    FString MyTeamStr;
                    if ((*TeamsObj)->TryGetStringField(MyNetIdString, MyTeamStr))
                    {
                        MyTeam = FCString::Atoi(*MyTeamStr);
                    }
                }

                if (MyTeam > 0)
                {
                    PendingTeamID = MyTeam;
                    UE_LOG(LogTemp, Warning, TEXT("[Matchmaking] Success! Assigned to team: %d"), PendingTeamID);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Matchmaking] Response had no 'teams' object — no team assignments will be propagated."));
    }

    UE_LOG(LogTemp, Warning, TEXT("[Matchmaking] Raw teams map from Go: %s"), *TeamAssignmentsJson);
    UE_LOG(LogTemp, Warning, TEXT("[Matchmaking] Leader's own team resolved to: %d"), PendingTeamID);
    UE_LOG(LogTemp, Warning, TEXT("[Matchmaking] Match found — Server: %s | LocalTeam: %d"), *PendingServerIP, PendingTeamID);

    // If the local player is the lobby leader, broadcast the server info to all clients in the lobby
    if (UEOSLobbySubsystem* LobbySub = GetGameInstance()->GetSubsystem<UEOSLobbySubsystem>())
    {
        if (LobbySub->IsLobbyLeader())
        {
            LobbySub->SyncMatchmakingState(TEXT("Match Found!"));
            LobbySub->BroadcastMatchInfo(PendingServerIP, TeamAssignmentsJson);
        }
    }
}

#pragma endregion