#include "MarbleColor.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	// Since UE 5.8 the engine BasicShapes meshes default to WorldGridMaterial/DefaultMaterial,
	// which have no colour parameter, so always build the tint from BasicShapeMaterial.
	UMaterialInterface* GetTintableMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		return Cached.Get();
	}
}

UMaterialInstanceDynamic* MarbleColor::Apply(UPrimitiveComponent* Mesh, const FLinearColor& Color, UMaterialInterface* BaseMaterial)
{
	if (!Mesh)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0, BaseMaterial ? BaseMaterial : GetTintableMaterial());
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
	}
	return MID;
}
