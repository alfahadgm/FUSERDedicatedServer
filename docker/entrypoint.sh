#!/bin/sh
# FDS container entrypoint. Turns the container's environment into server arguments and execs
# the server, so it is PID 1 and receives docker stop's SIGTERM directly (FDS shuts down cleanly on it).
#
#   FDS_PORT       game port (default 7777); the beacon uses FDS_PORT + 100
#   FDS_EXTRA_ARGS extra server arguments, split on whitespace (development only)
# Any arguments given to `docker run <image> ...` are passed through to the server after these.
#
# Secrets (the fuserd registration token, the pak key) are read by the server from its environment and must never be
# put on its command line: UE4 logs the command line at start-up.
set -eu

port="${FDS_PORT:-7777}"
case "$port" in
    ''|*[!0-9]*) echo "fds-entrypoint: FDS_PORT must be a number, got '$port'" >&2; exit 64 ;;
esac

# shellcheck disable=SC2086 # FDS_EXTRA_ARGS is split on purpose
exec /opt/fds/Fuser/Binaries/Linux/FuserServerServer Fuser \
    -port="$port" -log -unattended \
    ${FDS_EXTRA_ARGS:-} "$@"
