#include "TankFactory.h"
#include "TimerManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MapLoader.h"


ATankFactory::ATankFactory()
{
	PrimaryActorTick.bCanEverTick = false;
	
	USceneComponent* sceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = sceneComp;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Building Mesh"));
	BuildingMesh->SetupAttachment(sceneComp);

	DestroyedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Destroyed Mesh"));
	DestroyedMesh->SetupAttachment(sceneComp);
	DestroyedMesh->SetVisibility(false); // Скрыть разрушенный меш изначально

	TankSpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("Cannon setup point"));
	TankSpawnPoint->AttachToComponent(sceneComp, FAttachmentTransformRules::KeepRelativeTransform);

	HitCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("Hit collider"));
	HitCollider->SetupAttachment(sceneComp);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health component"));
	HealthComponent->OnDie.AddUObject(this, &ATankFactory::Die);
	HealthComponent->OnDamaged.AddUObject(this, &ATankFactory::DamageTaked);

}

// Called when the game starts or when spawned
void ATankFactory::BeginPlay()
{
	Super::BeginPlay();

	if (LinkedMapLoader)
		LinkedMapLoader->SetIsActivated(false);

	if (!bIsDestroyed)
	{
		DestroyedMesh->SetVisibility(false);

		GetWorld()->GetTimerManager().SetTimer(_targetingTimerHandle, this, &ATankFactory::SpawnNewTank, 
			SpawnTankRate, true, SpawnTankRate);
	}
	
}

void ATankFactory::TakeDamage(FDamageData DamageData)
{
	HealthComponent->TakeDamage(DamageData);
}

void ATankFactory::Die()
{
	// Остановка таймера
	GetWorld()->GetTimerManager().ClearTimer(_targetingTimerHandle);

	if (LinkedMapLoader)
		LinkedMapLoader->SetIsActivated(true);

	BuildingMesh->SetVisibility(false); // Скрыть нормальный меш
	DestroyedMesh->SetVisibility(true); // Отобразить разрушенный меш
	bIsDestroyed = true;

	//Destroy();
}

void ATankFactory::DamageTaked(float DamageValue)
{
	UE_LOG(LogTemp, Warning, TEXT("Factory %s taked damage:%f Health:%f"), *GetName(), DamageValue, HealthComponent->GetHealth());
}

void ATankFactory::SpawnNewTank()
{
	if (bIsDestroyed)
		return;

	FTransform spawnTransform(TankSpawnPoint->GetComponentRotation(), 
					TankSpawnPoint->GetComponentLocation(), FVector(1));

	ATankPawn* newTank = GetWorld()->SpawnActorDeferred<ATankPawn>(SpawnTankClass, spawnTransform, this, 
					nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	newTank->SetPatrollingPoints(TankWayPoints);

	UGameplayStatics::FinishSpawningActor(newTank, spawnTransform);
}


void ATankFactory::OnTriggerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor*	OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsDestroyed && LinkedMapLoader)
		if (LinkedMapLoader->GetIsActivated())
		{
			APawn* PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
			if (OtherActor == PlayerPawn)
			{
				UGameplayStatics::OpenLevel(GetWorld(), LinkedMapLoader->GetLoadLevelName());
			}
		}
	
}