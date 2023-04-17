// Fill out your copyright notice in the Description page of Project Settings.


#include "CaptureTheWingGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Kismet/GamePlayStatics.h"
#include "Blaster/Pickups/ArtifactSpawnPoint.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Pickups/Artifact.h"
#include "Blaster/BlasterComponents/CombatComponent.h"

void ACaptureTheWingGameMode::ArtifactCaptured(ABlasterCharacter* BlasterCharacter)
{
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BlasterGameState && BlasterCharacter)
	{
		if (BlasterCharacter->GetPlayerTeam() == ETeam::ET_BlueTeam) BlasterGameState->AddToBlueTeamScore(true);	// true means wing is equipped - score 10 points
		if (BlasterCharacter->GetPlayerTeam() == ETeam::ET_RedTeam) BlasterGameState->AddToRedTeamScore(true);

		if (BlasterCharacter->IsHoldingArtifact())
		{
			// Destroy the artifact

			BlasterCharacter->GetArtifact()->Destroy();

			//  spawn another

			TArray<AActor*> ArtifactSpawnPoints;
			UGameplayStatics::GetAllActorsOfClass(this, AArtifactSpawnPoint::StaticClass(), ArtifactSpawnPoints);

			int32 RandomSpawnPoint = FMath::RandRange(0, ArtifactSpawnPoints.Num() - 1);
			AArtifactSpawnPoint* SpawnPoint = Cast<AArtifactSpawnPoint>(ArtifactSpawnPoints[RandomSpawnPoint]);
			if (SpawnPoint) SpawnPoint->SpawnArtifact();
		}
	}
}
