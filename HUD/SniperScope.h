// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SniperScope.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API USniperScope : public UUserWidget
{
	GENERATED_BODY()

private:

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ScopeZoomIn;

public:

	FORCEINLINE UWidgetAnimation* GetScopeAnimation() const { return ScopeZoomIn; }
};
