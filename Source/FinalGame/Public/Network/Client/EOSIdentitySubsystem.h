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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "EOS|Identity")
	void LoginWithDevAuthTool();

	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	bool IsLoggedIn() const;

	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	FUniqueNetIdRepl GetLocalUserId() const;

	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	FString GetLocalDisplayName() const { return CachedDisplayName; }

	UPROPERTY(BlueprintAssignable, Category = "EOS|Identity")
	FOnEOSLoginComplete OnLoginComplete;

private:
	void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	IOnlineSubsystem* GetOSS()              const;
	IOnlineIdentityPtr  GetIdentityInterface() const;

	FDelegateHandle LoginDelegateHandle;
	FString         CachedDisplayName;
};