#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UCombatComponent();

	friend class ABlasterCharacter;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*
	* Fire
	*/

	void FireButtonPressed(bool bPressed);						//to be used in player controller to disable firing in cooldown state.

	/*
	* Equip Weapon
	*/

	void EquipWeapon(class AWeapon* WeaponToEquip);

	/*
	* Aiming
	*/

	void SetAiming(bool bIsAiming);

	/*
	* Reload
	*/

	void Reload();

	void JumpToShotgunEnd();	//needed in weapon class

	/*
	* Grenades
	*/

	void ThrowGrenade();

	/*
	* Pickup Carried Ammo From Spawn Points
	*/

	bool PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

	/*
	* Swap Weapons
	*/
	void SwapWeapons();
	bool ShouldSwapWeapons();

protected:

	virtual void BeginPlay() override;

	/*
	* Fire
	*/

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void LocalFire(const FVector_NetQuantize& TraceHitTarget);			// play fire effects on clients -Lag

	UFUNCTION(Server, Reliable)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);	// Take HitTargets from client's local simulation of scatter.

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	void ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	void FireProjectileWeapon();
	void FireHitscanWeapon();
	void FireShotgunWeapon();
	/*
	* Aiming
	*/

	UFUNCTION(Server, Reliable) //aiming from a client machine doesn't replicate on other clients & server. create an RPC
	void ServerSetAiming(bool bIsAiming);

	/*
	* Reload
	*/

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	void UpdateAmmoValues();

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void UpdateShotgunAmmoValues();

	/*
	* Corsshairs
	*/

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	/*
	* Grenades
	*/

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void GrabGrenade();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	void ShowAttachedGrenade(bool bShowGrenade);
	
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

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 750;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 300;

	/*
	*	Equipped Weapon	/ Secondary Weapon
	*/

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	class AWeapon* SecondaryWeapon;

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackpack(AActor* ActorToAttach);

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

	FVector HitTarget;				//need in the anime instance to fix the weapon rotation

	/*
	* AIMING AND FOV
	*/
	
	UPROPERTY(Replicated)
	bool bAiming;

	float DefaultFOV;				//field of view when not aiming; Set to the camera's base FOV in BeginPlay

	float CurrentFOV;

	UPROPERTY(EditAnywhere , Category = "Combat")
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed = 20.f; //used to make all weapons unzoom at the same rate.

	void InterpFOV(float DeltaTime);

	/*
	* AUTOMATIC FIRE
	*/

	bool bFireButtonPressed;

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

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 70;

	TMap<EWeaponType, int32> CarriedAmmoMap;

	void InitializeCarriedAmmo();

	void SetCarriedAmmoOnEquip();
	
	void UpdateHUDCarriedAmmo();

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 2;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 12;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 20;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 5;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 15;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 8;

	/*
	* Combat State
	*/

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied; //start of as unoccpied

	UFUNCTION()
	void OnRep_CombatState();

	/*
	* Grenades
	*/

	UPROPERTY(ReplicatedUsing = OnREp_Grenades)
	int32 Grenades = 4;

	UFUNCTION()
	void OnRep_Grenades();

	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;

	void UpdateHUDGrenades();

	/*
	* COSMETICS
	*/

	void AutoReloadIfEmpty();
	void UpdateHUDWeaponType();

	void PlaySoundIfEmpty();

	UPROPERTY(EditAnywhere)
	USoundCue* SniperZoomInSound;

	UPROPERTY(EditAnywhere)
	USoundCue* SniperZoomOutSound;

	void DrawSniperScope(bool bDraw);
	void PlaySniperScopeSound(bool bSniperAiming);

public:	

	FORCEINLINE int32 GetGrenades() const { return Grenades; }
	FORCEINLINE int32 GetCarriedAmmo() const { return CarriedAmmo; }
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
};
