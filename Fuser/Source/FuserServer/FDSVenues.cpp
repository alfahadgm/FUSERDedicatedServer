#include "FDSVenues.h"
#include "FDSInstallContent.h"
#include "FuserServer.h"
#include "GameMapsSettings.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/ArchiveProxy.h"
#include "Serialization/MemoryReader.h"
#include "UObject/ObjectResource.h"
#include "UObject/PackageFileSummary.h"

namespace
{
    TArray<FDSVenues::FVenue> GCatalog;
    FDSVenues::FSelection GSelected;
    const TCHAR* TimesOfDay[] = {TEXT("morning"), TEXT("day"), TEXT("sunset"), TEXT("night")}; // EVenueTimeOfDay order

    // FNames in the package are indices into its name map.
    class FNameMapReader : public FArchiveProxy
    {
    public:
        FNameMapReader(FArchive& Inner, const TArray<FName>& InNames) : FArchiveProxy(Inner), Names(InNames) {}
        virtual FArchive& operator<<(FName& Value) override
        {
            int32 Index = 0;
            int32 Number = 0;
            InnerArchive << Index << Number;
            Value = Names.IsValidIndex(Index) ? FName(Names[Index], Number) : NAME_None;
            if (!Names.IsValidIndex(Index))
            {
                SetError();
            }
            return *this;
        }
        using FArchiveProxy::operator<<;

        // The proxy's own error flag, or the memory reader's (reading past the end leaves it set and returns zeros).
        bool Failed() const { return IsError() || InnerArchive.IsError(); }

    private:
        const TArray<FName>& Names;
    };

    // UE4.24's FPropertyTag on disk (PropertyTag.cpp, all the versions FUSER's cooks have).
    struct FTag
    {
        FName Name, Type, InnerType, ValueType;
        int32 Size = 0;
    };
    bool ReadTag(FNameMapReader& Ar, FTag& Tag)
    {
        Ar << Tag.Name;
        if (Tag.Name.IsNone() || Ar.Failed())
        {
            return false;
        }
        int32 ArrayIndex = 0;
        Ar << Tag.Type << Tag.Size << ArrayIndex;
        static const FName Struct(TEXT("StructProperty")), Bool(TEXT("BoolProperty")), Byte(TEXT("ByteProperty")), Enum(TEXT("EnumProperty")),
            Array(TEXT("ArrayProperty")), Set(TEXT("SetProperty")), Map(TEXT("MapProperty"));
        if (Tag.Type == Struct)
        {
            FName StructName;
            FGuid StructGuid;
            Ar << StructName << StructGuid;
        }
        else if (Tag.Type == Bool)
        {
            uint8 BoolVal = 0;
            Ar << BoolVal;
        }
        else if (Tag.Type == Byte || Tag.Type == Enum)
        {
            FName EnumName;
            Ar << EnumName;
        }
        else if (Tag.Type == Array || Tag.Type == Set)
        {
            Ar << Tag.InnerType;
        }
        else if (Tag.Type == Map)
        {
            Ar << Tag.InnerType << Tag.ValueType;
        }
        uint8 HasPropertyGuid = 0;
        Ar << HasPropertyGuid;
        if (HasPropertyGuid)
        {
            FGuid PropertyGuid;
            Ar << PropertyGuid;
        }
        return !Ar.Failed();
    }

    // FSoftObjectPath: asset path name, sub-path. Returns the package part (/Game/Maps/Venue01/Venue01).
    FString ReadSoftPackage(FNameMapReader& Ar)
    {
        FName AssetPathName;
        FString SubPath;
        Ar << AssetPathName << SubPath;
        FString Path = AssetPathName.ToString();
        int32 Dot = INDEX_NONE;
        if (Path.FindChar(TEXT('.'), Dot))
        {
            Path = Path.Left(Dot);
        }
        return Path;
    }

    // A VenueData package: MapFile and LightingSublevels of its asset export.
    bool ReadVenue(const FString& UAssetFile, FDSVenues::FVenue& Out, FString& OutWhy)
    {
        TArray<uint8> Bytes, Exp;
        if (!FFileHelper::LoadFileToArray(Bytes, *UAssetFile) || !FFileHelper::LoadFileToArray(Exp, *FPaths::ChangeExtension(UAssetFile, TEXT("uexp"))))
        {
            OutWhy = TEXT("cannot read .uasset/.uexp");
            return false;
        }
        Bytes.Append(Exp); // export offsets count from the start of the .uasset
        FMemoryReader Raw(Bytes);
        FPackageFileSummary Summary;
        Raw << Summary;
        if (Raw.IsError() || Summary.Tag != PACKAGE_FILE_TAG)
        {
            OutWhy = TEXT("not a package");
            return false;
        }
        Raw.SetUE4Ver(Summary.GetFileVersionUE4());
        Raw.SetLicenseeUE4Ver(Summary.GetFileVersionLicenseeUE4());
        Raw.SetEngineVer(Summary.SavedByEngineVersion);
        Raw.SetCustomVersions(Summary.GetCustomVersionContainer());
        TArray<FName> Names;
        Raw.Seek(Summary.NameOffset);
        for (int32 i = 0; i < Summary.NameCount && !Raw.IsError(); ++i)
        {
            FNameEntrySerialized Entry(ENAME_LinkerConstructor);
            Raw << Entry;
            Names.Add(FName(Entry));
        }
        FNameMapReader Ar(Raw, Names);
        Ar.Seek(Summary.ExportOffset);
        const FString AssetName = FPaths::GetBaseFilename(UAssetFile);
        FObjectExport Asset;
        bool bFound = false;
        for (int32 i = 0; i < Summary.ExportCount && !Ar.Failed(); ++i)
        {
            FObjectExport Export;
            Ar << Export;
            if (Export.ObjectName.ToString() == AssetName)
            {
                Asset = Export;
                bFound = true;
            }
        }
        if (!bFound || Ar.Failed() || Asset.SerialOffset <= 0 || Asset.SerialOffset + Asset.SerialSize > Bytes.Num())
        {
            OutWhy = TEXT("no asset export");
            return false;
        }
        static const FName MapFile(TEXT("MapFile")), LightingSublevels(TEXT("LightingSublevels"));
        Ar.Seek(Asset.SerialOffset);
        FTag Tag;
        while (ReadTag(Ar, Tag))
        {
            const int64 ValueStart = Ar.Tell();
            if (Tag.Name == MapFile)
            {
                Out.Map = ReadSoftPackage(Ar);
            }
            else if (Tag.Name == LightingSublevels)
            {
                int32 NumRemoved = 0;
                Ar << NumRemoved;
                for (int32 i = 0; i < NumRemoved && !Ar.Failed(); ++i)
                {
                    FName Key;
                    Ar << Key;
                }
                int32 Num = 0;
                Ar << Num;
                for (int32 i = 0; i < Num && !Ar.Failed(); ++i)
                {
                    FName Key; // EVenueTimeOfDay::Day
                    Ar << Key;
                    FString Time = Key.ToString();
                    int32 Colon = INDEX_NONE;
                    if (Time.FindLastChar(TEXT(':'), Colon))
                    {
                        Time = Time.RightChop(Colon + 1);
                    }
                    Out.Lighting.Add(Time.ToLower(), ReadSoftPackage(Ar));
                }
            }
            Ar.Seek(ValueStart + Tag.Size);
            if (Ar.Failed())
            {
                break;
            }
        }
        Out.Id = AssetName;
        if (Ar.Failed() || Out.Map.IsEmpty())
        {
            OutWhy = Ar.Failed() ? TEXT("unreadable properties") : TEXT("no MapFile");
            return false;
        }
        return true;
    }

    void ReadCatalog()
    {
        GCatalog.Reset();
        if (!FDSInstallContent::IsMounted())
        {
            return;
        }
        const FString Dir = FPaths::ProjectContentDir() / TEXT("Gameplay/Venues");
        TArray<FString> Files;
        IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.uasset")), true, false);
        Files.Sort();
        for (const FString& File : Files)
        {
            FDSVenues::FVenue Venue;
            FString Why;
            if (!ReadVenue(Dir / File, Venue, Why))
            {
                UE_LOG(LogFDS, Warning, TEXT("FDS: venue data %s unreadable: %s"), *File, *Why);
                continue;
            }
            TArray<FString> Times;
            Venue.Lighting.GetKeys(Times);
            UE_LOG(LogFDS, Display, TEXT("FDS: venue %s: map %s%s, time of day [%s]"), *Venue.Id, *Venue.Map,
                FDSInstallContent::SourceOf(Venue.Map) == FDSInstallContent::ESource::Own ? TEXT("") : TEXT(" (no FDS stub world)"), *FString::Join(Times, TEXT(", ")));
            GCatalog.Add(MoveTemp(Venue));
        }
    }

    FString Setting(const TCHAR* Flag, const TCHAR* Env)
    {
        FString Value = FPlatformMisc::GetEnvironmentVariable(Env);
        FParse::Value(FCommandLine::Get(), Flag, Value);
        return Value.TrimStartAndEnd();
    }
}

void FDSVenues::Select()
{
    ReadCatalog();
    FString Wanted = Setting(TEXT("FDSVenue="), TEXT("FDS_VENUE"));
    if (Wanted.IsEmpty())
    {
        FParse::Value(FCommandLine::Get(), TEXT("server_venue="), Wanted); // FUSER's own dedicated-server option
    }
    GSelected = FSelection();
    GSelected.Map = UGameMapsSettings::GetGameDefaultMap(); // ServerDefaultMap on a dedicated server
    if (!Wanted.IsEmpty())
    {
        // A catalogue ID, a map name or a map package.
        const FVenue* Venue = GCatalog.FindByPredicate([&Wanted](const FVenue& V)
        {
            return V.Id.Equals(Wanted, ESearchCase::IgnoreCase) || V.Map.Equals(Wanted, ESearchCase::IgnoreCase) ||
                FPackageName::GetShortName(V.Map).Equals(Wanted, ESearchCase::IgnoreCase);
        });
        FString Map = Venue ? Venue->Map : Wanted.StartsWith(TEXT("/Game/")) ? Wanted : FString::Printf(TEXT("/Game/Maps/%s/%s"), *Wanted, *Wanted);
        if (FDSInstallContent::SourceOf(Map) != FDSInstallContent::ESource::Own)
        {
            UE_LOG(LogFDS, Error, TEXT("FDS: venue '%s' -> %s has no FDS stub world (Config/FDSVenueMaps.txt%s); staying on %s"), *Wanted, *Map,
                GCatalog.Num() == 0 ? TEXT("; venue IDs need the FUSER install's catalogue") : TEXT(""), *GSelected.Map);
        }
        else
        {
            GSelected.Map = Map;
            GSelected.Id = Venue ? Venue->Id : FString();
            const FString Short = FPackageName::GetShortName(Map);
            UGameMapsSettings::SetGameDefaultMap(FString::Printf(TEXT("%s.%s"), *Map, *Short)); // ServerDefaultMap on a dedicated server
        }
    }
    if (GSelected.Id.IsEmpty())
    {
        if (const FVenue* Venue = GCatalog.FindByPredicate([](const FVenue& V) { return V.Map == GSelected.Map; }))
        {
            GSelected.Id = Venue->Id;
        }
    }
    const FString Time = Setting(TEXT("FDSTimeOfDay="), TEXT("FDS_TIME_OF_DAY")).ToLower();
    if (!Time.IsEmpty())
    {
        bool bKnown = false;
        for (const TCHAR* Each : TimesOfDay)
        {
            bKnown |= Time == Each;
        }
        if (bKnown)
        {
            GSelected.TimeOfDay = Time;
        }
        else
        {
            UE_LOG(LogFDS, Error, TEXT("FDS: time of day '%s' ignored (morning, day, sunset or night)"), *Time);
        }
    }
    const FVenue* Venue = GCatalog.FindByPredicate([](const FVenue& V) { return V.Id == GSelected.Id; });
    const FString* Lighting = Venue && !GSelected.TimeOfDay.IsEmpty() ? Venue->Lighting.Find(GSelected.TimeOfDay) : nullptr;
    UE_LOG(LogFDS, Display, TEXT("FDS: session venue %s, world %s, time of day %s%s"), GSelected.Id.IsEmpty() ? TEXT("(not in a catalogue)") : *GSelected.Id,
        *GSelected.Map, GSelected.TimeOfDay.IsEmpty() ? TEXT("(client's own)") : *GSelected.TimeOfDay,
        Lighting ? *FString::Printf(TEXT(" (lighting %s)"), **Lighting) : TEXT(""));
    // Without the install, only venues whose streamed sublevels FDS stubs itself (Venue01) keep clients connected:
    // stock UE4.24 closes a connection that reports a visible level the server can't find.
    if (!FDSInstallContent::IsMounted() && !GSelected.Map.StartsWith(TEXT("/Game/Maps/Venue01/")))
    {
        UE_LOG(LogFDS, Warning, TEXT("FDS: %s without the FUSER install: FDS has no stubs for its sublevels, clients will be disconnected"), *GSelected.Map);
    }
}

const TArray<FDSVenues::FVenue>& FDSVenues::Catalog()
{
    return GCatalog;
}

const FDSVenues::FSelection& FDSVenues::Selected()
{
    return GSelected;
}
