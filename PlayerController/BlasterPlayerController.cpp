// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blaster/Character/BlasterCharacter.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	//set the BlasterHUD
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetHUDTime();
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	//on possessing the character, set the hud health
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	uint32 SecondsLeft = FMath::CeilToInt32(MatchTime - GetWorld()->GetTimeSeconds());

	if (CountdownInt != SecondsLeft) //only update the HUD after 1 second has passed.
	{
		SetHUDMatchCountdown(MatchTime - GetWorld()->GetTimeSeconds());
	}

	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	if (!BlasterHUD)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetHealthBar() &&
		BlasterHUD->GetCharacterOverlay()->GetHealthText();

	if (bHUDValid)
	{
		//calc and set health bar
		const float HealthPercentage = Health / MaxHealth;
		BlasterHUD->GetCharacterOverlay()->GetHealthBar()->SetPercent(HealthPercentage);
		//calc and set health text
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth)); //generate a formatted string
		BlasterHUD->GetCharacterOverlay()->GetHealthText()->SetText(FText::FromString(HealthText));
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetScoreText();

	if (bHUDValid)
	{
		//set Score text
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Score)); //generate a formatted string
		BlasterHUD->GetCharacterOverlay()->GetScoreText()->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetDefeatsText();

	if (bHUDValid)
	{
		//set Defeats text
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats); //generate a formatted string
		BlasterHUD->GetCharacterOverlay()->GetDefeatsText()->SetText(FText::FromString(DefeatsText));
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 WeaponAmmoAmount)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetAmmoText();

	if (bHUDValid)
	{
		//set Defeats text
		FString AmmoText = FString::Printf(TEXT("%d"), WeaponAmmoAmount); //generate a formatted string
		BlasterHUD->GetCharacterOverlay()->GetAmmoText()->SetText(FText::FromString(AmmoText));
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmoAmount)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetCarriedAmmoText();

	if (bHUDValid)
	{
		//set Defeats text
		FString CarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmoAmount); //generate a formatted string
		BlasterHUD->GetCharacterOverlay()->GetCarriedAmmoText()->SetText(FText::FromString(CarriedAmmoText));
	}
}

void ABlasterPlayerController::SetHUDWeaponType(FText WeaponTypeText)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetWeaponTypeText();

	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->GetWeaponTypeText()->SetText(WeaponTypeText);
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText();

	if (bHUDValid)
	{
		//set MatchCountdown text in 00:00 format
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString MatchCountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds); //generate a formatted string
		BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()->SetText(FText::FromString(MatchCountdownText));
	}
}