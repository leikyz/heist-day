//#pragma once
//
//#include "CoreMinimal.h"
//#include "Network/EOSNetworkSubsystem.h"
//#include "Interfaces/OnlineSessionInterface.h"
//#include "EOSClientSubsystem.generated.h"
//
//UCLASS()
//class FINALGAME_API UEOSClientSubsystem : public UEOSNetworkSubsystem
//{
//	GENERATED_BODY()
//
//public:
//	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
//	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
//
//	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoginStateChanged, bool, bSuccess);
//
//	UPROPERTY(BlueprintAssignable, Category = "EOS")
//	FOnLoginStateChanged OnLoginStateChanged;
//	// --- FONCTIONS BLUEPRINT ---
//
//	UFUNCTION(BlueprintCallable, Category = "EOS")
//	void LoginWithDevAuth();
//
//	UFUNCTION(BlueprintCallable, Category = "EOS")
//	bool IsLoggedIn() const { return bIsLoggedIn; }
//
//	UFUNCTION(BlueprintCallable, Category = "EOS")
//	FString GetPlayerName();
//
//	UFUNCTION(BlueprintCallable, Category = "EOS")
//	void FindSessions();
//
//	UFUNCTION(BlueprintCallable, Category = "EOS")
//	void JoinGameSession(int32 Index);
//
//	// --- LOGIQUE INTERNE ---
//
//	void CreateLobby();
//	void OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
//
//protected:
//	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
//
//	void OnFindSessionsComplete(bool bWasSuccessful);
//	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
//	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
//	void OnCreateLobbyComplete(FName SessionName, bool bWasSuccessful);
//	void OnRegisterLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result);
//
//private:
//	IOnlineSessionPtr SessionInterface;
//	IOnlineIdentityPtr IdentityInterface;
//
//	// Simple variable pour verifier l'etat depuis l'UI
//	bool bIsLoggedIn = false;
//};