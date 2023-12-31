// Fill out your copyright notice in the Description page of Project Settings.


#include "Cannon.h"

#include "DrawDebugHelpers.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "DamageTaker.h"

ACannon::ACannon()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* sceeneCpm = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = sceeneCpm;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cannon mesh"));
	Mesh->SetupAttachment(RootComponent);

	ProjectileSpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn point"));
	ProjectileSpawnPoint->SetupAttachment(Mesh);

	
	ShootEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Shoot effect"));
	ShootEffect->SetupAttachment(ProjectileSpawnPoint);

	AudioEffect = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio effect"));
	AudioEffect->SetupAttachment(ProjectileSpawnPoint);
	
}

void ACannon::Fire()
{
	if (!ReadyToFire)
	{
		return;
	}
	else
	{
		count--;
	}
	ReadyToFire = false;

	
	ShootEffect->ActivateSystem();
	AudioEffect->Play();

	
	if (GetOwner() && GetOwner() == GetWorld()->GetFirstPlayerController()->GetPawn())
	{
		if (ShootForceEffect)
		{
			FForceFeedbackParameters shootForceEffectParams;
			shootForceEffectParams.bLooping = false;
			shootForceEffectParams.Tag = "shootForceEffectParams";
			GetWorld()->GetFirstPlayerController()->ClientPlayForceFeedback(ShootForceEffect, shootForceEffectParams);
		}

		if (ShootShake)
		{
			//GetWorld()->GetFirstPlayerController()->PlayerCameraManager->PlayCameraShake(ShootShake, 1.0f);
			GetWorld()->GetFirstPlayerController()->ClientPlayCameraShake(ShootShake);
		}
	}
	

	if (Type == ECannonType::FireProjectile)
	{
		GEngine->AddOnScreenDebugMessage(10, 1, FColor::Green, "Fire - projectile");
		
		FTransform projectileTransform(ProjectileSpawnPoint->GetComponentRotation(), ProjectileSpawnPoint->GetComponentLocation(), FVector(1));

		//AProjectile* projectile = GetWorld()->SpawnActorDeferred<AProjectile>(ProjectileClass, projectileTransform, this);
		AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, 
										ProjectileSpawnPoint->GetComponentLocation(), 
										ProjectileSpawnPoint->GetComponentRotation());
		if (projectile)
		{
			// ....
			//projectile->FinishSpawning(projectileTransform);

			projectile->Start();

		}
		
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(10, 1, FColor::Green, "Fire - trace");
		
		
		FHitResult hitResult;
		FCollisionQueryParams traceParams = FCollisionQueryParams(FName(TEXT("FireTrace")), true, this);
		traceParams.bTraceComplex = true;
		traceParams.bReturnPhysicalMaterial = false;

		FVector start = ProjectileSpawnPoint->GetComponentLocation();
		FVector end = ProjectileSpawnPoint->GetForwardVector() * FireRange + start;
		if (GetWorld()->LineTraceSingleByChannel(hitResult, start, end, ECollisionChannel::ECC_Visibility, traceParams))
		{
			DrawDebugLine(GetWorld(), start, hitResult.Location, FColor::Red, false, 0.5f, 0, 5);
			if (hitResult.GetActor())
			{
				hitResult.GetActor()->Destroy();
			}

			IDamageTaker * damageTakerActor = Cast<IDamageTaker>(hitResult.GetActor());
			if(damageTakerActor)
			{
				FDamageData damageData;
				damageData.DamageValue = FireDamage;
			 	damageData.Instigator = this;
			 	damageData.DamageMaker = this;
			
			 	damageTakerActor->TakeDamage(damageData);
			}
		}
		else
		{
			DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.5f, 0, 5);
		}
		
		
	}

	GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, this, &ACannon::Reload, 1 / FireRate, false);
}


void ACannon::FireSpecial()
{
	Fire();
	Fire();
}

bool ACannon::IsReadyToFire()
{
	return (ReadyToFire && count > 0);
}

void ACannon::AddAmmo(int _count)
{
	count += _count;
}



void ACannon::Reload()
{
	ReadyToFire = true;
	count = 1000;
}

// Called when the game starts or when spawned
void ACannon::BeginPlay()
{
	Super::BeginPlay();
	Reload();
}

void ACannon::SwitchFireType()
{
	if (Type == ECannonType::FireProjectile)
		Type = ECannonType::FireTrace;

	else if (Type == ECannonType::FireTrace)
		Type = ECannonType::FireProjectile;

}