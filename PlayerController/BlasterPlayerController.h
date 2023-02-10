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
	* Sync time between client and server
	*/
	virtual void ReceivedPlayer() override; //sync with server clock as soon as possible;

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void OnPossess(APawn* InPawn) override;

	//show amount of match time left 
	void SetHUDTime();
	/*
	* Sync time between client and server
	*/
	//Request the current server time, passing in the client's time when the request was sent.
	UFUNCTION(Server, Reliable)
		void ServerRequestServerTime(float TimeOfClientRequest);

	//Reports the current server time to the client in response to ServerRequestServerTime..
	UFUNCTION(Client, Reliable)
		void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f; //difference between client and server time.

	virtual float GetServerTime(); //synced with server world clock; NOT OVERRIDEN

	UPROPERTY(EditAnywhere, Category = "Time")
		float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);

private:

	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	//total match time, will be moved to game mode class later.
	float MatchTime = 120.f; 

	//used to update the HUD every second
	uint32 CountdownInt = 0;

public:

};
