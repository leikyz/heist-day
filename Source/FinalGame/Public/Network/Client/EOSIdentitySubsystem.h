#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "EOSIdentitySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEOSLoginComplete, bool, bWasSuccessful, FString, DisplayName);

UCLASS()
class FINALGAME_API UEOSIdentitySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
#pragma region Initialization

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#pragma endregion

#pragma region Authentication

	// Attempts to log in using the local EOS Developer Authentication Tool
	UFUNCTION(BlueprintCallable, Category = "EOS|Identity")
	void LoginWithDevAuthTool();

	// Fired when an EOS login attempt finishes
	UPROPERTY(BlueprintAssignable, Category = "EOS|Identity")
	FOnEOSLoginComplete OnLoginComplete;

#pragma endregion

#pragma region Getters

	// Returns true if the local player is currently authenticated with EOS
	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	bool IsLoggedIn() const;

	// Retrieves the unique network identifier for the local user
	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	FUniqueNetIdRepl GetLocalUserId() const;

	// Retrieves the cached display name of the local user
	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	FString GetLocalDisplayName() const { return CachedDisplayName; }

#pragma endregion

private:
#pragma region Internal Helpers

	void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	IOnlineSubsystem* GetOSS() const;
	IOnlineIdentityPtr GetIdentityInterface() const;

#pragma endregion

#pragma region Cached Data

	FDelegateHandle LoginDelegateHandle;
	FString CachedDisplayName;

#pragma endregion
};