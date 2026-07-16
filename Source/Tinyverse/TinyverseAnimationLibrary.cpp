#include "TinyverseAnimationLibrary.h"

float UTinyverseAnimationLibrary::CalculatePlanetDirection(
	const FVector& Velocity,
	const FRotator& BaseRotation)
{
	const FRotationMatrix RotationMatrix(BaseRotation);
	const FVector Forward = RotationMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = RotationMatrix.GetUnitAxis(EAxis::Y);
	const FVector LocalUp = RotationMatrix.GetUnitAxis(EAxis::Z);
	const FVector TangentVelocity = FVector::VectorPlaneProject(Velocity, LocalUp).GetSafeNormal();

	if (TangentVelocity.IsNearlyZero())
	{
		return 0.0f;
	}

	return FMath::RadiansToDegrees(FMath::Atan2(
		FVector::DotProduct(TangentVelocity, Right),
		FVector::DotProduct(TangentVelocity, Forward)));
}
