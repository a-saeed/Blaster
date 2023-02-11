// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/HUD/Announcement.h"
/*
*  Overriden functions
*/
void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());			//set the BlasterHUD

	if (BlasterHUD)
	{
		BlasterHUD->AddAnnouncement();					//HUD isn't valid yet in WaitingToStart match state. need to be in begin play.
	}
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetHUDTime();

	CheckTimeSync(DeltaTime);

	PollInit();
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	/*on possessing the character, set the hud health*/
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}
/*
* Poll for character overlay, since health, score and defeats values are initialized BEFORE the overlay is valid. which shouldn't be true.
*/
void ABlasterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->GetCharacterOverlay())
		{
			CharacterOverlay = BlasterHUD->GetCharacterOverlay();
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}
/*
*  HUD Timers
*/
void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;

	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	uint32 SecondsLeft = FMath::CeilToInt32(MatchTime - GetServerTime());

	if (CountdownInt != SecondsLeft)				//only update the HUD after 1 second has passed.
	{
		SetHUDMatchCountdown(MatchTime - GetServerTime());
	}

	CountdownInt = SecondsLeft;
}
/*
*  Sync time between client and server
*/
void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);

	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}
	else
	{
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
	}
}
/*
* Handle different behavior for match states, to be called from the game mode.
*/
void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;									//the state passed from the game mode.

	if (MatchState == MatchState::InProgress)			//MatchState::InProgress is in the game mode namespace
	{
		HandleMatchHasStarted();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress) //MatchState::InProgress is in the game mode namespace
	{
		HandleMatchHasStarted();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	/* -Check for blasterHUD -Add character overlay -Hide Announcement */
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->AddCharacterOverlay();

		if (BlasterHUD->GetAnnouncement())
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);				//hide announcemenet widget when game starts.
		}
	}
}
/*
*  HUDs used in other classes
*/
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
		/*calc and set health bar*/
		const float HealthPercentage = Health / MaxHealth;																
		BlasterHUD->GetCharacterOverlay()->GetHealthBar()->SetPercent(HealthPercentage);	
		/*calc and set health text*/
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->GetCharacterOverlay()->GetHealthText()->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
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
		/*set Score text*/
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Score));
		BlasterHUD->GetCharacterOverlay()->GetScoreText()->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
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
		/*set Defeats text*/
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->GetCharacterOverlay()->GetDefeatsText()->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
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
		/*set Defeats text*/
		FString AmmoText = FString::Printf(TEXT("%d"), WeaponAmmoAmount);
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
		/*set Carried Ammo text*/
		FString CarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmoAmount);
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
		/*set MatchCountdown text in 00:00 format*/
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString MatchCountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()->SetText(FText::FromString(MatchCountdownText));
	}
}