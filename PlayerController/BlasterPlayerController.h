// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "BlasterPlayerController.generated.h"

/**
 *	Delegate that a fn in the weapon class wil bind to to enable/disable SSR
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	/*
	* Create a delegate of type FhighPingDelegate; public so fns from other classes can bind to it.
	*/
	FHighPingDelegate HighPingDelegate;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/*
	* Blaster Character HUD
	*/
	void SetHUDHealth(float Health, float MaxHealth); //called in blaster character
	void SetHUDShield(float Shield, float MaxShield);
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
	/*
	* Grenades
	*/
	void SetHUDGrenades(int32 Grenades);
	/*
	* Server Time & Single trip time used for SSR
	*/
	virtual float GetServerTime();						//synced with server world clock; NOT OVERRIDEN
	float SingleTripTime = 0.f;

	/*
	* Broadcast Eliminations
	*/

	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);

	/*
	* Broadcast Chat Message
	*/

	UFUNCTION(Server, Reliable)
	void ServerShowChatMessage(const FText& Text);

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void OnPossess(APawn* InPawn) override;

	/**
	* Bind inputs in player controller
	*/

	virtual void SetupInputComponent() override;

	// return menu

	void ShowReturnToMainMenu();

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<class UUserWidget> ReturnToMainMenuClass;

	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;

	bool bReturnToMainMenuOpen = false;

	// Chat Box

	void ShowChatWidget();

	bool bChatTextFocus = false;

	/*
	* Sync time between client and server
	*/
	void SetHUDTime();									//show amount of match time left 

	UFUNCTION(Server, Reliable)							//Request the current server time, passing in the client's time when the request was sent.
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)							//Reports the current server time to the client in response to ServerRequestServerTime..
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f;						//difference between client and server time.

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

	/*
	* Broadcast Eliminations
	*/

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

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

	/*We cache variabled that need to be set in BeginPlay() of other classes.. Character overlay isn't valid yet*/
	float HUDHealth;
	float HUDMaxHealth;
	bool bInitializeHealth = false;
	float HUDShield;
	float HUDMaxShield;
	bool bInitializeShield = false;
	float HUDScore;
	bool bInitializeScore = false;
	int32 HUDDefeats;
	bool bInitializeDefeats = false;
	int32 HUDGrenades;
	bool bInitializeGrenades = false;
	int32 HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;
	int32 HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;
	/*
	* Timer Blink / Sound
	*/
	FTimerHandle BlinkTimer;
	void BlinkTimerFinished();

	bool bAlarmplayed = false;

	UPROPERTY(EditAnywhere, Category = "Timer")
	class USoundCue* TickSound;

	void TimeRunningOut();

	/*
	* High Ping
	*/
	void CheckPing(float DeltaTime);

	void HighPingWarning();
	void StopHighPingWarning();

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);	//used broadcast a delegate

	float HighPingRunningTime = 0.f;

	float PingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;

public:

	FORCEINLINE void SetChatTextFocus(bool bFocus) { bChatTextFocus = bFocus; }
};
