// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState
{
	extern BLASTER_API const FName Cooldown; //Match duration has been reached, Display winner and begin cooldown timer.
}
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	ABlasterGameMode();

	virtual void Tick(float DeltaTime) override;

	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimPlayerController, ABlasterPlayerController* AttackerPlayerController);

	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

	void PlayerLeftGame(class ABlasterPlayerState* LeavingPlayerState);

	UPROPERTY(EditDefaultsOnly)
		float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
		float CooldownTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
		float MatchTime = 120.f;

	float LevelStartingTime = 0.f;

	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

protected:

	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

	AActor* FindPlayerStartWithLeastPlayersInrange();

	UPROPERTY(EditAnywhere)
		int32 PlayerStartRange = 2500;

private:

	float CountdownTime = 0.f;

public:

	FORCEINLINE float GetCountdownTime() const {return CountdownTime;}
};
