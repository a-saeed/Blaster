// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "BlasterPlayerController.generated.h"
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/*
	* Blaster Character HUD
	*/
	void SetHUDHealth(float Health, float MaxHealth); //called in blaster character
	/*
	* Player state HUD
	*/
	void SetHUDScore(float Score);

	void SetHUDDefeats(int32 Defeats);
	/*
	* Ammo HUD
	*/
	void SetHUDWeaponAmmo(int32 WeaponAmmoAmount);
	void SetHUDCarriedAmmo(int32 CarriedAmmoAmount);
	/*
	* Weapon Type HUD
	*/
	void SetHUDWeaponType(FText WeaponTypeText);
	/*
	* MatchCountdownText
	*/
	void SetHUDMatchCountdown(float CountdownTime);
	/*
	* Announcement (WarmupCountdown - Text - InfoText)
	*/
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDAnnouncementText(FString AnnouncementText);
	void SetHUDAnnouncementInfoText(FString InfoText);
	/*
	* Sync time between client and server
	*/
	virtual void ReceivedPlayer() override; //sync with server clock as soon as possible;
	/*
	* Handle different behavior for match states
	*/
	void OnMatchStateSet(FName State);
	/*
	* Play Sniper scope animation
	*/
	void SetHUDSniperScope(bool bIsAiming);

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void OnPossess(APawn* InPawn) override;
	/*
	* Sync time between client and server
	*/
	void SetHUDTime();									//show amount of match time left 

	UFUNCTION(Server, Reliable)							//Request the current server time, passing in the client's time when the request was sent.
		void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)							//Reports the current server time to the client in response to ServerRequestServerTime..
		void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f;						//difference between client and server time.

	virtual float GetServerTime();						//synced with server world clock; NOT OVERRIDEN

	UPROPERTY(EditAnywhere, Category = "Time")
		float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);
	/*
	* Poll for character overlay
	*/
	void PollInit();
	/*
	* Handle match states
	*/
	void HandleMatchHasStarted();
	void HandleCooldown();

	UFUNCTION(Server, Reliable)							//needed for players who want to join midgame. Change HUD based on match state.
		void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)							//send match state and timers information from server to all clients once when they join. 														
		void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartTime);

private:

	UPROPERTY()
	class ABlasterHUD* BlasterHUD;
	
	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	/*Filled from BlasterGameMode*/
	float LevelStartingTime = 0.f;							//at which time did the Blaster map got loaded
	float MatchTime = 0.f;									//total match time.
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;

	uint32 CountdownInt = 0;								//used to update the HUD every second

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)			//match state is used in player controller since it's responsible for showing the HUD, we want to delay the overlay widget until match has begun.
	FName MatchState;

	UFUNCTION()
		void OnRep_MatchState();
	/*
	* Character overlay and "cached" variables
	*/
	UPROPERTY()
		class UCharacterOverlay* CharacterOverlay;

	bool bInitializeCharacterOverlay = false;				//used later

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
	/*
	* Timer Blink / Sound
	*/
	FTimerHandle BlinkTimer;
	void BlinkTimerFinished();

	bool bAlarmplayed = false;

	UPROPERTY(EditAnywhere, Category = "Timer")
		class USoundCue* TickSound;

	void TimeRunningOut();

public:

};
