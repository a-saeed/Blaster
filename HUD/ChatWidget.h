// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UChatWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta = (BindWidget))
	class UEditableText* ChatText;
};
