# Development, test and packaging image for tdepackagekit.
FROM debian:trixie

ENV DEBIAN_FRONTEND=noninteractive

# Trinity's mirror redirects to plain HTTP. The keyring anchors apt's trust in
# the Trinity repository, so its hash is pinned here.
ARG TDE_MIRROR=http://mirror.ppa.trinitydesktop.org/trinity/deb
ARG TDE_KEYRING_SHA256=ed7b8b5378a769dd66e554e5ffa9e750f548b93be02e6d23dbc3824462e75cd2

RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates curl \
 && curl -fsSL "${TDE_MIRROR}/trinity-keyring.deb" -o /tmp/trinity-keyring.deb \
 && echo "${TDE_KEYRING_SHA256}  /tmp/trinity-keyring.deb" | sha256sum -c - \
 && dpkg -i /tmp/trinity-keyring.deb \
 && rm /tmp/trinity-keyring.deb \
 && echo "deb ${TDE_MIRROR}/trinity-r14.1.x trixie main deps" > /etc/apt/sources.list.d/tde.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build pkg-config gettext \
      devscripts debhelper dpkg-dev apt-utils fakeroot lintian \
      tde-cmake tdelibs14-trinity-dev libtqt3-mt-dev tqt3-dev-tools \
      libdbus-1-dev libdbus-1-tqt-dev libpackagekit-glib2-dev \
      packagekit packagekit-tools dbus polkitd \
      python3 sudo procps \
 && rm -rf /var/cache/apt/archives/*.deb

# Test tools are a separate layer, so changing them doesn't rebuild the one above.
RUN apt-get install -y --no-install-recommends \
      xvfb xdotool imagemagick x11-utils xauth x11-xserver-utils fonts-dejavu-core \
      tdebase-trinity-bin twin-trinity python3-dbus python3-gi gir1.2-glib-2.0 \
      desktop-file-utils \
 && rm -rf /var/cache/apt/archives/*.deb

# The build/test user. Its UID is matched to the host user by ./dk so that files
# written to the bind-mounted source stay owned by the developer.
ARG DEV_UID=1000
RUN useradd -m -u "${DEV_UID}" -s /bin/bash dev \
 && echo 'dev ALL=(root) NOPASSWD: /src/docker/test-services.sh, /usr/bin/dpkg, /usr/bin/apt-get' > /etc/sudoers.d/dev \
 && chmod 0440 /etc/sudoers.d/dev \
 && mkdir -p /build && chown dev:dev /build \
 && mkdir -p -m 1777 /tmp/.ICE-unix /tmp/.X11-unix

ENV PATH=/opt/trinity/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
USER dev
WORKDIR /src
