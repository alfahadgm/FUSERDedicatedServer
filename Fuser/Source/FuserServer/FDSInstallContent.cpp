#include "FDSInstallContent.h"
#include "FuserServer.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "IPlatformFilePak.h"
#include "Misc/AES.h"
#include "Misc/Base64.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace
{
    const TCHAR* Section = TEXT("/Script/FuserServer.FDSInstall");
    constexpr uint32 PakMagic = 0x5A6F12E1;

    FPakPlatformFile* GPakPlatformFile = nullptr;
    TSet<FString> GInstallPaks;      // FPakFile::GetFilename() of each mounted install pak
    TArray<FString> GAllowLoad;      // package path prefixes FDS may load from the install
    TSet<FString> GRefusedLogged;
    FAES::FAESKey GKey;

    bool DecodeKey(const FString& Text, FAES::FAESKey& OutKey)
    {
        FString Value = Text.TrimStartAndEnd();
        if (Value.StartsWith(TEXT("0x"), ESearchCase::IgnoreCase))
        {
            Value = Value.RightChop(2);
        }
        TArray<uint8> Bytes;
        if (Value.Len() == 2 * FAES::FAESKey::KeySize)
        {
            Bytes.SetNumZeroed(FAES::FAESKey::KeySize);
            for (int32 i = 0; i < Value.Len(); ++i)
            {
                if (!FChar::IsHexDigit(Value[i]))
                {
                    Bytes.Reset();
                    break;
                }
                Bytes[i / 2] = static_cast<uint8>((Bytes[i / 2] << 4) | FParse::HexDigit(Value[i]));
            }
        }
        if (Bytes.Num() != FAES::FAESKey::KeySize && !(FBase64::Decode(Value, Bytes) && Bytes.Num() == FAES::FAESKey::KeySize))
        {
            return false;
        }
        FMemory::Memcpy(OutKey.Key, Bytes.GetData(), FAES::FAESKey::KeySize);
        return true;
    }

    // Before mounting: an encrypted index that doesn't decrypt is fatal in UE4.24's pak reader. Decrypt the index's
    // first block and check it starts with the mount point FUSER's paks carry (FString "../../../").
    bool KeyOpensIndex(const FString& PakPath, const FAES::FAESKey& Key, FString& OutWhy)
    {
        TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*PakPath));
        if (!Reader)
        {
            OutWhy = TEXT("cannot open");
            return false;
        }
        const int64 Size = Reader->TotalSize();
        const int32 TailSize = static_cast<int32>(FMath::Min<int64>(Size, 512));
        TArray<uint8> Tail;
        Tail.SetNumUninitialized(TailSize);
        Reader->Seek(Size - TailSize);
        Reader->Serialize(Tail.GetData(), TailSize);
        int32 MagicAt = INDEX_NONE;
        for (int32 i = TailSize - 4; i >= 0; --i)
        {
            if (FMemory::Memcmp(&Tail[i], &PakMagic, 4) == 0)
            {
                MagicAt = i;
                break;
            }
        }
        if (MagicAt == INDEX_NONE || MagicAt < 1 || MagicAt + 24 > TailSize)
        {
            OutWhy = TEXT("no pak footer");
            return false;
        }
        const bool bEncryptedIndex = Tail[MagicAt - 1] != 0;
        int64 IndexOffset = 0;
        FMemory::Memcpy(&IndexOffset, &Tail[MagicAt + 8], sizeof(IndexOffset));
        if (!bEncryptedIndex)
        {
            return true;
        }
        if (IndexOffset < 0 || IndexOffset + FAES::AESBlockSize > Size)
        {
            OutWhy = TEXT("bad index offset");
            return false;
        }
        uint8 Block[FAES::AESBlockSize];
        Reader->Seek(IndexOffset);
        Reader->Serialize(Block, sizeof(Block));
        FAES::DecryptData(Block, sizeof(Block), Key);
        int32 MountLen = 0;
        FMemory::Memcpy(&MountLen, Block, sizeof(MountLen));
        if (MountLen < 4 || MountLen > 1024 || FMemory::Memcmp(Block + 4, "../", 3) != 0)
        {
            OutWhy = TEXT("FDS_PAK_KEY does not decrypt its index");
            return false;
        }
        return true;
    }
}

void FDSInstallContent::Mount()
{
    FString Root = FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_FUSER_ROOT"));
    FParse::Value(FCommandLine::Get(), TEXT("FDSFuserRoot="), Root);
    GConfig->GetArray(Section, TEXT("AllowLoad"), GAllowLoad, GGameIni);
    if (Root.IsEmpty())
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: no FUSER install configured (FDS_FUSER_ROOT): FDS content and placeholders only"));
        return;
    }
    const FString PakDir = FPaths::Combine(Root, TEXT("Fuser/Content/Paks"));
    if (!IFileManager::Get().DirectoryExists(*PakDir))
    {
        UE_LOG(LogFDS, Display, TEXT("FDS: no FUSER install at %s (no Fuser/Content/Paks): FDS content and placeholders only"), *Root);
        return;
    }
    GPakPlatformFile = static_cast<FPakPlatformFile*>(FPlatformFileManager::Get().FindPlatformFile(FPakPlatformFile::GetTypeName()));
    if (!GPakPlatformFile)
    {
        UE_LOG(LogFDS, Warning, TEXT("FDS: FUSER install at %s not mounted: no pak file layer (not a packaged server)"), *Root);
        return;
    }
    if (!DecodeKey(FPlatformMisc::GetEnvironmentVariable(TEXT("FDS_PAK_KEY")), GKey))
    {
        UE_LOG(LogFDS, Warning, TEXT("FDS: FUSER install at %s not mounted: FDS_PAK_KEY missing or not a 32-byte key (64 hex digits or base64)"), *Root);
        return;
    }
    FString Pattern = TEXT("pakchunk*-WindowsNoEditor.pak");
    GConfig->GetString(Section, TEXT("PakPattern"), Pattern, GGameIni);
    TArray<FString> Names;
    IFileManager::Get().FindFiles(Names, *FPaths::Combine(PakDir, Pattern), true, false);
    Names.Sort();
    if (Names.Num() == 0)
    {
        UE_LOG(LogFDS, Warning, TEXT("FDS: FUSER install at %s not mounted: no %s in Fuser/Content/Paks"), *Root, *Pattern);
        return;
    }
    // FUSER's paks carry the zero key GUID: the reader asks for the default key through this delegate.
    if (!FCoreDelegates::GetPakEncryptionKeyDelegate().IsBound())
    {
        FCoreDelegates::GetPakEncryptionKeyDelegate().BindLambda([](uint8 OutKey[32]) { FMemory::Memcpy(OutKey, GKey.Key, FAES::FAESKey::KeySize); });
    }
    int32 Mounted = 0;
    for (const FString& Name : Names)
    {
        const FString Path = FPaths::Combine(PakDir, Name);
        FString Why;
        if (!KeyOpensIndex(Path, GKey, Why))
        {
            UE_LOG(LogFDS, Warning, TEXT("FDS: install pak %s skipped: %s"), *Name, *Why);
            continue;
        }
        // Order 0: below FDS's own pak (4, FPakPlatformFile::GetPakOrderFromPakFilePath), so FDS's files always win.
        if (GPakPlatformFile->Mount(*Path, 0))
        {
            GInstallPaks.Add(Path);
            ++Mounted;
        }
        else
        {
            UE_LOG(LogFDS, Warning, TEXT("FDS: install pak %s failed to mount"), *Name);
        }
    }
    UE_LOG(LogFDS, Display, TEXT("FDS: FUSER install at %s: %d of %d pak(s) mounted read-only below FDS's own; %d load allowlist prefix(es)"), *Root, Mounted,
        Names.Num(), GAllowLoad.Num());
    if (Mounted > 0)
    {
        // Self-check: FDS's own default map must still come from FDS, and a FUSER-only package from the install.
        auto Name = [](ESource Source) { return Source == ESource::Own ? TEXT("own") : Source == ESource::Install ? TEXT("install") : TEXT("none"); };
        FString DefaultMap = TEXT("/Game/Maps/Venue01/Venue01");
        GConfig->GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("ServerDefaultMap"), DefaultMap, GEngineIni);
        const TCHAR* InstallProbe = TEXT("/Game/Gameplay/Venues/VenueA");
        UE_LOG(LogFDS, Display, TEXT("FDS: install self-check: %s -> %s (expected own), %s -> %s (expected install)"), *DefaultMap, Name(SourceOf(DefaultMap)),
            InstallProbe, Name(SourceOf(InstallProbe)));
    }
}

bool FDSInstallContent::IsMounted()
{
    return GInstallPaks.Num() > 0;
}

FDSInstallContent::ESource FDSInstallContent::SourceOf(const FString& LongPackageName)
{
    FString Filename;
    if (!FPackageName::DoesPackageExist(LongPackageName, nullptr, &Filename))
    {
        return ESource::None;
    }
    if (!IsMounted())
    {
        return ESource::Own;
    }
    FPakFile* Pak = nullptr;
    if (GPakPlatformFile->FindFileInPakFiles(*Filename, &Pak) && Pak && GInstallPaks.Contains(Pak->GetFilename()))
    {
        return ESource::Install;
    }
    return ESource::Own;
}

bool FDSInstallContent::IsLoadAllowed(const FString& LongPackageName)
{
    for (const FString& Prefix : GAllowLoad)
    {
        if (LongPackageName.StartsWith(Prefix))
        {
            return true;
        }
    }
    if (!GRefusedLogged.Contains(LongPackageName))
    {
        GRefusedLogged.Add(LongPackageName);
        UE_LOG(LogFDS, Display, TEXT("FDS: install package %s not on the load allowlist: placeholder"), *LongPackageName);
    }
    return false;
}
