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
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX"),
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	

	AWeapon();

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; //we need to replicate the weapon state enum variable.

	void ShowPickupWidget(bool bPickupWidget); //C7_6

	virtual void Fire(const FVector& HitTarget);
	/*
	*
	*  AMMO
	*
	*/
	virtual void OnRep_Owner() override;

	void SetHUDAmmo();
	/**
	*	TEXTURES FOR THE WEAPON CROSSHAIRS
	*/

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
		class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
		UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
		UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
		UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
		UTexture2D* CrosshairsBottom;

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

	/*
	* ZOOMED FOV WHILE AIMING.
	*/
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
		float ZoomedFOV = 30.f; //amount to zoom depending on each weapon.

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
		float ZoomInterpSpeed = 20.f;

private:

	/*
	*	BODY
	*/
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class USphereComponent* AreaSphere; //to detect overlap events with the weapon, just like a capsule comp.

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class UWidgetComponent* PickupWidget;

	/*
	*	WEAPON PROPERTIES
	*/
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
		EWeaponState WeaponState;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties")
		float FiringRange;

	void SetWeaponMeshPhysics(bool bEnable);

	/*
	*	REP_NOTIFIES
	*/
	UFUNCTION()
		void OnRep_WeaponState(); //what should we do once the new value of weapon state replicates to clients

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
	* 
	*  AMMO
	*  
	*/
	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;

	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo, Category = "Weapon Properties")
		int32 Ammo = 0;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
		int32 MagCapacity = 50;

	UFUNCTION()
		void OnRep_Ammo();

	void SpendRound();
	/*
	*
	*  WEAPON TYPESS
	*
	*/
	UPROPERTY(EditAnywhere, Category = "Weapon Type")
		EWeaponType WeaponType;

public:

	void SetWeaponState(EWeaponState State);

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() { return WeaponMesh; }

	FORCEINLINE float GetWeaponFiringRange() { return FiringRange; }

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

	FORCEINLINE EWeaponType GetWeaponType() { return WeaponType; }
};
