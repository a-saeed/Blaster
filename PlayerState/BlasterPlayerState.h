// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	//this will only be called on the server, we ensure that by calling it only in the game mode.
	void AddToScore(float ScoreAmount);

	//once the score changes, replicate it to each relevant client.
	virtual void OnRep_Score() override;

private:

	class ABlasterCharacter* BlasterCharacter;
	class ABlasterPlayerController* BlasterController;

	void UpdateHUDScore();
};
