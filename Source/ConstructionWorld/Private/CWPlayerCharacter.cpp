#include "CWPlayerCharacter.h"

#include "CWInteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

ACWPlayerCharacter::ACWPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 540.f, 0.f);
	Movement->JumpZVelocity = 420.f;
	Movement->AirControl = 0.35f;
	Movement->MaxWalkSpeed = 450.f;
	Movement->BrakingDecelerationWalking = 1800.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 420.f;
	CameraBoom->SocketOffset = FVector(0.f, 60.f, 70.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->bAutoActivate = false;

	InteractionComponent = CreateDefaultSubobject<UCWInteractionComponent>(TEXT("Interaction"));

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
	PlaceholderMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	PlaceholderMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.9f));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetGenerateOverlapEvents(false);
	PlaceholderMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void ACWPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	EnsureInputObjects();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ACWPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureInputObjects();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("CWPlayerCharacter: se esperaba un UEnhancedInputComponent. Revisa DefaultInput.ini / EnhancedInput."));
		return;
	}

	EnhancedInput->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &ACWPlayerCharacter::MoveForward);
	EnhancedInput->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &ACWPlayerCharacter::MoveRight);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACWPlayerCharacter::Look);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACWPlayerCharacter::StartJump);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACWPlayerCharacter::StopJump);
	EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ACWPlayerCharacter::InteractInput);
	EnhancedInput->BindAction(ToggleCameraAction, ETriggerEvent::Started, this, &ACWPlayerCharacter::ToggleCameraInput);
}

void ACWPlayerCharacter::EnsureInputObjects()
{
	if (IsValid(DefaultMappingContext))
	{
		return;
	}

	MoveForwardAction = NewObject<UInputAction>(this, TEXT("IA_MoveForward"));
	MoveForwardAction->ValueType = EInputActionValueType::Axis1D;

	MoveRightAction = NewObject<UInputAction>(this, TEXT("IA_MoveRight"));
	MoveRightAction->ValueType = EInputActionValueType::Axis1D;

	LookAction = NewObject<UInputAction>(this, TEXT("IA_Look"));
	LookAction->ValueType = EInputActionValueType::Axis2D;

	JumpAction = NewObject<UInputAction>(this, TEXT("IA_Jump"));
	JumpAction->ValueType = EInputActionValueType::Boolean;

	InteractAction = NewObject<UInputAction>(this, TEXT("IA_Interact"));
	InteractAction->ValueType = EInputActionValueType::Boolean;

	ToggleCameraAction = NewObject<UInputAction>(this, TEXT("IA_ToggleCamera"));
	ToggleCameraAction->ValueType = EInputActionValueType::Boolean;

	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));

	DefaultMappingContext->MapKey(MoveForwardAction, EKeys::W);

	FEnhancedActionKeyMapping& BackwardMapping = DefaultMappingContext->MapKey(MoveForwardAction, EKeys::S);
	BackwardMapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));

	DefaultMappingContext->MapKey(MoveRightAction, EKeys::D);

	FEnhancedActionKeyMapping& LeftMapping = DefaultMappingContext->MapKey(MoveRightAction, EKeys::A);
	LeftMapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));

	DefaultMappingContext->MapKey(LookAction, EKeys::Mouse2D);
	DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	DefaultMappingContext->MapKey(InteractAction, EKeys::E);
	DefaultMappingContext->MapKey(ToggleCameraAction, EKeys::V);
}

void ACWPlayerCharacter::MoveForward(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>();
	if (!Controller || FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, AxisValue);
}

void ACWPlayerCharacter::MoveRight(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>();
	if (!Controller || FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, AxisValue);
}

void ACWPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void ACWPlayerCharacter::StartJump(const FInputActionValue& Value)
{
	Jump();
}

void ACWPlayerCharacter::StopJump(const FInputActionValue& Value)
{
	StopJumping();
}

void ACWPlayerCharacter::InteractInput(const FInputActionValue& Value)
{
	if (InteractionComponent)
	{
		InteractionComponent->Interact();
	}
}

void ACWPlayerCharacter::ToggleCameraInput(const FInputActionValue& Value)
{
	ToggleCamera();
}

void ACWPlayerCharacter::ToggleCamera()
{
	bFirstPerson = !bFirstPerson;

	FollowCamera->SetActive(!bFirstPerson);
	FirstPersonCamera->SetActive(bFirstPerson);
	PlaceholderMesh->SetOwnerNoSee(bFirstPerson);
}
