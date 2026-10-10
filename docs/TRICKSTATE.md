# TrickState: what FDS needs to relay DJ tricks

Status: **not implemented.** FDS has no `UTrickState`. A client's `TrickState.ServerAddCompletedTrick` therefore has no
target on the server, nothing ever records a `TrickToDeck` (type 12) action, and `TrickResults` stays zero. These notes
record what the native code does, so the work can be done once the reflection generator and an EGS v1.4.2 build are
available to check it.

Sources:

- The UE4SS SDK dumps of the **Steam** build (CL 1317755): `fuser-decomp\sdk\UHTHeaderDump\Hype\Public\TrickState.h`,
  `HypePlayerState.h`, and `sdk\CXXHeaderDump\Hype.hpp`.
- The Ghidra export of the same build: `fuser-decomp\full\c\chunk_*.c`, and `ufunc_map.tsv` for the live UFunction
  addresses.
- FuserProject `SERVER_SPEC.md` §4.3 / §8 and `actions.lua:799-835`.

Every address below is from the Steam build. Where FDS records an EGS offset for the same field, it matches the Steam
one. For example, `_djPlayerState` is "FUSER offset 4264" (0x10A8) in `HypePlayerState.h` and 0x10A8 in the Steam
dump. The EGS layout is still unverified for `UTrickState` itself.

## Class

```cpp
// /Script/Hype.TrickState (parent /Script/Hype.HypeActorComponent); Size 0xC0
UCLASS(Blueprintable, ClassGroup=Custom, meta=(BlueprintSpawnableComponent))
class HYPE_API UTrickState : public UHypeActorComponent
{
    GENERATED_BODY()
public:
    UTrickState(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Reliable, Server, WithValidation)
    void ServerAddCompletedTrick(int32 Tick, const FTrickAndTiming& completedTrick);
};
```

- **No reflected properties.** `UHypeActorComponent` ends at 0xB0. TrickState's only extra field is a native,
  non-reflected `TArray<FTrickAndTiming>` at 0xB0 (data) / 0xB8 (num), which holds the tricks waiting for the local
  tick (see "Native behaviour").
- **One net function,** `ServerAddCompletedTrick`, with the RPC parameters `(int32 Tick, const FTrickAndTiming&
  completedTrick)` in that order.
  - Exec thunk 0x1408d6110 reads the int32, then the struct. It calls `_Validate` at vtable +0x3e0 and
    `_Implementation` at +0x3e8.
  - Validation failure is fatal (`ServerAddCompletedTrick_Validate` goes to the RPC-failure path).
- `FTrickAndTiming` already exists in FDS (`HypeNetTypes.h:811`). It is `{ ATrick* Trick; ETrickTiming myTiming; }`
  with a size of 16, and its NetSerialize is `HypeNetSerializers.cpp:236` (FUN_14089f560).
- Constructor 0x140697fb0: the standard component constructor. It sets the primary tick flags (byte +0x3a |= 0x0e:
  can ever tick, start enabled, allowed on a dedicated server) and zeroes the array. It does **not** set
  `bReplicates`.

## Subobject on the player state

The `AHypePlayerState` constructor creates four default subobjects in this order, using
`CreateDefaultSubobject(Outer, Name, Class, Class, bIsRequired=1, bIsTransient=0)` (FUN_140b66cd0):

| Name (exact) | Class | PS field | Offset |
|---|---|---|---|
| `"CurveballState"` | UCurveballState | `_curveballState` | 0x10B0 |
| `"TrickState"` | UTrickState | `_trickState` | 0x10B8 |
| `"ResultsState"` | UResultsState | `_resultsState` | 0x10C0 |
| `"BattleState"` | UBattlePlayerState | `_battleState` | 0x10C8 |

Evidence: `full\c\chunk_1406b0000.c:7704-7724`.

UHT specifiers for the field: `UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, meta=(AllowPrivateAccess=true))
UTrickState* _trickState;`. The field is **not replicated**, like `_resultsState`. There is also a
`UFUNCTION(BlueprintCallable, BlueprintPure) UTrickState* BPGetTrickState();`, which is not a net function.

FDS creates only `ResultsState` (`HypeGenerated.cpp:259`).

## What the generator has to add

These files are generated (see `Fuser/Source/Hype/generated-files.txt`), so they must not be edited by hand:

1. A new generated header `Hype/Public/TrickState.h` with the class above. Add it to `generated-files.txt`.
2. In `HypePlayerState.h`, a `UTrickState* _trickState;` field, declared like `_resultsState` (`// default subobject
   "TrickState"`, `UPROPERTY(VisibleAnywhere)`).
   - Optionally add `BPGetTrickState`. It is not network-visible, so it can be left out.
3. In `HypeGenerated.cpp`:
   - include `TrickState.h`;
   - add `UTrickState::UTrickState(const FObjectInitializer&) : Super(ObjectInitializer) {}`;
   - in the `AHypePlayerState` constructor, add `_trickState = CreateDefaultSubobject<UTrickState>(TEXT("TrickState"));`
     **before** `ResultsState`, to keep FUSER's order;
   - do not emit `GetLifetimeReplicatedProps` (TrickState has no replicated properties);
   - do not emit generated stub `_Implementation` / `_Validate` bodies for `ServerAddCompletedTrick` (the
     hand-written ones below replace them).

Check these against the EGS build before building:

- the EGS class's size and parent, and that `ServerAddCompletedTrick` is its only net function. The RPC's field index
  in the class's net cache has to match the client's;
- the parameter types and order;
- the subobject name `TrickState`.

The client resolves the component through the PlayerState by that stable name.

## Hand-written parts (not generated)

### `HypeServerRpcs.cpp`

This is the native `_Implementation` 0x1406a8330. It gets the owning `AHypePlayerState` (component +0xA0). FUN_1406d5230
appends a new item to its `_multiplayerActions`, and FUN_1406627e0 fills it in:

- `MasterTimelineTick = Tick`
- `ActionType = TrickToDeck (12)`
- `Trick = completedTrick` (16 bytes at item +0xD8)
- `UniqueId = the global counter`

The native body does not mark the item dirty (FuserProject's post-hook does). FDS's `OnRecordAction` assigns the
UniqueId, marks the item dirty and calls `ForceNetUpdate`, so the body is:

```cpp
void UTrickState::ServerAddCompletedTrick_Implementation(int32 Tick, const FTrickAndTiming& completedTrick)
{
    AHypePlayerState* Player = Cast<AHypePlayerState>(GetOwner());
    IHypeServerAuthority* Authority = Player ? IHypeServerAuthority::Get(this) : nullptr;
    if (!Authority)
    {
        return;
    }
    FMultiplayerActionItemEntry Item = MakeAction(EMultiplayerActionType::TrickToDeck, Tick); // this file's helper
    Item.Trick = completedTrick;
    Authority->OnRecordAction(Player, Item);
}
bool UTrickState::ServerAddCompletedTrick_Validate(int32 Tick, const FTrickAndTiming& completedTrick) { return true; }
```

`MakeAction(Type, Tick)` is the helper the other action RPCs in this file use. `OnRecordAction` already ignores TrickToDeck when it
folds the mix (`FoldIntoMix`).

### `FDSLobbyAuthority::OnPlayerJoined`

Replicate the component, as is already done for ResultsState:

```cpp
if (Player->_trickState) { Player->_trickState->SetIsReplicated(true); }
```

A server RPC on a component reaches the server only when the client knows the component's NetGUID. The native
constructor does not set `bReplicates`, and the place where FUSER turns replication on at run time was not found. FDS
already does the same for ResultsState, whose template does not replicate either.

## Open risks (check on EGS before turning this on)

1. **The trick object may arrive as null.** `FTrickAndTiming::Trick` points to the performing client's own trick actor.
   - `Trick__TrickPerformed` (0x1406a9b80) passes `this`.
   - The actors are spawned on demand by the game state's trick manager: GameState +0x2A0, FUN_1406a83e0, a per-class
     cache at +0x170, `SpawnActor` with default parameters.
   - So they are dynamic, non-replicated, client-local actors. The client cannot give such an object a NetGUID, so
     the server will most likely receive `Trick == nullptr`. FDS's placeholder resolver only handles objects that
     arrive by path.
2. **A null trick may crash listeners.** The listener's action executor, case 0xC (chunk_140650000.c:11290 ->
   FUN_1406a4040), appends the received `FTrickAndTiming` to its TrickState array and broadcasts it.
   `UTrickState::TickComponent` (vfunc +0x330, 0x1406a9920) then reads `*(Trick + 0x270)` without a null check.
   - Relaying a TrickToDeck with a null trick could therefore crash every stock listener.
   - Until a live EGS test shows the trick resolves, the RPC body should **drop** items whose `Trick` is null. Log and
     count them, and do not relay them.
   - FuserProject's "29 tricks" (`SERVER_SPEC.md:515`) ran on a listen-server room, which does not settle this.
3. **Trick results.** `TotalTricks` and `TrickTimingCounts[timing]` are counted by FUN_140687630 (ResultsState +0xC0
   / +0xD0) from `UTrickState::TickComponent`, and only on the PlayerState's authority (`Role == ROLE_Authority`).
   - The authority's TrickState array is filled only by the action executor and by `Trick__TrickPerformed`, and
     neither runs on a dedicated server.
   - So FUSER's own dedicated server probably never counted tricks either.
   - If FDS counts them in the RPC body (`++TrickTimingCounts[myTiming]`, `++TotalTricks`, `ResetResults` already
     sizes the table to 5), confirm first how the EGS results screen shows them.
   - Each player's TrickState array is cleared on Lobby entry (FUN_1406a83a0, chunk_140660000.c:11320).
4. **EGS verification.** Every address here is from the Steam build. Before building, confirm the class, the RPC
   signature and the subobject name in the EGS v1.4.2 reflection data that the generator reads.
