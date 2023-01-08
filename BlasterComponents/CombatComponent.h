#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/Weapon/Weapontypes.h"
#include "CombatComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UCombatComponent();

	friend class ABlasterCharacter;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

	void DropWeapon();

	void EquipWeapon(class AWeapon* WeaponToEquip);

	void SetAiming(bool bIsAiming);

	void FireButtonPressed(bool bPressed);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	/*
	*	RPCs 
	*/
	UFUNCTION(Server, Reliable) //aiming from a client machine doesn't replicate on other clients & server. create an RPC
		void ServerSetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
		void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
		void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	/*
	*	REP_NOTIFIES
	*/
	UFUNCTION()
		void OnRep_EquippedWeapon();

private:

	/*
	*	BODY
	*/
	UPROPERTY()
	class ABlasterCharacter* Character;
	UPROPERTY()
	class ABlasterPlayerController* BlasterController;
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
		class AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
		bool bAiming;

	bool bFireButtonPressed;

	/*
	*	CHARACTER CONTROLS
	*/
	UPROPERTY(EditAnywhere)
		float BaseWalkSpeed = 750;

	UPROPERTY(EditAnywhere)
		float AimWalkSpeed = 300;

	/*
	*	CROSSHAIRS AND HUD
	*/
	float CrosshairsVelocityFactor;
	float CrosshairsInAirFactor;
	float CrosshairsAimFactor;
	float CrosshairsShootingFactor;
	float CrosshairsOnTargetFactor;
	FHUDPackage HUDPackage;
	void SetCrosshairsColor(FHitResult& TraceHitResult);
	void SetCrosshairsSpread(float DeltaTime);

	FVector HitTarget; //need in the anime instance to fix the weapon rotation

	/*
	* AIMING AND FOV
	*/
	//field of view when not aiming; Set to the camera's base FOV in BeginPlay
	float DefaultFOV;

	float CurrentFOV;

	UPROPERTY(EditAnywhere , Category = "Combat")
		float ZoomedFOV = 30.f; //not used till now

	UPROPERTY(EditAnywhere, Category = "Combat")
		float ZoomInterpSpeed = 20.f; //used to make all weapons unzoom at the same rate.

	void InterpFOV(float DeltaTime);
	/*
	* AUTOMATIC FIRE
	*/
	FTimerHandle FireTimer;
	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

	UPROPERTY(EditAnywhere, Category = "Combat")
		class USoundCue* EmptySound;
	/*
	* Carried Ammo
	*/
	 /*Carried ammo for the currently equipped weapon*/
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
		int32 CarriedAmmo;

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UFUNCTION()
		void OnRep_CarriedAmmo();

public:	
};
