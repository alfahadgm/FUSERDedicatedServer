// Native struct serializers FUSER registers for its network structs.
//
// The fast arrays use the stock FFastArraySerializer template (FUSER's FUN_140612c40 / FUN_140868080 are instances of
// it; the FUSER client accepts FDS's arrays). Every per-item / per-struct body is implemented from its native
// function: the RPC-parameter structs (effects, mute/solo, instrument crafting; a DJ's RPC with an unreadable
// parameter closes the connection), the reliable and unreliable action items and TrickAndTiming.
#include "HypeNetTypes.h"
#include "AudioEffectData.h"
#include "InstrumentCelData.h"
#include "Trick.h"

bool FMultiplayerActionArray::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
    return FFastArraySerializer::FastArrayDeltaSerialize<FMultiplayerActionItemEntry, FMultiplayerActionArray>(ActionItems, DeltaParms, *this);
}

bool FMultiplayerUnreliableActionArray::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
    return FFastArraySerializer::FastArrayDeltaSerialize<FMultiplayerUnreliableActionItemEntry, FMultiplayerUnreliableActionArray>(UnreliableActionItems, DeltaParms, *this);
}

namespace
{
    // FUSER's archive helpers for the action item: single bytes for enums, the archive's 4-byte bool form.
    template <typename TEnum> void SerializeByte(FArchive& Ar, TEnum& Value)
    {
        uint8 Byte = static_cast<uint8>(Value);
        Ar.Serialize(&Byte, 1);
        Value = static_cast<TEnum>(Byte);
    }

    template <typename TObject> void SerializeObject(FArchive& Ar, TObject*& Value)
    {
        UObject* Object = Value;
        Ar << Object; // the archive's object reference (vtable +0x28): the package map's NetGUID form
        if (Ar.IsLoading())
        {
            Value = Cast<TObject>(Object);
        }
    }

    // FUN_14061c6c0 / FUN_1405ef8e0: int32 count, then each deck's two 4-byte bools.
    void SerializeDeckStates(FArchive& Ar, TArray<FDeckMuteSoloState>& States)
    {
        int32 Num = States.Num();
        Ar << Num;
        if (Ar.IsLoading())
        {
            if (Num < 0 || Num > 64)
            {
                Ar.SetError();
                return;
            }
            States.SetNum(Num);
        }
        for (FDeckMuteSoloState& State : States)
        {
            Ar << State.isMutePressed;
            Ar << State.isSoloPressed;
        }
    }
}

// FUN_140626eb0: ActionType byte, UniqueId and MasterTimelineTick int32, then only
// the fields of that action type. FFastArraySerializerItem's IDs are the fast array's own (not serialized here).
//   1 SpawnDisc: celShortName (FName), trackType byte, spawnTransform (FUN_14061c530: rotation, translation, scale),
//     actionAfterDiscSpawn byte, dropStartTick, dropEndTick, isBattleSuperDrop (4-byte bool)
//   2 EjectDisc, 3 HotSwapOnDeck, 5 EjectQueue: trackType byte        4 DoDropQueue: nothing
//   6 EffectSlotState: slot byte, SlotInfo (AudioEffectInfo)          7 GlobalAudioState: MasterBpm, key, mode bytes
//   8 SoloMuteStates: DeckStates                                      9 TrackGain: trackType byte, gain float
//   10 CurrentCraftedInstrument: InstSlot byte, InstrumentCelData     11 RequestComplete: uniqueRequestId, completionType
//   12 TrickToDeck: Trick (object, timing byte)
bool FMultiplayerActionItemEntry::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    SerializeByte(Ar, ActionType);
    Ar << UniqueId;
    Ar << MasterTimelineTick;
    switch (ActionType)
    {
    case EMultiplayerActionType::SpawnDisc:
        Ar << celShortName;
        SerializeByte(Ar, trackType);
        Ar << spawnTransform;
        SerializeByte(Ar, actionAfterDiscSpawn);
        Ar << dropStartTick;
        Ar << dropEndTick;
        Ar << isBattleSuperDrop;
        break;
    case EMultiplayerActionType::EjectDisc:
    case EMultiplayerActionType::HotSwapOnDeck:
    case EMultiplayerActionType::EjectQueue:
        SerializeByte(Ar, trackType);
        break;
    case EMultiplayerActionType::EffectSlotState:
    {
        SerializeByte(Ar, slot);
        bool bInfoSuccess = true;
        SlotInfo.NetSerialize(Ar, Map, bInfoSuccess);
        break;
    }
    case EMultiplayerActionType::GlobalAudioState:
        Ar << MasterBpm;
        SerializeByte(Ar, MasterKey);
        SerializeByte(Ar, MasterMode);
        break;
    case EMultiplayerActionType::SoloMuteStates:
        SerializeDeckStates(Ar, DeckStates);
        break;
    case EMultiplayerActionType::TrackGain:
        SerializeByte(Ar, trackType);
        Ar << gain;
        break;
    case EMultiplayerActionType::CurrentCraftedInstrument:
        SerializeByte(Ar, InstSlot);
        SerializeObject(Ar, InstrumentCelData);
        break;
    case EMultiplayerActionType::RequestComplete:
        Ar << uniqueRequestId;
        SerializeByte(Ar, completionType);
        break;
    case EMultiplayerActionType::TrickToDeck:
        SerializeObject(Ar, Trick.Trick);
        SerializeByte(Ar, Trick.myTiming);
        break;
    default:
        break;
    }
    bOutSuccess = !Ar.IsError();
    return true;
}

// Defined below with the RPC-parameter structs.
static void SerializeGridState(FArchive& Ar, FGridInstrumentCraftingState& State);

// FUN_140627410: ActionType byte, MasterTimelineTick int32, then
//   1 FXParam: FXSlot byte, paramX and paramY each as a byte of value x 255 (DAT_14299f0f8), restored as byte / 255
//     (DAT_1429b3ffc);
//   2 InstrumentParam: InstSlot byte, then InstState in GridInstrumentCraftingState's form (FUN_14085a050).
bool FMultiplayerUnreliableActionItemEntry::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    uint8 Type = static_cast<uint8>(ActionType);
    Ar.Serialize(&Type, 1);
    ActionType = static_cast<EMultiplayerUnreliableActionType>(Type);
    Ar << MasterTimelineTick;
    if (ActionType == EMultiplayerUnreliableActionType::FXParam)
    {
        uint8 Slot = static_cast<uint8>(FXSlot);
        Ar.Serialize(&Slot, 1);
        FXSlot = static_cast<EAudioEffectSlot>(Slot);
        uint8 X = 0;
        uint8 Y = 0;
        if (Ar.IsSaving())
        {
            X = static_cast<uint8>(static_cast<int32>(paramX * 255.0f));
            Y = static_cast<uint8>(static_cast<int32>(paramY * 255.0f));
        }
        Ar.Serialize(&X, 1);
        Ar.Serialize(&Y, 1);
        if (Ar.IsLoading())
        {
            paramX = X * (1.0f / 255.0f);
            paramY = Y * (1.0f / 255.0f);
        }
    }
    else if (ActionType == EMultiplayerUnreliableActionType::InstrumentParam)
    {
        uint8 Slot = static_cast<uint8>(InstSlot);
        Ar.Serialize(&Slot, 1);
        InstSlot = static_cast<EInstrumentCraftingSlot>(Slot);
        SerializeGridState(Ar, InstState);
    }
    bOutSuccess = true;
    return true;
}

// FUN_140626d00: the object reference (archive vtable +0x28), Params as a stock
// TArray<float> (FUN_14052adb0), the bool (4-byte form), AppliedToDecks as a stock TArray<bool> and the int32 owner.
bool FAudioEffectInfo::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    UObject* Effect = effectData;
    Ar << Effect;
    if (Ar.IsLoading())
    {
        effectData = Cast<UAudioEffectData>(Effect);
    }
    Ar << Params;
    Ar << IsEffectEnabled;
    Ar << AppliedToDecks;
    Ar << OwnedByHypeUserId;
    bOutSuccess = true;
    return true;
}

// FUN_1407fe480: the two bools in the archive's 4-byte bool form.
bool FDeckMuteSoloState::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    Ar << isMutePressed;
    Ar << isSoloPressed;
    bOutSuccess = true;
    return true;
}

// FUN_14085a050: one flags byte (bit 0 PrimaryActionOn, bit 1 SecondaryActionOn, bit 2 mode != 0), then GridPos X
// and Y truncated to a byte each. Loading restores mode as 0/1 and the positions as whole numbers, as FUSER does.
static void SerializeGridState(FArchive& Ar, FGridInstrumentCraftingState& State)
{
    uint8 Flags = 0;
    uint8 X = 0;
    uint8 Y = 0;
    if (Ar.IsSaving())
    {
        Flags = (State.PrimaryActionOn ? 1 : 0) | (State.SecondaryActionOn ? 2 : 0) | (static_cast<uint8>(State.mode) != 0 ? 4 : 0);
        X = static_cast<uint8>(static_cast<int32>(State.GridPos.X));
        Y = static_cast<uint8>(static_cast<int32>(State.GridPos.Y));
    }
    Ar.Serialize(&Flags, 1);
    Ar.Serialize(&X, 1);
    Ar.Serialize(&Y, 1);
    if (Ar.IsLoading())
    {
        State.PrimaryActionOn = (Flags & 1) != 0;
        State.SecondaryActionOn = (Flags & 2) != 0;
        State.mode = static_cast<EGridInstrumentMode>((Flags >> 2) & 1);
        State.GridPos = FVector2D(X, Y);
    }
}

bool FGridInstrumentCraftingState::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    SerializeGridState(Ar, *this);
    bOutSuccess = true;
    return true;
}

// FUN_14089f560: the trick object, then the timing byte.
bool FTrickAndTiming::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    SerializeObject(Ar, Trick);
    SerializeByte(Ar, myTiming);
    bOutSuccess = true;
    return true;
}
