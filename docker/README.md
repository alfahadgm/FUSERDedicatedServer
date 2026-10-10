# FDS (FuserDedicatedServer) container

This folder, with the packaged Linux server added, is a complete Docker build context:

| File | |
|---|---|
| `Dockerfile` | debian bookworm-slim, a non-root `fds` user (uid 1000), the server under `/opt/fds` |
| `entrypoint.sh` | turns the container's environment into server arguments and execs the server (PID 1) |
| `fds-server-linux.tar.gz` | the Linux server: no FUSER content, no debug symbols |
| `MANIFEST.json`, `VERSION` | build version, engine 4.24.3, network version 10402, per-file SHA-256 |

## Build

Copy the folder to an x86-64 Linux Docker host, then in it:

```sh
docker build -t fds-server:$(cat VERSION) -t fds-server:latest .
```

## Run

```sh
docker run --rm --name fds --network host \
  -e FDS_PORT=7777 \
  fds-server:latest
```

- Clients join the host's address on UDP `FDS_PORT`; the session-browser beacon uses `FDS_PORT + 100`.
- `--network host` matches how fuserd runs its containers. With bridge networking, publish both ports as UDP.
- `docker stop fds` sends SIGTERM and the server shuts down cleanly. Logs go to stdout (`docker logs -f fds`).

### With a FUSER install (optional)

Mount an EGS v1.4.2 install read-only at `/fuser`, meaning the folder that holds `Fuser/Content/Paks`, and give FUSER's pak key through the environment:

```sh
docker run --rm --name fds --network host \
  -e FDS_PORT=7777 -e FDS_PAK_KEY \
  --mount type=bind,src=/path/to/FUSER,dst=/fuser,readonly \
  fds-server:latest
```

- `-e FDS_PAK_KEY` with no value takes the key from the calling shell's environment, keeping it off the command line.
- With the install, `-e FDS_VENUE=VenueC` (`VenueA`–`VenueF`) picks the venue. FDS logs the catalogue it read (`FDS: venue VenueC: map /Game/Maps/Venue03/Venue03, …`) and `FDS: session venue …`.
- Without the install, only Venue01 works: FDS has no stubs for the other venues' streamed sublevels, so clients would be disconnected.
- FDS mounts the install's `pakchunk*-WindowsNoEditor.pak` below its own content and logs `FDS: FUSER install at /fuser: N of N pak(s) mounted`.
- Without the mount or the key it runs on its own content and logs why.

## Environment

| Variable | Default | |
|---|---|---|
| `FDS_PORT` | `7777` | game port |
| `FDS_FUSER_ROOT` | `/fuser` | install root inside the container |
| `FDS_PAK_KEY` | (none) | FUSER's AES pak key, 64 hex digits or base64; never logged |
| `FDS_VENUE` | `Venue01` | the session's venue: an ID from the install's catalogue (`VenueA`–`VenueF`), a map name (`Venue03`) or a map package. IDs need the install |
| `FDS_TIME_OF_DAY` | (client's own) | `morning`, `day`, `sunset` or `night`, recorded for fuserd and the session browser |
| `FDS_EXTRA_ARGS` | (none) | extra server arguments (development) |
| `FDS_ROUNDS`, `FDS_TURN_MEASURES` | Game.ini (1 × 32) | fallback set length (1-3 rounds × 32/64/128 bars; other values are ignored). The creator's own choice replaces it in the Lobby; a public session keeps these rounds and takes only the chosen turn length (`PlayerChosenSetLength` in Game.ini) |
| `FDS_SERVER_ACCESS` | `private` | `public` or `private`, as the lobby reports it |
| `FDS_REGISTER_URL`, `FDS_SLOT_ID`, `FDS_REGISTER_TOKEN` | (none) | set by fuserd. FDS registers with fuserd's `/fds/v1` API, sends a status heartbeat and reports its exit. The token is never logged. Unset (manual runs): no registration |
| `FDS_HEARTBEAT_SECONDS` | `10` | status interval (fuserd sets it) |
| `FDS_TOKEN_PUBLIC_KEY` | (none) | fuserd's Ed25519 session-token key (base64url), set by fuserd. With it, every client must log in with a fuserd token for this `FDS_SLOT_ID`. Refused logins get `invalid_player_session`, and the reason is logged as `FDS: login refused: …` (the token never is). Unset: anyone may join |
| `FDS_REQUIRE_TOKEN` | (from key) | `0` admits anyone even with a key; `1` without a usable key refuses everyone |
| `FDS_ADDRESS` | (none) | the public address reported in the session-browser beacon reply (fuserd sets it) |
| `FDS_EXIT_WHEN_EMPTY` | on | with registration, the server exits when its last player leaves; `0` keeps it running. Never on the Main Stage, except for an orphaned server (below) |
| `FDS_MAX_UPTIME_HOURS` | `24` | in every mode (Main Stage, public, private), once the server has run this long its status reports `recycle_pending: true` (fuserd places no new players on it) and it exits once it is empty and in the Lobby. `0` never recycles |

### fuserd registration

- **Ports.** The register body reports the UDP ports the game and beacon net drivers actually bound (`port`, `beacon_port`; `0` when not listening). UE's `IpNetDriver` moves to the next free port when the requested one is taken (`MaxPortCountToTry`, left at the engine default), so a server whose port was taken binds elsewhere and logs `FDS: bound UDP … not the assigned …` (a port of `0` logs `FDS: not listening on …` as an error); fuserd then stops it at once and puts both ports in a cooldown, instead of waiting out its startup timeout.
- **Orphaned containers.** A container fuserd no longer knows (left over from an earlier fuserd run) gets HTTP 401 on every call. After 30 s of consecutive 401s, or 10 minutes with no reply from fuserd at all, it exits at once when no player is connected; otherwise it exits when its last player leaves (also on the Main Stage). A replied call that is not a 401 ends a run of 401s, and a later 200 cancels the exit-when-empty. A server fuserd still answers is never treated as orphaned, even idle and never joined: fuserd keeps ready servers waiting.
- **Blocked custom songs.** Each register / status reply carries `blocked_songs`, the custom songs (lowercase shortnames) not every player in the session can load. FDS replaces its list on every reply that has the field and does not relay a `SpawnDisc` whose cel is `<short>(_)?(bt|bs|ld|lp)(_trans)?` for one of them (case-insensitive, longest shortname first), nor puts such a disc in a late joiner's state-of-the-world snapshot; each is logged as `FDS: player N disc … not relayed`. `BlockUnreadyCustomSongs=False` in `[/Script/FuserServer.FDSLobby]` turns this off. An older fuserd sends no list and nothing is blocked.

Arguments after the image name are passed to the server, for example `-ini:Game:[/Script/FuserServer.FDSLobby]:ResultsXPPayout=True`.
