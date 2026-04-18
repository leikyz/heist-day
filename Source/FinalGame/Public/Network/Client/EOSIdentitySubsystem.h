#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "EOSIdentitySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoginStateChanged, bool, bIsSuccessful);

UCLASS()
class FINALGAME_API UEOSIdentitySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "EOS|Identity")
	void LoginWithDevAuth();

	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	FString GetPlayerName() const;

	UFUNCTION(BlueprintPure, Category = "EOS|Identity")
	bool IsLoggedIn() const { return bIsLoggedIn; }

	UPROPERTY(BlueprintAssignable, Category = "EOS|Identity")
	FOnLoginStateChanged OnLoginStateChanged;

private:
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	bool bIsLoggedIn = false;
	TSharedPtr<const FUniqueNetId> LocalAccountId;
};