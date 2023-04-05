// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterGameMode.h"
#include "TeamsGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ATeamsGameMode : public ABlasterGameMode
{
	GENERATED_BODY()
	
public:

	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimPlayerController, ABlasterPlayerController* AttackerPlayerController);

	// handle players joining mid-game
	virtual void PostLogin(APlayerController* NewPlayer) override;

	//handle players leaving
	virtual void Logout(AController* Exiting) override;

	// NUllify friendly fire
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;

protected:

	// match just begun
	virtual void HandleMatchHasStarted() override;
};
