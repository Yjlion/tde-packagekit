# Building and packaging

Development happens in a Debian 13 container with Trinity 14.1 and PackageKit. You only need Docker,
and a few GB of free disk space for the image.

## Commands

| Command | What it does |
|---|---|
| `./dk image` | Build the Docker image (`build` and the others do this if it's missing) |
| `./dk build` | Compile into the `tdepackagekit-build` Docker volume |
| `./dk test` | Run every test suite. See [testing](testing.md). |
| `./dk screenshot` | Regenerate the PNGs in `screenshots/` |
| `./dk deb` | Build the Debian package into `dist/` |
| `./dk verify-deb` | Install `dist/*.deb` in a throwaway container and check it |
| `./dk lint` | Check documentation links, desktop files, scripts and the GitHub workflow |
| `./dk shell` | Open a shell in the container, with the source at `/src` |
| `./dk clean` | Delete the build volume |

Containers run as an unprivileged user with your user ID, so files written to the source tree stay
yours. They don't need `--privileged`. Inside the test container that user can run the service
script, `dpkg` and `apt-get` through `sudo`, which the tests need.

## Building without Docker

On a Debian system with the [Trinity repository](https://wiki.trinitydesktop.org/DebianInstall):

```sh
sudo apt install cmake ninja-build pkg-config tde-cmake tdelibs14-trinity-dev \
    libtqt3-mt-dev tqt3-dev-tools libdbus-1-dev libdbus-1-tqt-dev
PATH=/opt/trinity/bin:$PATH cmake -S . -B build -G Ninja \
    -DCMAKE_INSTALL_PREFIX=/opt/trinity -DBUILD_TESTING=OFF
cmake --build build
sudo cmake --install build
```

With `-DBUILD_TESTING=ON` you also need `libpackagekit-glib2-dev`. The unit tests use its header
to check enum values; the application itself doesn't link against it.

## Documentation

`doc/en/index.docbook` is the handbook. The build turns it into `index.cache.bz2` with `meinproc`
(from `tdelibs14-trinity`), and installs it with screenshots from `screenshots/` to
`share/doc/tde/HTML/en/tdepackagekit/`. The man pages in `doc/man/` are plain groff, installed to
`share/man/man1/`. `./dk lint` checks both. The Markdown files in `doc/` are for GitHub and aren't
installed.

## Runtime requirements

- Trinity 14.1
- `packagekit`, with a working backend (apt on Debian)
- `polkit-agent-tde`, or another polkit agent, so that users can authenticate
- Optional: `apt-config-icons` or `apt-config-icons-large`, for application icons

## Continuous integration

`.github/workflows/build.yml` runs on every push and pull request. It builds the Debian package and
uploads it as a workflow artifact, on the run's page under **Artifacts**. Those runs have read-only
access to the repository.

Publishing a release (or pre-release) on GitHub runs it too, and then attaches the `.deb` to the
release. Only that last step, in its own job, can write to the repository.

It doesn't use Docker, which is only for development. `.github/build-deb.sh` creates a Debian 13
chroot with `debootstrap`, adds the Trinity repository, installs what `debian/control` asks for, and
runs `dpkg-buildpackage`. It doesn't run the tests, so run `./dk test` before pushing.

## Making a release

The version is written in a few places. Keep them the same as the release tag:

1. `project( tdepackagekit VERSION x.y.z ...)` in `CMakeLists.txt`, shown in the About dialog
2. A new entry at the top of `debian/changelog` (`dch -v x.y.z` adds one), which names the `.deb`
3. The `.TH` lines of the man pages in `doc/man/`, and `<releaseinfo>` in `doc/en/index.docbook`

Merge that, then publish a release on GitHub with the tag `vx.y.z`. The `.deb` is attached to it a
few minutes later. The release build stops with an error if the tag doesn't match the first two.

## The application icon

`data/make-icons.sh` draws the PNGs in `data/` with ImageMagick. They are committed, so you only
need to run it (`./dk shell`, then `data/make-icons.sh`) to change the design.
