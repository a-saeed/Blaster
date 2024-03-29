// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Blaster/BlasterTypes/Team.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/*
	*
	* Player Score
	*
	*/
	//this will only be called on the server, we ensure that by calling it only in the game mode.
	void AddToScore(float ScoreAmount);

	//once the score changes, replicate it to each relevant client.
	virtual void OnRep_Score() override;
	/*
	*
	* Player Defeats
	*
	*/
	void AddToDefeats(int32 Defeat);

	/**
	* Chat Widget
	*/

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShowChatMessage(const FText& Text);	// called from Player controller

private:

	UPROPERTY()
	class ABlasterCharacter* BlasterCharacter;

	UPROPERTY()
	class ABlasterPlayerController* BlasterController;
	/*
	*
	* Player Score
	*
	*/
	void UpdateHUDScore();
	/*
	* 
	* Player Defeats
	* 
	*/
	UPROPERTY(ReplicatedUsing = OnRep_PlayerDefeats)
		int32 PlayerDefeats = 0.f;

	UFUNCTION()
		void OnRep_PlayerDefeats();

	void UpdateHUDDefeats();

	/**
	* Player Team
	*/

	UPROPERTY(Replicated)
	ETeam Team = ETeam::ET_NoTeam;	// NoTeam unless we specify otherwise.

public:

	FORCEINLINE ETeam GetTeam() const { return Team; }
	FORCEINLINE void SetTeam(ETeam TeamToSet) { Team = TeamToSet; }
};
