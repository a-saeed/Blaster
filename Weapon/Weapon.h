#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8 //create an enum constant for weapon state (initial, equipped, dropped)
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSeconadry UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX"),
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_Hitscan UMETA (DisplayName = "Hitscan"),
	EFT_Projectile UMETA (DisplayName = "Projectile"),
	EFT_Shotgun UMETA (DisplayName = "Shotgun"),

	EFT_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	

	AWeapon();

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ShowPickupWidget(bool bPickupWidget);

	virtual void Fire(const FVector& HitTarget);
 
	virtual void OnRep_Owner() override;	//update HUD ammo once a weapon has a new owner

	void SetWeaponState(EWeaponState State);

	void Dropped();

	/**
	*   SCATTER
	*/

	FVector TraceEndWithScatter(const FVector& HitTarget);

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;

	/**
	*	TEXTURES FOR THE WEAPON CROSSHAIRS
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UTexture2D* CrosshairsBottom;

	/*
	*  Cosmetics
	*/

	void EnableCustomDepth(bool bEnable);
	void PlayEquipSound();

protected:

	virtual void BeginPlay() override;

	UFUNCTION()  //this fn. isn't inherited. it's virtual as other classes may derive from weapon later and change what happens when the player overlpas with that certain weapon 
		virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex,
			bool bFromSweep,
			const FHitResult& SweepResult);

	UFUNCTION() 
		void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex);

private:

	/*
	*	BODY
	*/

	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;

	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere; //to detect overlap events with the weapon, just like a capsule comp.

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EFireType FireType;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class USoundCue* EquipSound;

	/*
	*	WEAPON STATE
	*/

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	void OnWeaponStateSet();

	void HandlePrimaryWeaponEquipped();

	void HandleSecondaryWeaponEquipped();

	void HandleWeaponDropped();

	/*
	*	MESH PHYSICS
	*/

	void SetWeaponMeshPhysics(bool bEnable);
	void EnablePhysicsSMG(bool bEnable);

	/*
	*	ANIMATION ASSETS
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	/*
	*	BULLET SHELL
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<class ACasing> CasingClass;

	/*
	*   AUTOMATIC FIRE
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float FireDelay = 0.15f; //fire rate

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	bool bAutomatic = true;

	/*
	*  AMMO
	*/

	UPROPERTY(EditAnywhere)
	int32 Ammo = 0;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	int32 MagCapacity = 50;

	void SpendRound();

	UFUNCTION(Client, Reliable)					//Server Reconciliation
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)					//Delayed ammo addition
	void ClientAddAmmo(int32 ServerAmmo);

	int32 Sequence = 0;							// The number of unprocessed server requests for ammo; Incremented in SpendRound, decremented in ClientUpdateAmmo
	/*
	* ZOOMED FOV WHILE AIMING.
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float ZoomedFOV = 30.f; //amount to zoom depending on each weapon.

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float ZoomInterpSpeed = 20.f;

public:

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() { return WeaponMesh; }

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }

	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE float IsAutomatic() const { return bAutomatic; }

	FORCEINLINE void SetOwnerCharacter(ABlasterCharacter* Character){ BlasterOwnerCharacter = Character; }
	FORCEINLINE void SetOwnerController(ABlasterPlayerController* Controller) { BlasterOwnerController = Controller; }

	FORCEINLINE bool IsEmpty() { return Ammo <= 0; }
	FORCEINLINE bool IsFull() { return Ammo == MagCapacity; }
	FORCEINLINE int32 GetCurrentAmmo() { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() { return MagCapacity; }
	void AddAmmo(int32 AmmoAmount);
	void SetHUDAmmo();

	FORCEINLINE EWeaponType GetWeaponType() { return WeaponType; }
	FORCEINLINE EFireType GetFireType() { return FireType; }
};
