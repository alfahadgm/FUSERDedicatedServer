# FDS (FuserDedicatedServer) image, built from the distribution tarball.
#
# Build context: this Dockerfile, entrypoint.sh, fds-server-linux.tar.gz (the packaged Linux server) and
# MANIFEST.json. Copy it to the Docker host and run, in it:
#   docker build -t fds-server:<version> .
# (<version> is in MANIFEST.json and VERSION.)
#
# The image holds only FDS: its code, the reconstructed /Script/Hype types and its own empty worlds. A FUSER
# install, when used, is bind-mounted read-only at /fuser by the host; nothing from it is copied here.
FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --create-home --uid 1000 --shell /usr/sbin/nologin fds

ARG FDS_TARBALL=fds-server-linux.tar.gz
# A local .tar.gz is unpacked by ADD; the tarball already carries the modes (binary 0755) and root ownership.
ADD ${FDS_TARBALL} /opt/fds/
COPY entrypoint.sh /usr/local/bin/fds-entrypoint

# UE4 writes Saved/ (logs, crash reports) under the project; everything else stays read-only for the server user.
RUN chmod 755 /usr/local/bin/fds-entrypoint \
    && mkdir -p /opt/fds/Fuser/Saved \
    && chown fds:fds /opt/fds/Fuser/Saved

USER fds
WORKDIR /opt/fds
ENV FDS_PORT=7777 \
    FDS_FUSER_ROOT=/fuser

# Game port and the session-browser beacon (game port + 100). With --network host the actual ports follow FDS_PORT.
EXPOSE 7777/udp 7877/udp
ENTRYPOINT ["/usr/local/bin/fds-entrypoint"]
