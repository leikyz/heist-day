#pragma once

#include "CoreMinimal.h"
#include "Network/EOSNetworkSubsystem.h"
#include "EOSIdentitySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoginStateChangedDynamic, bool, bLoggedIn);

UCLASS()
class FINALGAME_API UEOSIdentitySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable)
	void LoginWithDevAuth();

	UFUNCTION(BlueprintPure)
	FString GetPlayerName() const;

	UFUNCTION(BlueprintCallable)
	bool IsLoggedIn() const { return bIsLoggedIn; }

	UE::Online::FAccountId GetLocalAccountId() const { return LocalAccountId; }

	UPROPERTY(BlueprintAssignable)
	FOnLoginStateChangedDynamic OnLoginStateChanged;

private:
	void OnLoginComplete(const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result);

	bool bIsLoggedIn = false;

	UE::Online::FAccountId LocalAccountId;
};