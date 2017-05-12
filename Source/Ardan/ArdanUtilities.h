//A place for all the useful function snippets that don't belong anywhere else
#pragma once

/**
 * 
 */
class ARDAN_API ArdanUtilities
{
public:
	ArdanUtilities();
	~ArdanUtilities();

	template <typename ObjClass> static FORCEINLINE ObjClass* LoadObjFromPath(const FName& Path) {
		if (Path == NAME_None) return NULL;
		return Cast<ObjClass>(StaticLoadObject(ObjClass::StaticClass(), NULL, *Path.ToString()));
	}

	static FORCEINLINE UMaterialInterface* LoadMatFromPath(const FName& Path) {
		if (Path == NAME_None) return NULL;
		return LoadObjFromPath<UMaterialInterface>(Path);
	}

	template<class T> static T* SpawnActor(UWorld* world, FVector const& Location, FRotator const& Rotation, AActor* Owner = NULL, APawn* Instigator = NULL, bool bNoCollisionFail = false) {
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = Owner;
		SpawnInfo.Instigator = Instigator;
		SpawnInfo.bDeferConstruction = false;
		SpawnInfo.bNoFail = bNoCollisionFail;
		// SpawnInfo.bNoCollisionFail = bNoCollisionFail;
		return (T*)(world->SpawnActor<T>(&Location, &Rotation, SpawnInfo));
	}

	template <typename VictoryObjType> static VictoryObjType* SpawnBP(UWorld* TheWorld, UClass* TheBP, const FVector& Loc, const FRotator& Rot, const bool bNoCollisionFail = true, AActor* Owner = NULL, APawn* Instigator = NULL) {
		if (!TheWorld) return NULL;
		if (!TheBP) return NULL;

		FActorSpawnParameters SpawnInfo;
		// SpawnInfo.bNoCollisionFail 		= bNoCollisionFail;
		SpawnInfo.Owner = Owner;
		SpawnInfo.Instigator = Instigator;
		SpawnInfo.bDeferConstruction = false;
		return TheWorld->SpawnActor<VictoryObjType>(TheBP, Loc, Rot, SpawnInfo);
	}


};


