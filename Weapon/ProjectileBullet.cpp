// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include"GameFramework/Character.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	/*
	* Call ApplyDamage() which will trigger a damage event..
	* this damage event needs to be bound to a callback to actually apply the damage.
	*/
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
