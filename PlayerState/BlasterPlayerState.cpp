// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerState.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/HUD/BlasterHUD.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, PlayerDefeats);
	DOREPLIFETIME(ABlasterPlayerState, Team);
}
/*
*
* Player Score
*
*/
void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	//once the score changes, we need to update the HUD.
	UpdateHUDScore();
}

void ABlasterPlayerState::OnRep_Score()
{
	//once the score changes,inform all clients.
	UpdateHUDScore();
}

void ABlasterPlayerState::UpdateHUDScore()
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->Controller) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDScore(GetScore());
		}
	}
}
/*
*
* Player Defeats
*
*/
void ABlasterPlayerState::AddToDefeats(int32 Defeat)
{
	PlayerDefeats += Defeat;
	UpdateHUDDefeats();
}

void ABlasterPlayerState::OnRep_PlayerDefeats()
{
	UpdateHUDDefeats();
}

void ABlasterPlayerState::UpdateHUDDefeats()
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->Controller) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDDefeats(PlayerDefeats);
		}
	}
}

/**
* Chat Widget
*/

void ABlasterPlayerState::MulticastShowChatMessage_Implementation(const FText& Text)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		ABlasterHUD* HUD = Cast<ABlasterHUD>(It->Get()->GetHUD());
		if (HUD)
		{
			HUD->AddChatMessage(GetPlayerName(), Text.ToString());
		}
	}
}