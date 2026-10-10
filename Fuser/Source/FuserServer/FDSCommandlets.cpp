#include "FDSCommandlets.h"
#include "FDSSessionToken.h"
#include "Dom/JsonObject.h"
#include "Misc/DateTime.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "FuserServer.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/CoreNet.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

// ---------------------------------------------------------------------------------------------------- FDSCreateMaps

UFDSCreateMapsCommandlet::UFDSCreateMapsCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UFDSCreateMapsCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    FString MapList = TEXT("/Game/Maps/Server/ServerDefaultMap");
    FParse::Value(*Params, TEXT("maps="), MapList);
    TArray<FString> Maps;
    MapList.ParseIntoArray(Maps, TEXT("+"), true);
    FString MapsFile; // one package name per line, e.g. Config/FDSVenueSublevels-Venue01.txt
    if (FParse::Value(*Params, TEXT("mapsfile="), MapsFile))
    {
        TArray<FString> Lines;
        FFileHelper::LoadFileToStringArray(Lines, *MapsFile);
        Maps.Reset();
        for (const FString& Line : Lines)
        {
            if (!Line.TrimStartAndEnd().IsEmpty())
            {
                Maps.Add(Line.TrimStartAndEnd());
            }
        }
    }
    int32 Failures = 0;
    for (const FString& PackageName : Maps)
    {
        const FString WorldName = FPackageName::GetShortName(PackageName);
        UPackage* Package = CreatePackage(nullptr, *PackageName);
        UWorld* World = UWorld::CreateWorld(EWorldType::Inactive, false, FName(*WorldName), Package);
        World->SetFlags(RF_Public | RF_Standalone);
        const FString File = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetMapPackageExtension());
        const bool bSaved = UPackage::SavePackage(Package, World, RF_Public | RF_Standalone, *File, GError, nullptr, false, true, SAVE_NoError);
        UE_LOG(LogFDS, Display, TEXT("FDSCreateMaps: %s (world %s, settings %s) -> %s: %s"), *PackageName, *World->GetName(),
            World->GetWorldSettings() ? *World->GetWorldSettings()->GetClass()->GetPathName() : TEXT("none"), *File, bSaved ? TEXT("saved") : TEXT("FAILED"));
        Failures += bSaved ? 0 : 1;
        World->DestroyWorld(false);
        World->RemoveFromRoot();
    }
    return Failures;
#else
    return 1;
#endif
}

// ---------------------------------------------------------------------------------------------------- FDSNetLayout

namespace
{
    FString Q(const FString& S)
    {
        return TEXT("\"") + S.ReplaceCharWithEscapedChar() + TEXT("\"");
    }

    FString QPath(const UObject* O)
    {
        return O ? Q(O->GetPathName()) : FString(TEXT("null"));
    }

    FString Hex32(uint32 V) { return FString::Printf(TEXT("\"0x%08X\""), V); }
    FString Hex64(uint64 V) { return FString::Printf(TEXT("\"0x%016llX\""), V); }

    struct FLayoutWriter
    {
        TSet<UStruct*> Structs;

        FString Prop(UProperty* P, int32 Depth = 0)
        {
            FString O = FString::Printf(TEXT("{\"name\":%s,\"cls\":%s,\"dim\":%d,\"esize\":%d,\"offset\":%d,\"flags\":%s,\"repIndex\":%d,\"cond\":%d"),
                *Q(P->GetName()), *Q(P->GetClass()->GetName()), P->ArrayDim, P->ElementSize, P->GetOffset_ForInternal(),
                *Hex64((uint64)P->PropertyFlags), P->RepIndex, (int32)P->GetBlueprintReplicationCondition());
            if (P->RepNotifyFunc != NAME_None)
            {
                O += TEXT(",\"repNotify\":") + Q(P->RepNotifyFunc.ToString());
            }
            auto Inner = [&](const TCHAR* Key, UProperty* I)
            {
                O += FString::Printf(TEXT(",\"%s\":"), Key) + (I && Depth < 4 ? Prop(I, Depth + 1) : FString(TEXT("null")));
            };
            if (UStructProperty* SP = Cast<UStructProperty>(P))
            {
                O += TEXT(",\"struct\":") + QPath(SP->Struct);
                Structs.Add(SP->Struct);
            }
            else if (UArrayProperty* AP = Cast<UArrayProperty>(P)) { Inner(TEXT("inner"), AP->Inner); }
            else if (USetProperty* SetP = Cast<USetProperty>(P)) { Inner(TEXT("element"), SetP->ElementProp); }
            else if (UMapProperty* MP = Cast<UMapProperty>(P)) { Inner(TEXT("key"), MP->KeyProp); Inner(TEXT("value"), MP->ValueProp); }
            else if (UClassProperty* CP = Cast<UClassProperty>(P)) { O += TEXT(",\"objClass\":") + QPath(CP->PropertyClass) + TEXT(",\"metaClass\":") + QPath(CP->MetaClass); }
            else if (USoftClassProperty* SCP = Cast<USoftClassProperty>(P)) { O += TEXT(",\"metaClass\":") + QPath(SCP->MetaClass); }
            else if (UObjectPropertyBase* OP = Cast<UObjectPropertyBase>(P)) { O += TEXT(",\"objClass\":") + QPath(OP->PropertyClass); }
            else if (UInterfaceProperty* IP = Cast<UInterfaceProperty>(P)) { O += TEXT(",\"interfaceClass\":") + QPath(IP->InterfaceClass); }
            else if (UByteProperty* BP = Cast<UByteProperty>(P)) { O += TEXT(",\"enum\":") + QPath(BP->Enum); }
            else if (UEnumProperty* EP = Cast<UEnumProperty>(P)) { O += TEXT(",\"enum\":") + QPath(EP->GetEnum()); Inner(TEXT("underlying"), EP->GetUnderlyingProperty()); }
            else if (UBoolProperty* BoolP = Cast<UBoolProperty>(P))
            {
                // Field size and the native-bool marker only (the probe's byte offset / mask are not exposed here).
                O += FString::Printf(TEXT(",\"bool\":[%d,0,0,%d]"), BoolP->ElementSize, BoolP->IsNativeBool() ? 255 : 1);
            }
            else if (UDelegateProperty* DP = Cast<UDelegateProperty>(P)) { O += TEXT(",\"signature\":") + QPath(DP->SignatureFunction); }
            else if (UMulticastDelegateProperty* MDP = Cast<UMulticastDelegateProperty>(P)) { O += TEXT(",\"signature\":") + QPath(MDP->SignatureFunction); }
            return O + TEXT("}");
        }

        FString Function(UFunction* F)
        {
            FString O = FString::Printf(TEXT("{\"kind\":\"function\",\"name\":%s,\"funcFlags\":%s,\"numParms\":%d,\"parmsSize\":%d,\"returnOffset\":%d,\"rpcId\":%d,\"rpcResponseId\":%d,\"params\":["),
                *Q(F->GetName()), *Hex32((uint32)F->FunctionFlags), F->NumParms, F->ParmsSize, F->ReturnValueOffset, F->RPCId, F->RPCResponseId);
            bool bFirst = true;
            for (TFieldIterator<UProperty> It(F); It; ++It)
            {
                if (It->HasAnyPropertyFlags(CPF_Parm))
                {
                    O += (bFirst ? TEXT("") : TEXT(",")) + Prop(*It);
                    bFirst = false;
                }
            }
            return O + TEXT("]}");
        }

        FString Class(UClass* C, FClassNetCacheMgr& NetCache)
        {
            C->SetUpRuntimeReplicationData();
            const uint32 Checksum = NetCache.GetClassNetCache(C)->GetClassChecksum();
            FString O = FString::Printf(TEXT("{\"path\":%s,\"super\":%s,\"classFlags\":%s,\"repSetUp\":true,\"numClassReps\":%d,\"numNetFields\":%d,\"netChecksum\":%u,\"reps\":["),
                *QPath(C), *QPath(C->GetSuperClass()), *Hex32((uint32)C->ClassFlags), C->ClassReps.Num(), C->NetFields.Num(), Checksum);
            bool bFirst = true;
            for (int32 i = 0; i < C->ClassReps.Num(); ++i)
            {
                const FRepRecord& R = C->ClassReps[i];
                if (R.Property->GetOuter() != C)
                {
                    continue;
                }
                O += FString::Printf(TEXT("%s{\"rep\":%d,\"arrayIndex\":%d,\"property\":%s}"), bFirst ? TEXT("") : TEXT(","), i, R.Index, *Prop(R.Property));
                bFirst = false;
            }
            O += TEXT("],\"netFields\":[");
            bFirst = true;
            for (UField* F : C->NetFields)
            {
                O += bFirst ? TEXT("") : TEXT(",");
                bFirst = false;
                if (UFunction* Fn = Cast<UFunction>(F))
                {
                    O += Function(Fn);
                }
                else
                {
                    O += FString::Printf(TEXT("{\"kind\":\"property\",\"name\":%s}"), *Q(F->GetName()));
                }
            }
            return O + TEXT("]}");
        }

        FString Struct(UStruct* S)
        {
            const UScriptStruct* SS = Cast<UScriptStruct>(S);
            FString O = FString::Printf(TEXT("{\"path\":%s,\"super\":%s,\"class\":%s,\"structFlags\":%s,\"size\":%d,\"props\":["),
                *QPath(S), *QPath(S->GetSuperStruct()), *QPath(S->GetClass()), *Hex32(SS ? (uint32)SS->StructFlags : 0), S->GetStructureSize());
            bool bFirst = true;
            for (TFieldIterator<UProperty> It(S, EFieldIteratorFlags::ExcludeSuper); It; ++It)
            {
                O += (bFirst ? TEXT("") : TEXT(",")) + Prop(*It);
                bFirst = false;
            }
            if (S->GetSuperStruct())
            {
                Structs.Add(S->GetSuperStruct());
            }
            return O + TEXT("]}");
        }
    };
}

UFDSNetLayoutCommandlet::UFDSNetLayoutCommandlet()
{
    IsClient = false;
    IsServer = true;
    IsEditor = true;
    LogToConsole = true;
}

int32 UFDSNetLayoutCommandlet::Main(const FString& Params)
{
    FString ListFile, OutFile;
    if (!FParse::Value(*Params, TEXT("classes="), ListFile) || !FParse::Value(*Params, TEXT("out="), OutFile))
    {
        UE_LOG(LogFDS, Error, TEXT("FDSNetLayout: -classes=<file> -out=<json> required"));
        return 1;
    }
    TArray<FString> Paths;
    FFileHelper::LoadFileToStringArray(Paths, *ListFile);
    TArray<UClass*> Classes;
    for (const FString& Path : Paths)
    {
        UClass* C = LoadObject<UClass>(nullptr, *Path.TrimStartAndEnd()); // loads blueprint classes too
        if (!C)
        {
            UE_LOG(LogFDS, Warning, TEXT("FDSNetLayout: class %s not found"), *Path);
            continue;
        }
        for (UClass* It = C; It; It = It->GetSuperClass())
        {
            Classes.AddUnique(It);
        }
    }
    FLayoutWriter W;
    FClassNetCacheMgr NetCache;
    FString O = TEXT("{\"probe\":\"fds-net-layout\",\"schema\":1,\"classes\":[");
    for (int32 i = 0; i < Classes.Num(); ++i)
    {
        O += (i ? TEXT(",") : TEXT("")) + W.Class(Classes[i], NetCache);
    }
    O += TEXT("],\"structs\":[");
    TSet<UStruct*> Written;
    bool bFirst = true;
    for (bool bMore = true; bMore;)
    {
        bMore = false;
        const TSet<UStruct*> Pending = W.Structs;
        for (UStruct* S : Pending)
        {
            if (Written.Contains(S))
            {
                continue;
            }
            Written.Add(S);
            O += (bFirst ? TEXT("") : TEXT(",")) + W.Struct(S);
            bFirst = false;
            bMore = true;
        }
    }
    O += TEXT("]}");
    const bool bSaved = FFileHelper::SaveStringToFile(O, *OutFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogFDS, Display, TEXT("FDSNetLayout: %d classes, %d structs -> %s (%s)"), Classes.Num(), Written.Num(), *OutFile, bSaved ? TEXT("saved") : TEXT("FAILED"));
    return bSaved ? 0 : 1;
}

// ---------------------------------------------------------------------------------------------------- FDSTokenSelfTest

UFDSTokenSelfTestCommandlet::UFDSTokenSelfTestCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UFDSTokenSelfTestCommandlet::Main(const FString& Params)
{
    FString VectorsFile, Text;
    TSharedPtr<FJsonObject> Vectors;
    if (!FParse::Value(*Params, TEXT("vectors="), VectorsFile) || !FFileHelper::LoadFileToString(Text, *VectorsFile) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Vectors) || !Vectors.IsValid())
    {
        UE_LOG(LogFDS, Error, TEXT("FDSTokenSelfTest: -vectors=<json> missing or unreadable"));
        return 2;
    }
    auto Hex = [](const FString& In, TArray<uint8>& Out)
    {
        Out.SetNumUninitialized(In.Len() / 2);
        return HexToBytes(In, Out.GetData()) == Out.Num();
    };
    int32 Passed = 0, Failed = 0;
    for (const TSharedPtr<FJsonValue>& Value : Vectors->GetArrayField(TEXT("raw")))
    {
        const TSharedPtr<FJsonObject> V = Value->AsObject();
        TArray<uint8> Key, Message, Signature;
        const FString MessageHex = V->GetStringField(TEXT("message_hex"));
        Hex(V->GetStringField(TEXT("public_key_hex")), Key);
        Hex(V->GetStringField(TEXT("signature_hex")), Signature);
        if (!MessageHex.IsEmpty())
        {
            Hex(MessageHex, Message);
        }
        const bool bExpected = V->GetBoolField(TEXT("valid"));
        const bool bGot = Key.Num() == 32 && Signature.Num() == 64 &&
            FDSSessionToken::VerifyEd25519(Key.GetData(), Message.GetData(), Message.Num(), Signature.GetData());
        (bGot == bExpected ? Passed : Failed)++;
        if (bGot != bExpected)
        {
            UE_LOG(LogFDS, Error, TEXT("FDSTokenSelfTest: raw vector (%d-byte message) expected %s, got %s"), Message.Num(),
                bExpected ? TEXT("valid") : TEXT("invalid"), bGot ? TEXT("valid") : TEXT("invalid"));
        }
    }
    const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
    for (const TSharedPtr<FJsonValue>& Value : Vectors->GetArrayField(TEXT("tokens")))
    {
        const TSharedPtr<FJsonObject> V = Value->AsObject();
        FDSSessionToken::SetPublicKey(V->GetStringField(TEXT("public_key")));
        FDSSessionToken::FClaims Claims;
        const FString Why = FDSSessionToken::Verify(V->GetStringField(TEXT("token")), Now, Claims);
        const bool bExpected = V->GetBoolField(TEXT("valid"));
        bool bOk = Why.IsEmpty() == bExpected;
        if (bOk && bExpected)
        {
            bOk = Claims.Slot == V->GetStringField(TEXT("slot")) && Claims.User == static_cast<int64>(V->GetNumberField(TEXT("uid"))) &&
                Claims.Expires == static_cast<int64>(V->GetNumberField(TEXT("exp")));
        }
        (bOk ? Passed : Failed)++;
        if (!bOk)
        {
            UE_LOG(LogFDS, Error, TEXT("FDSTokenSelfTest: token expected %s, got '%s'"), bExpected ? TEXT("valid") : TEXT("invalid"),
                Why.IsEmpty() ? TEXT("valid") : *Why);
        }
    }
    UE_LOG(LogFDS, Display, TEXT("FDSTokenSelfTest: %d passed, %d failed"), Passed, Failed);
    return Failed == 0 && Passed > 0 ? 0 : 1;
}
