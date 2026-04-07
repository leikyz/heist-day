#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Online/OnlineServices.h"
#include "Online/Sessions.h"
#include "EOSServerSubsystem.generated.h"

UCLASS()
class FINALGAME_API UEOSServerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void CreateServerSession();

private:
	void OnSessionCreated(const UE::Online::TOnlineResult<UE::Online::FCreateSession>& Result);
};