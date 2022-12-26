#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
	void Fire();

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

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class USphereComponent* AreaSphere; //to detect overlap events with the weapon, just like a capsule comp.

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
		EWeaponState WeaponState;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class UWidgetComponent* PickupWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties")
		float FiringRange;

	UFUNCTION()
		void OnRep_WeaponState(); //what should we do once the new value of weapon state replicates to clients


	/*
	*	ANIMATION ASSETS
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
		class UAnimationAsset* FireAnimation;

public:

	void SetWeaponState(EWeaponState State);

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() { return WeaponMesh; }

	FORCEINLINE void SetWeaponFiringRange(float range) { FiringRange = range; }
	FORCEINLINE float GetWeaponFiringRange() { return FiringRange; }
};
