// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()
	
public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdateTopScore(class ABlasterPlayerState* ScoringPlayer);

	UPROPERTY(Replicated)												//so all clients know who the top scoring players are and show them in the announcement widget
	TArray <ABlasterPlayerState*> TopScoringPlayers;

	/**
	* Teams
	*/

	//Blue

	TArray <ABlasterPlayerState*> BlueTeam;

	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.f;

	void AddToBlueTeamScore(bool bWing);

	UFUNCTION()
	void OnRep_BlueTeamScore();

	//Red

	TArray <ABlasterPlayerState*> RedTeam;

	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.f;

	void AddToRedTeamScore(bool bWing);

	UFUNCTION()
	void OnRep_RedTeamScore();

	/**
	* Broadcast Artifact status
	*/

	UFUNCTION(NetMultiCast, Reliable)
	void MultiCastBroadcastStatus(class ABlasterCharacter* BlasterCharacter, EArtifactState ArtifactState);

private:
	
	float TopScore = 0.f;
};
