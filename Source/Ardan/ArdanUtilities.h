//A place for all the useful function snippets that don't belong anywhere else
#pragma once
#include "PlatformFeatures.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "GameFramework/SaveGame.h"
#include "SaveGameSystem.h"

#define LOG(txt) { GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, txt); };
/**
 * 
 */
class ARDAN_API ArdanUtilities
{
public:
	ArdanUtilities();
	~ArdanUtilities();
	static const int SCP_UE4_SAVEGAME_FILE_TYPE_TAG = 0x53415643;		// "SAVC", not SAVG
	static const int SCP_UE4_SAVEGAME_FILE_VERSION = 1;


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

	static bool SaveGameToSlotCompressed(USaveGame* SaveGameObject, const FString& SlotName, const int32 UserIndex) {

		bool bSuccess = false;

		ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
		// If we have a system and an object to save and a save name...
		if (SaveSystem && (SaveGameObject != NULL) && (SlotName.Len() > 0))
		{
			TArray<uint8> ObjectBytes;
			FMemoryWriter MemoryWriter(ObjectBytes, true);

			// write file type tag. identifies this file type and indicates it's using proper versioning
			// since older UE4 versions did not version this data.
			int32 FileTypeTag = SCP_UE4_SAVEGAME_FILE_TYPE_TAG;
			MemoryWriter << FileTypeTag;

			// Write version for this file format
			int32 SavegameFileVersion = SCP_UE4_SAVEGAME_FILE_VERSION;
			MemoryWriter << SavegameFileVersion;

			// Write out engine and UE4 version information
			int32 PackageFileUE4Version = GPackageFileUE4Version;
			MemoryWriter << PackageFileUE4Version;
			FEngineVersion SavedEngineVersion = FEngineVersion::Current();
			MemoryWriter << SavedEngineVersion;

			// Write the class name so we know what class to load to
			FString SaveGameClassName = SaveGameObject->GetClass()->GetName();
			MemoryWriter << SaveGameClassName;
			FBufferArchive buffer;
			// Then save the object state, replacing object refs and names with strings
			//FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
			FObjectAndNameAsStringProxyArchive A(buffer, false);
			SaveGameObject->Serialize(A);
			// Save Compressed Data
			TArray<uint8> CompressedData;
			FArchiveSaveCompressedProxy Compressor =
				FArchiveSaveCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);
			//SaveGameObject->Serialize(Compressor);
			//SaveGameObject->
			Compressor << buffer;
			Compressor.Flush();
			// Output Compressed Data
			MemoryWriter << CompressedData;

			// Stuff that data into the save system with the desired file name
			bSuccess = SaveSystem->SaveGame(false, *SlotName, UserIndex, ObjectBytes);

			Compressor.FlushCache();
			CompressedData.Empty();
			MemoryWriter.FlushCache();
			MemoryWriter.Close();
		}

		return bSuccess;
	}

	static USaveGame*LoadGameFromSlotCompressed(const FString& SlotName, const int32 UserIndex)
	{
		USaveGame* OutSaveGameObject = NULL;

		ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
		// If we have a save system and a valid name..
		if (SaveSystem && (SlotName.Len() > 0))
		{
			// Load raw data from slot
			TArray<uint8> ObjectBytes;
			bool bSuccess = SaveSystem->LoadGame(false, *SlotName, UserIndex, ObjectBytes);
			if (bSuccess)
			{
				FMemoryReader MemoryReader(ObjectBytes, true);

				int32 FileTypeTag;
				MemoryReader << FileTypeTag;

				int32 SavegameFileVersion;
				if (FileTypeTag != SCP_UE4_SAVEGAME_FILE_TYPE_TAG)
				{
					// this is an old saved game, back up the file pointer to the beginning and assume version 1
					MemoryReader.Seek(0);
					SavegameFileVersion = 1;

					// Note for 4.8 and beyond: if you get a crash loading a pre-4.8 version of your savegame file and 
					// you don't want to delete it, try uncommenting these lines and changing them to use the version 
					// information from your previous build. Then load and resave your savegame file.
					//MemoryReader.SetUE4Ver(MyPreviousUE4Version);				// @see GPackageFileUE4Version
					//MemoryReader.SetEngineVer(MyPreviousEngineVersion);		// @see GEngineVersion
				}
				else
				{
					// Read version for this file format
					MemoryReader << SavegameFileVersion;

					// Read engine and UE4 version information
					int32 SavedUE4Version;
					MemoryReader << SavedUE4Version;

					FEngineVersion SavedEngineVersion;
					MemoryReader << SavedEngineVersion;

					MemoryReader.SetUE4Ver(SavedUE4Version);
					MemoryReader.SetEngineVer(SavedEngineVersion);
				}

				// Get the class name
				FString SaveGameClassName;
				MemoryReader << SaveGameClassName;

				// Try and find it, and failing that, load it
				UClass* SaveGameClass = FindObject<UClass>(ANY_PACKAGE, *SaveGameClassName);
				if (SaveGameClass == NULL)
				{
					SaveGameClass = LoadObject<UClass>(NULL, *SaveGameClassName);
				}

				// If we have a class, try and load it.
				if (SaveGameClass != NULL)
				{
					OutSaveGameObject = NewObject<USaveGame>(GetTransientPackage(), SaveGameClass);

					//FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
					//OutSaveGameObject->Serialize(Ar);

					// slice tail
					TArray<uint8> CompressedData;
					MemoryReader << CompressedData;

					FArchiveLoadCompressedProxy Decompressor =
						FArchiveLoadCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);
					if (Decompressor.GetError())
					{
						LOG(TEXT("FArchiveLoadCompressedProxy>> ERROR : File Was Not Compressed "));
						return nullptr;
					}
					FBufferArchive buffer;
					FObjectAndNameAsStringProxyArchive Ar(buffer, true);
					
					
					Decompressor << buffer;
					UE_LOG(LogNet, Log, TEXT("SAVESIZE: %d"), buffer.Num());
					//Decompressor.Flush();
					OutSaveGameObject->Serialize(Ar);
					//OutSaveGameObject->Serialize(buffer);

 					Decompressor.FlushCache();
					CompressedData.Empty();
					MemoryReader.FlushCache();
					MemoryReader.Close();
				}
			}
		}

		return OutSaveGameObject;
	}


};


