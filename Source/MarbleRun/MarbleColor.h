#pragma once

#include "CoreMinimal.h"

class UPrimitiveComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

namespace MarbleColor
{
	/**
	 * Tints a mesh through a dynamic instance of BaseMaterial (default: the engine BasicShapeMaterial),
	 * which must expose a "Color" vector parameter. Safe in the editor (construction) and at runtime.
	 */
	MARBLERUN_API UMaterialInstanceDynamic* Apply(UPrimitiveComponent* Mesh, const FLinearColor& Color,
		UMaterialInterface* BaseMaterial = nullptr);
}
