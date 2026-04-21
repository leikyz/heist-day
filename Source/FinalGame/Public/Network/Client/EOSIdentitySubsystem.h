#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "EOSIdentitySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEOSLoginComplete, bool, bWasSuccessful, const FString&, DisplayName);

UCLASS()
class FINALGAME_API UEOSIdentitySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "EOS|Identity")
	void LoginWithDevAuthTool();

	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	bool IsLoggedIn() const;

	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	FString GetLocalDisplayName() const { return CachedDisplayName; }

	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	FUniqueNetIdRepl GetLocalUserId() const;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Identity")
	FOnEOSLoginComplete OnLoginComplete;

private:
	void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	IOnlineSubsystem* GetOSS() const;
	IOnlineIdentityPtr GetIdentityInterface() const;

	FString CachedDisplayName;

	// Delegate handle for cleanup
	FDelegateHandle LoginDelegateHandle;
};