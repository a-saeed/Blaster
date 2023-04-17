// Fill out your copyright notice in the Description page of Project Settings.


#include "Artifact.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SphereComponent.h"
#include "Blaster/Weapon/Projectile.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Kismet/GamePlayStatics.h"
#include "Sound/SoundCue.h"

AArtifact::AArtifact()
{
	SetReplicateMovement(true);

	PickupMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	PickupMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	LightBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Light Beam"));
	LightBeam->SetupAttachment(RootComponent);
	LightBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LightBeam->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
}

void AArtifact::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(AArtifact, ArtifactState);
	DOREPLIFETIME(AArtifact, BlasterOwnerCharacter);
}

void AArtifact::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);
}

void AArtifact::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// only called on the server

	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OtherActor) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter && !BlasterOwnerCharacter->IsElimmed() && GetOwner() == nullptr)
	{
		SetOwner(BlasterOwnerCharacter);
		// disable overlap events
		GetOverlapSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		SetArtifactState(EArtifactState::EAS_Equipped);
	}
}

void AArtifact::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	AProjectile* Projectile = Cast<AProjectile>(OtherActor);

	if (!PickupMesh || Projectile) return;	// don't want artifact to stop falling if hit by a projectile..

	PickupMesh->SetSimulatePhysics(false);
	PickupMesh->SetEnableGravity(false);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetWorldRotation(FRotator(0.f, 0.f, 0.f));

	LightBeam->SetVisibility(true);
}

// public -should only be called on the server
void AArtifact::Dropped()
{
	SetArtifactState(EArtifactState::EAS_Dropped); //replicated
}

void AArtifact::SetArtifactState(EArtifactState NewState)
{
	ArtifactState = NewState;

	OnArtifactStateSet();
}

void AArtifact::OnRep_ArtifactState()
{
	OnArtifactStateSet();
}

void AArtifact::OnArtifactStateSet()
{
	switch (ArtifactState)
	{
	case EArtifactState::EAS_Equipped:
		HandleArtifactEquipped();
		break;

	case EArtifactState::EAS_Dropped:
		HandleArtifactDropped();
		break;
	}
}

void AArtifact::HandleArtifactEquipped()
{
	if (!BlasterOwnerCharacter || !PickupMesh) return;

	if (PickupMesh->OnComponentHit.IsBound())
	{
		PickupMesh->OnComponentHit.RemoveDynamic(this, &AArtifact::OnHit);
		PickupMesh->SetNotifyRigidBodyCollision(false);
	}

	if (HasAuthority())
	{
		// Broadcast State
		ABlasterGameState* BlasterGameState = GetWorld()->GetGameState<ABlasterGameState>();
		if (BlasterGameState) BlasterGameState->MultiCastBroadcastStatus(BlasterOwnerCharacter, EArtifactState::EAS_Equipped);
	}

	PickupMesh->SetSimulatePhysics(false);
	PickupMesh->SetEnableGravity(false);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BlasterOwnerCharacter->SetArtifact(this);
	SetOwner(BlasterOwnerCharacter);

	LightBeam->SetVisibility(false);
	PlayEquipEffects();
}

void AArtifact::HandleArtifactDropped()
{	
	if (!PickupMesh) return;

	if (HasAuthority())
	{
		GetOverlapSphere()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);	// re-generate overlap events only on the server

		// Broadcast State
		ABlasterGameState* BlasterGameState = GetWorld()->GetGameState<ABlasterGameState>();
		if (BlasterGameState) BlasterGameState->MultiCastBroadcastStatus(BlasterOwnerCharacter, EArtifactState::EAS_Dropped);
	}

	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;

	if (PowerComponent)
	{
		PowerComponent->Deactivate();
		PowerComponent = nullptr;
	}

	/* Detach from socket*/
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	PickupMesh->DetachFromComponent(DetachRules);

	/* Allow free falling*/
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PickupMesh->SetSimulatePhysics(true);
	PickupMesh->SetEnableGravity(true);

	/* register hit events*/
	PickupMesh->SetNotifyRigidBodyCollision(true);
	PickupMesh->OnComponentHit.AddDynamic(this, &AArtifact::OnHit);

	if (EquipSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), EquipSound, GetActorLocation());
}

void AArtifact::Destroyed()
{
	if (HasAuthority())
	{
		// Broadcast State
		ABlasterGameState* BlasterGameState = GetWorld()->GetGameState<ABlasterGameState>();
		if (BlasterGameState) BlasterGameState->MultiCastBroadcastStatus(BlasterOwnerCharacter, EArtifactState::EAS_Captured);
	}

	if (BlasterOwnerCharacter)
	{
		BlasterOwnerCharacter->SetMovementMode(EMovementMode::MOVE_Walking);
		BlasterOwnerCharacter->SetArtifact(nullptr);
	}
	Super::Destroyed();
}

void AArtifact::PlayEquipEffects()
{
	if (PowerEffect && !PowerComponent)
	{
		PowerComponent = UGameplayStatics::SpawnEmitterAttached(PowerEffect, GetRootComponent());
	}
	if (EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), EquipSound, GetActorLocation());
	}
}