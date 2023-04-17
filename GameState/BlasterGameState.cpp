// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameState.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterTypes/ArtifactState.h"
#include "Styling/SlateColor.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);
	DOREPLIFETIME(ABlasterGameState, BlueTeamScore);
	DOREPLIFETIME(ABlasterGameState, RedTeamScore);
}

void ABlasterGameState::UpdateTopScore(ABlasterPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if(ScoringPlayer->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void ABlasterGameState::AddToBlueTeamScore(bool bWing)
{
	if (bWing) BlueTeamScore += 10;
	else ++BlueTeamScore;
	
	ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PlayerController)
	{
		PlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABlasterGameState::OnRep_BlueTeamScore()
{
	ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PlayerController)
	{
		PlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABlasterGameState::AddToRedTeamScore(bool bWing)
{
	if (bWing) RedTeamScore += 10;
	else ++RedTeamScore;
	
	ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PlayerController)
	{
		PlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::OnRep_RedTeamScore()
{
	ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PlayerController)
	{
		PlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::MultiCastBroadcastStatus_Implementation(class ABlasterCharacter* BlasterCharacter, EArtifactState ArtifactState)
{
	if (!BlasterCharacter) return;

	FString ArtifactStatus;
	FSlateColor Color;

	switch (ArtifactState)
	{
	case EArtifactState::EAS_Equipped:

		if (BlasterCharacter->GetPlayerTeam() == ETeam::ET_RedTeam)
		{
			ArtifactStatus = "Red Team Has The Wings..";
			Color = FSlateColor(FLinearColor::Red);
		}
		else if (BlasterCharacter->GetPlayerTeam() == ETeam::ET_BlueTeam)
		{
			ArtifactStatus = "Blue Team Has The Wings..";
			Color = FSlateColor(FLinearColor::Blue);
		}
		break;

	case EArtifactState::EAS_Dropped:

		if (BlasterCharacter->GetPlayerTeam() == ETeam::ET_RedTeam)
		{
			ArtifactStatus = "Red Team Dropped The Wings..";
			Color = FSlateColor(FLinearColor::Red);
		}
		else if (BlasterCharacter->GetPlayerTeam() == ETeam::ET_BlueTeam)
		{
			ArtifactStatus = "Blue Team Dropped The Wings..";
			Color = FSlateColor(FLinearColor::Blue);
		}
		break;

	case EArtifactState::EAS_Captured:

		if (BlasterCharacter->GetPlayerTeam() == ETeam::ET_RedTeam)
		{
			ArtifactStatus = "Red Team Captured The Wings..";
			Color = FSlateColor(FLinearColor::Red);
		}
		else if (BlasterCharacter->GetPlayerTeam() == ETeam::ET_BlueTeam)
		{
			ArtifactStatus = "Blue Team Captured The Wings..";
			Color = FSlateColor(FLinearColor::Blue);
		}
		break;
	}


	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		UE_LOG(LogTemp, Warning, TEXT("Checking HUD"));
		ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayerController)
		{
			ABlasterHUD* BlasterHUD = Cast<ABlasterHUD>(BlasterPlayerController->GetHUD());
			if (BlasterHUD)
			{
				UE_LOG(LogTemp, Warning, TEXT("HUD Valid"));
				BlasterHUD->AddArtifactStatus(ArtifactStatus, Color);
			}
		}
	}
}
