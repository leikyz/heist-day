//#include "Network/Client/EOSClientSubsystem.h"
//#include "OnlineSessionSettings.h"
//#include "OnlineSubsystemUtils.h"
//#include "OnlineSubsystem.h"
//#include <Online/OnlineSessionNames.h>
//
//
//
//bool UEOSClientSubsystem::ShouldCreateSubsystem(UObject* Outer) const
//{
//	return !IsRunningDedicatedServer();
//}
//
//void UEOSClientSubsystem::Initialize(FSubsystemCollectionBase& Collection)
//{
//	Super::Initialize(Collection);
//
//	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
//	if (Subsystem)
//	{
//		SessionInterface = Subsystem->GetSessionInterface();
//		IdentityInterface = Subsystem->GetIdentityInterface();
//
//		if (SessionInterface.IsValid())
//		{
//			SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
//				FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UEOSClientSubsystem::OnSessionUserInviteAccepted));
//		}
//	}
//}
//
//void UEOSClientSubsystem::LoginWithDevAuth()
//{
//	if (!IdentityInterface.IsValid()) return;
//
//	FOnlineAccountCredentials Credentials;
//	Credentials.Type = TEXT("developer");
//	Credentials.Id = TEXT("localhost:8081");
//	Credentials.Token = TEXT("DevUser");
//
//	FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), Credentials.Token);
//
//	IdentityInterface->AddOnLoginCompleteDelegate_Handle(0,
//		FOnLoginCompleteDelegate::CreateUObject(this, &UEOSClientSubsystem::OnLoginComplete));
//
//	UE_LOG(LogTemp, Warning, TEXT("CLIENT : Tentative de connexion..."));
//	IdentityInterface->Login(0, Credentials);
//}
//
//void UEOSClientSubsystem::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
//{
//	bIsLoggedIn = bWasSuccessful;
//
//	if (bWasSuccessful)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("CLIENT : Login REUSSI"));
//		CreateLobby();
//	}
//	else
//	{
//		UE_LOG(LogTemp, Error, TEXT("CLIENT : Login ECHOUE"));
//	}
//
//	OnLoginStateChanged.Broadcast(bWasSuccessful);
//}
//
//FString UEOSClientSubsystem::GetPlayerName()
//{
//	if (bIsLoggedIn && IdentityInterface.IsValid())
//	{
//		FUniqueNetIdPtr LocalNetId = IdentityInterface->GetUniquePlayerId(0);
//		if (LocalNetId.IsValid())
//		{
//			return IdentityInterface->GetPlayerNickname(*LocalNetId);
//		}
//	}
//	return TEXT("Non Connecteeeee");
//}
//
//void UEOSClientSubsystem::CreateLobby()
//{
//	if (!SessionInterface.IsValid()) return;
//
//	FOnlineSessionSettings LobbySettings;
//	LobbySettings.bIsDedicated = false;
//	LobbySettings.bUseLobbiesIfAvailable = true;
//	LobbySettings.bUsesPresence = true;
//	LobbySettings.bAllowJoinViaPresence = true;
//	LobbySettings.bAllowJoinInProgress = true;
//	LobbySettings.bShouldAdvertise = true;
//	LobbySettings.NumPublicConnections = 4;
//
//	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
//		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UEOSClientSubsystem::OnCreateLobbyComplete));
//
//	SessionInterface->CreateSession(0, FName("PartyLobby"), LobbySettings);
//}
//
//void UEOSClientSubsystem::OnCreateLobbyComplete(FName SessionName, bool bWasSuccessful)
//{
//	if (bWasSuccessful)
//	{
//		FUniqueNetIdPtr LocalNetId = IdentityInterface->GetUniquePlayerId(0);
//		if (LocalNetId.IsValid())
//		{
//			SessionInterface->RegisterLocalPlayer(*LocalNetId, SessionName, FOnRegisterLocalPlayerCompleteDelegate::CreateUObject(this, &UEOSClientSubsystem::OnRegisterLocalPlayerComplete));
//		}
//	}
//}
//
//void UEOSClientSubsystem::OnRegisterLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
//{
//	UE_LOG(LogTemp, Warning, TEXT("CLIENT : Joueur enregistre dans le Lobby."));
//}
//
//void UEOSClientSubsystem::OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
//{
//	if (bWasSuccessful && SessionInterface.IsValid())
//	{
//		SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
//			FOnJoinSessionCompleteDelegate::CreateUObject(this, &UEOSClientSubsystem::OnJoinSessionComplete));
//
//		SessionInterface->JoinSession(0, FName("PartyLobby"), InviteResult);
//	}
//}
//
//void UEOSClientSubsystem::FindSessions()
//{
//	if (!SessionInterface.IsValid()) return;
//
//	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
//	LastSessionSearch->MaxSearchResults = 20;
//	LastSessionSearch->bIsLanQuery = false;
//	LastSessionSearch->QuerySettings.Set(SEARCH_DEDICATED_ONLY, true, EOnlineComparisonOp::Equals);
//	LastSessionSearch->QuerySettings.Set(SETTING_MAPNAME, FString("NewMap"), EOnlineComparisonOp::Equals);
//
//	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
//		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UEOSClientSubsystem::OnFindSessionsComplete));
//
//	SessionInterface->FindSessions(0, LastSessionSearch.ToSharedRef());
//}
//
//void UEOSClientSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
//{
//	UE_LOG(LogTemp, Warning, TEXT("CLIENT : Recherche terminee."));
//	SessionInterface->ClearOnFindSessionsCompleteDelegates(this);
//}
//
//void UEOSClientSubsystem::JoinGameSession(int32 Index)
//{
//	if (!SessionInterface.IsValid() || !LastSessionSearch.IsValid()) return;
//
//	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
//		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UEOSClientSubsystem::OnJoinSessionComplete));
//
//	SessionInterface->JoinSession(0, FName("MainSession"), LastSessionSearch->SearchResults[Index]);
//}
//
//void UEOSClientSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
//{
//	if (Result == EOnJoinSessionCompleteResult::Success)
//	{
//		FString ConnectString;
//		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
//		{
//			UE_LOG(LogTemp, Warning, TEXT("CLIENT : Travel vers %s"), *ConnectString);
//		}
//	}
//	SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
//}