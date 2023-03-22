// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include"GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;

	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

void AProjectileBullet::BeginPlay()
{
	AActor::BeginPlay();

	SpawnBulletTracer();

	StartDestroyTimer();

	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileBullet::OnBounce);

	/*Predict projectile path fo server side rewind*/
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithChannel = true;
	PathParams.bTraceWithCollision = true;
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	PathParams.MaxSimTime = 4.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.SimFrequency = 30.f;
	PathParams.StartLocation = GetActorLocation();
	PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	PathParams.ActorsToIgnore.Add(this);

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(GetWorld(), PathParams, PathResult);
}

void AProjectileBullet::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	SetImpactSurfaceEffects(ImpactResult.GetActor());

	ACharacter* TargetCharacter = Cast<ACharacter>(ImpactResult.GetActor());
	if (TargetCharacter)
	{
		ProjectileMovementComponent->SetVelocityInLocalSpace(FVector().ZeroVector);
		if (HasAuthority())
		{
			ApplyPointDamage(TargetCharacter);
		}
	}
	
}

void AProjectileBullet::ApplyPointDamage(AActor* Target)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		if (OwnerCharacter != Target)
		{
			AController* OwnerController = OwnerCharacter->Controller;
			if (OwnerController)
			{
				UGameplayStatics::ApplyDamage(Target, Damage, OwnerController, this, UDamageType::StaticClass());
			}
		}
	}

	Destroy();
}

/*
void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
if (OwnerCharacter)
{
	AController* OwnerController = OwnerCharacter->Controller;
	if (OwnerController)
	{
		UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
	}
}

//the Super version has a call to Destroy. keep super version last.
Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
*/