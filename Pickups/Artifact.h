// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "Blaster/BlasterTypes/ArtifactState.h"
#include "Artifact.generated.h"

/**
 *
 */
UCLASS()
class BLASTER_API AArtifact : public APickup
{
	GENERATED_BODY()
	
public:

	AArtifact();

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Destroyed() override;

	//virtual void OnRep_Owner() override;

	void Dropped();

protected:

	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:

	UPROPERTY(Replicated)
	class ABlasterCharacter* BlasterOwnerCharacter;

	UPROPERTY(EditAnywhere)
	class UStaticMeshComponent* LightBeam;

	UPROPERTY(EditAnywhere)
	class UParticleSystemComponent* PowerComponent;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* PowerEffect;

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	UPROPERTY(ReplicatedUsing = OnRep_ArtifactState, VisibleAnywhere, Category = "Artifact Properties")
	EArtifactState ArtifactState = EArtifactState::EAS_Dropped;

	UFUNCTION()
	void OnRep_ArtifactState();

	void SetArtifactState(EArtifactState NewState);

	void OnArtifactStateSet();

	void HandleArtifactEquipped();

	void HandleArtifactDropped();

	void PlayEquipEffects();

public:
};
