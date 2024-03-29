// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/GameMode/TeamsGameMode.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/HUD/Announcement.h"
#include "Blaster/HUD/SniperScope.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Sound/SoundCue.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/Image.h"
#include "Blaster/HUD/ReturnToMainMenu.h"
#include "Blaster/HUD/ChatWidget.h"
#include "Components/EditableText.h"
#include "Blaster/BlasterTypes/EndgameAnnouncement.h"

/**
*  Controller input key bindings
*/

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent) return;

	InputComponent->BindAction("Quit", IE_Pressed, this, &ABlasterPlayerController::ShowReturnToMainMenu);
	InputComponent->BindAction("Chat", IE_Pressed, this, &ABlasterPlayerController::ShowChatWidget);
}

void ABlasterPlayerController::ShowReturnToMainMenu()
{
	if (!ReturnToMainMenuClass) return;

	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuClass);
	}
	
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;

		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

void ABlasterPlayerController::ShowChatWidget()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		bChatTextFocus = !bChatTextFocus;
		BlasterHUD->AddChatWidget(bChatTextFocus);
	}
}

/*
*  Overriden functions
*/
void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
	DOREPLIFETIME(ABlasterPlayerController, bTeamsMatch);
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());			//set the BlasterHUD

	if (Cast<ATeamsGameMode>(GetWorld()->GetAuthGameMode())) bTeamsMatch = true;

	ServerCheckMatchState();							//once a client joins, set its match state and timers
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetHUDTime();

	CheckTimeSync(DeltaTime);

	PollInit();

	CheckPing(DeltaTime);
}

void ABlasterPlayerController::CheckPing(float DeltaTime)
{
	if (HasAuthority()) return;

	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		HighPingRunningTime = 0.f;	

		PlayerState = PlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : PlayerState;
		if (PlayerState)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerState->GetPing() * 4: %d"), PlayerState->GetPing() * 4)
			if (PlayerState->GetPing() * 4 > HighPingThreshold)		//Ping is compressed; it's actually / 4
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}

		bool bHighPingAnimationPlaying =
			BlasterHUD &&
			BlasterHUD->GetCharacterOverlay() &&
			BlasterHUD->GetCharacterOverlay()->GetWifiAnimation() &&
			BlasterHUD->GetCharacterOverlay()->IsAnimationPlaying(BlasterHUD->GetCharacterOverlay()->GetWifiAnimation());

		if (bHighPingAnimationPlaying)
		{
			PingAnimationRunningTime += DeltaTime;

			if (PingAnimationRunningTime > HighPingDuration)
			{
				StopHighPingWarning();
			}
		}
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	/*on possessing the character, set the hud health*/
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
		
		if (BlasterCharacter->GetCombatComponent() && BlasterCharacter->GetCombatComponent()->GetEquippedWeapon())
		{
			SetHUDCarriedAmmo(BlasterCharacter->GetCombatComponent()->GetCarriedAmmo());
			SetHUDWeaponAmmo(BlasterCharacter->GetCombatComponent()->GetEquippedWeapon()->GetCurrentAmmo());
		}
		
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
				if(bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if(bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if(bInitializeScore) SetHUDScore(HUDScore);
				if(bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if(bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
				if(bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if(bInitializeWeaponType) SetHUDWeaponType(HUDWeaponTypeText);

				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombatComponent())
				{
					if (bInitializeGrenades) SetHUDGrenades(BlasterCharacter->GetCombatComponent()->GetGrenades());
				}	
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
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if(MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt32(TimeLeft);

	if (HasAuthority())								//if on the server, get the countdown time directly from the game mode
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(GetWorld())) : BlasterGameMode;
		if (BlasterGameMode)
		{
			LevelStartingTime = BlasterGameMode->LevelStartingTime;
			SecondsLeft = FMath::CeilToInt32(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)				//only update the HUD after 1 second has passed.
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
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
	SingleTripTime = (0.5f * RoundTripTime);
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;

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
* Handle different behavior for match states, to be called from the game mode every time the match state changes..
*/
void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;									//the state passed from the game mode.

	if (MatchState == MatchState::InProgress)			//MatchState::InProgress is in the game mode namespace
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)			//MatchState::InProgress is in the game mode namespace
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		if(BlasterHUD->GetCharacterOverlay() == nullptr) BlasterHUD->AddCharacterOverlay();	   //Don't add character overlay twice

		if (BlasterHUD->GetAnnouncement()) BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);  //hide announcemenet widget when game starts.

		if (bTeamsMatch)
		{
			InitTeamScores();
			HidePlayerScoreDefeats();
		}
		else
		{
			InitPlayerScoreDefeats();
			HideTeamScores();
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->GetCharacterOverlay()->RemoveFromParent();

		if (BlasterHUD->GetAnnouncement())
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Visible);

			FString AnnouncementText = EndgameAnnouncement::NewMatchStartsIn;
			SetHUDAnnouncementText(AnnouncementText);

			/*Show the top scoring players in the Announ. widget*/
			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(GetWorld()));

			if (BlasterGameState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				FString CooldownInfoTextString = bTeamsMatch ? GetCooldownTeamsInfoText(BlasterGameState) : GetCooldownInfoText(TopPlayers);

				SetHUDAnnouncementInfoText(CooldownInfoTextString);
			}
		}
	}

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->GetCombatComponent())
	{
		BlasterCharacter->bDisableGameplay = true;
		BlasterCharacter->GetCombatComponent()->FireButtonPressed(false);						//stop if firing once cooldown state is reached.
	}
}

FString ABlasterPlayerController::GetCooldownInfoText(TArray<ABlasterPlayerState*>& TopPlayers)
{
	ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
	if (!BlasterPlayerState) return FString();

	FString InfoTextString;

	if (TopPlayers.Num() == 0)
	{
		InfoTextString = EndgameAnnouncement::ThereIsNoWinner;
	}
	else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
	{
		InfoTextString = EndgameAnnouncement::YouAreTheWinner;
	}
	else if (TopPlayers.Num() == 1)
	{
		InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
	}
	else if (TopPlayers.Num() > 1)
	{
		InfoTextString = EndgameAnnouncement::PlayersTiedForTheWin;
		InfoTextString.Append(FString("\n"));

		for (ABlasterPlayerState* TiedPlayer : TopPlayers)
		{
			InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
		}
	}

	return InfoTextString;
}

FString ABlasterPlayerController::GetCooldownTeamsInfoText(ABlasterGameState* GameState)
{
	if (!GameState) return FString();

	FString InfoText;
	int32 RedTeamScore = GameState->RedTeamScore;
	int32 BlueTeamScore = GameState->BlueTeamScore;

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoText = EndgameAnnouncement::ThereIsNoWinner;
	}

	else if (RedTeamScore == BlueTeamScore)
	{
		InfoText = EndgameAnnouncement::TeamsTiedForTheWin;
	}

	else if (RedTeamScore > BlueTeamScore)
	{
		InfoText = EndgameAnnouncement::RedTeamWins;
		InfoText.Append(TEXT("\n\n"));
		InfoText.Append(FString::Printf(TEXT("%s:\t%d \t\t\t %s:\t%d\n"), *EndgameAnnouncement::Reds, RedTeamScore, *EndgameAnnouncement::Blues, BlueTeamScore));
	}

	else if (BlueTeamScore > RedTeamScore)
	{
		InfoText = EndgameAnnouncement::BlueTeamWins;
		InfoText.Append(TEXT("\n\n"));
		InfoText.Append(FString::Printf(TEXT("%s:\t%d \t\t\t %s:\t%d\n"), *EndgameAnnouncement::Blues, BlueTeamScore, *EndgameAnnouncement::Reds, RedTeamScore));
	}

	return InfoText;
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	/* If a player wants to join midgame, based on the current match state, access game mode to fill in the values of Match time and warmup time.*/
	BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (BlasterGameMode)
	{
		MatchTime = BlasterGameMode->MatchTime;
		WarmupTime = BlasterGameMode->WarmupTime;
		CooldownTime = BlasterGameMode->CooldownTime;
		LevelStartingTime = BlasterGameMode->LevelStartingTime;

		MatchState = BlasterGameMode->GetMatchState();								//triggers OnRep_MatchState()

		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);	//only the client that invoked the server rpc will execute this fn.
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartTime)
{
	MatchTime = Match;
	WarmupTime = Warmup;
	CooldownTime = Cooldown;
	LevelStartingTime = StartTime;

	MatchState = StateOfMatch;

	OnMatchStateSet(MatchState);									//in case match state got replicated before this rpc could change the values of its HUD. 

	if (BlasterHUD && MatchState == MatchState::WaitingToStart)			//show announcement only if the game hasn't started yet
	{
		BlasterHUD->AddAnnouncement();
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
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	if (!BlasterHUD)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetShieldBar();

	if (bHUDValid)
	{
		const float ShieldPercentage = Shield / MaxShield;
		BlasterHUD->GetCharacterOverlay()->GetShieldBar()->SetPercent(ShieldPercentage);
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::HidePlayerScoreDefeats()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetScoreText() &&
		BlasterHUD->GetCharacterOverlay()->GetDefeatsText();

	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->GetScoreText()->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->GetDefeatsText()->SetText(FText());
	}
}

void ABlasterPlayerController::InitPlayerScoreDefeats()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetScoreText() &&
		BlasterHUD->GetCharacterOverlay()->GetDefeatsText() &&
		BlasterHUD->GetCharacterOverlay()->GetScoreAmount() &&
	    BlasterHUD->GetCharacterOverlay()->GetDefeatsAmount();

	if (bHUDValid)
	{
		FString ScoreText("Score:");
		FString DefeatsText("Defeats:");
		FString Zero("0");

		BlasterHUD->GetCharacterOverlay()->GetScoreText()->SetText(FText::FromString(ScoreText));
		BlasterHUD->GetCharacterOverlay()->GetDefeatsText()->SetText(FText::FromString(DefeatsText));
		BlasterHUD->GetCharacterOverlay()->GetScoreAmount()->SetText(FText::FromString(Zero));
		BlasterHUD->GetCharacterOverlay()->GetDefeatsAmount()->SetText(FText::FromString(Zero));
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetScoreAmount();

	if (bHUDValid)
	{
		/*set Score text*/
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Score));
		BlasterHUD->GetCharacterOverlay()->GetScoreAmount()->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetDefeatsAmount();

	if (bHUDValid)
	{
		/*set Defeats text*/
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->GetCharacterOverlay()->GetDefeatsAmount()->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
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
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = WeaponAmmoAmount;
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
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = CarriedAmmoAmount;
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
	else
	{
		bInitializeWeaponType = true;
		HUDWeaponTypeText = WeaponTypeText;
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
		if (CountdownTime < 0.f)
		{
			BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()->SetText(FText());			//Dispaly Empty text.. prevents setting HUD with -ve value momentarily
			return;
		}

		if (CountdownTime <= 30.f)
		{
			/* -play clock tick sound .. -Turn timer red .. -Blink timer*/
			TimeRunningOut();
		}
		/*set MatchCountdown text in 00:00 format*/
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString MatchCountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()->SetText(FText::FromString(MatchCountdownText));
	}
}

void ABlasterPlayerController::TimeRunningOut()
{
	if (!bAlarmplayed && TickSound)
	{
		FVector Location = FVector(0, 0, 0);
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), TickSound, Location);
		bAlarmplayed = true;
	}

	BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
	BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()->SetVisibility(ESlateVisibility::Hidden);

	GetWorldTimerManager().SetTimer(BlinkTimer, this, &ABlasterPlayerController::BlinkTimerFinished, 0.25f);
}

void ABlasterPlayerController::BlinkTimerFinished()
{
	BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()->SetVisibility(ESlateVisibility::Visible);
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetAnnouncement() &&
		BlasterHUD->GetAnnouncement()->GetWarmupText();

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->GetAnnouncement()->GetWarmupText()->SetText(FText());						//Dispaly Empty text.. prevents setting HUD with -ve value momentarily
			return;
		}

		/*set MatchCountdown text in 00:00 format*/
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString WarmupCountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->GetAnnouncement()->GetWarmupText()->SetText(FText::FromString(WarmupCountdownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementText(FString AnnouncementText)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetAnnouncement() &&
		BlasterHUD->GetAnnouncement()->GetAnnouncementText();

	if (bHUDValid)
	{
		BlasterHUD->GetAnnouncement()->GetAnnouncementText()->SetText(FText::FromString(AnnouncementText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementInfoText(FString InfoText)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetAnnouncement() &&
		BlasterHUD->GetAnnouncement()->GetInfoText();

	if (bHUDValid)
	{
		BlasterHUD->GetAnnouncement()->GetInfoText()->SetText(FText::FromString(InfoText));
	}
}

void ABlasterPlayerController::SetHUDSniperScope(bool bIsAiming)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetSniperScope() &&
		BlasterHUD->GetSniperScope()->GetScopeAnimation();

	if (!BlasterHUD->GetSniperScope())
	{
		BlasterHUD->AddSniperScope();			//only create the widget if it doesn't exist
	}

	if (bHUDValid)
	{
		if (bIsAiming)
		{
			BlasterHUD->GetSniperScope()->PlayAnimation(BlasterHUD->GetSniperScope()->GetScopeAnimation());
		}
		else
		{
			BlasterHUD->GetSniperScope()->PlayAnimation(BlasterHUD->GetSniperScope()->GetScopeAnimation(), 0.f, 1, EUMGSequencePlayMode::Reverse);
		}
		
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetGrenadesText();

	if (bHUDValid)
	{
		/*set Defeats text*/
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->GetCharacterOverlay()->GetGrenadesText()->SetText(FText::FromString(GrenadesText));
	}
	else	//Character overlay hasn't got initialized yet.
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetWifiImage() &&
		BlasterHUD->GetCharacterOverlay()->GetWifiAnimation();

	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->GetWifiImage()->SetRenderOpacity(1.f);
		BlasterHUD->GetCharacterOverlay()->PlayAnimation(BlasterHUD->GetCharacterOverlay()->GetWifiAnimation(), 0.f, 5);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetWifiImage() &&
		BlasterHUD->GetCharacterOverlay()->GetWifiAnimation();

	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->GetWifiImage()->SetRenderOpacity(0.f);
		if (BlasterHUD->GetCharacterOverlay()->IsAnimationPlaying(BlasterHUD->GetCharacterOverlay()->GetWifiAnimation()))
		{
			BlasterHUD->GetCharacterOverlay()->StopAnimation(BlasterHUD->GetCharacterOverlay()->GetWifiAnimation());
		}
	}
}

void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

/**
* Team Score
*/

void ABlasterPlayerController::OnRep_TeamsMatch()
{
	bTeamsMatch = true;
}

void ABlasterPlayerController::HideTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetBlueScoreText() && 
		BlasterHUD->GetCharacterOverlay()->GetRedScoreText() && 
		BlasterHUD->GetCharacterOverlay()->GetScoreSpacerText();

	if (bHUDValid)
	{
		BlasterHUD->GetCharacterOverlay()->GetBlueScoreText()->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->GetRedScoreText()->SetText(FText());
		BlasterHUD->GetCharacterOverlay()->GetScoreSpacerText()->SetText(FText());
	}
}

void ABlasterPlayerController::InitTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetBlueScoreText() &&
		BlasterHUD->GetCharacterOverlay()->GetRedScoreText() &&
		BlasterHUD->GetCharacterOverlay()->GetScoreSpacerText();

	if (bHUDValid)
	{
		FString Zero("0");
		FString Spacer("|");

		BlasterHUD->GetCharacterOverlay()->GetBlueScoreText()->SetText(FText::FromString(Zero));
		BlasterHUD->GetCharacterOverlay()->GetRedScoreText()->SetText(FText::FromString(Zero));
		BlasterHUD->GetCharacterOverlay()->GetScoreSpacerText()->SetText(FText::FromString(Spacer));
	}
}

void ABlasterPlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetBlueScoreText();

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), BlueScore);
		BlasterHUD->GetCharacterOverlay()->GetBlueScoreText()->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::SetHUDRedTeamScore(int32 RedScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->GetCharacterOverlay() &&
		BlasterHUD->GetCharacterOverlay()->GetRedScoreText();

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), RedScore);
		BlasterHUD->GetCharacterOverlay()->GetRedScoreText()->SetText(FText::FromString(ScoreText));
	}
}

/*
* Broadcast Eliminations
*/

void ABlasterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
}

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();

	if (Self && Attacker && Victim)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		if (BlasterHUD)
		{
			if (Attacker == Self && Victim != Self)
			{
				BlasterHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
			}

			else if (Victim == Self && Attacker != Self)
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "You");
			}

			else if (Attacker == Victim && Attacker == Self)
			{
				BlasterHUD->AddElimAnnouncement("You", "YourSelf");
			}

			else if (Attacker == Victim && Attacker != Self)
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "Himself");
			}

			else
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
			}
		}
	}
}

/*
* Broadcast Chat
*/

void ABlasterPlayerController::ServerShowChatMessage_Implementation(const FText& Text)
{
	ABlasterPlayerState* Self = Cast<ABlasterPlayerState>(GetPlayerState<APlayerState>());
	if (Self)
	{
		Self->MulticastShowChatMessage(Text);
	}
}