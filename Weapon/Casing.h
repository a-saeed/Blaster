// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class BLASTER_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	

	ACasing();

protected:

	virtual void BeginPlay() override;

	UFUNCTION() //it has to be a ufunction to work
		virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:

	UPROPERTY(VisibleAnyWhere)
		UStaticMeshComponent* CasingMesh;

	UPROPERTY(EditAnyWhere)
		float ShellEjectionImpulse;

	UPROPERTY(EditAnyWhere)
		class USoundCue* ShellSound;
public:	

};
