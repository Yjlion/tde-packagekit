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

`.github/workflows/build.yml` builds the Debian package on every push and pull request, and keeps it
as a workflow artifact, on the run's page under **Artifacts**. These runs have read-only access to
the repository.

It doesn't use Docker, which is only for development. `.github/build-deb.sh` creates a Debian 13
chroot with `debootstrap`, adds the Trinity repository, installs what `debian/control` asks for, and
runs `dpkg-buildpackage`. It doesn't run the tests, so run `./dk test` before pushing.

## Making a release

Tag a version, `v` followed by the version number, such as `v0.0.5`. Either:

- On GitHub, **Draft a new release**, type the new tag, and press **Publish release** (or tick
  **Set as a pre-release** first). Publishing creates the tag, which starts the build. A few minutes
  later `tdepackagekit-trinity_0.0.5_amd64.deb` is attached to the release.
- Or push the tag with git: `git tag v0.0.5 && git push origin v0.0.5`. The build creates a
  **draft** release with the package attached and generated notes. Edit it, then publish it.

Saving a release as a draft doesn't create its tag, so nothing is built until it is published.

Nothing needs bumping by hand. `.github/set-version.sh` adds a `debian/changelog` entry for the tag's
version, and `debian/rules` passes that version to the build, so the package and the About dialogs
show it. A tag that isn't a version, such as `vnext`, fails the build. The version in
`CMakeLists.txt` is only used for development builds.

The release job, a separate job that runs only for tags, is the only part of the workflow that can
write to the repository.

## The application icon

`data/make-icons.sh` draws the PNGs in `data/` with ImageMagick. They are committed, so you only
need to run it (`./dk shell`, then `data/make-icons.sh`) to change the design.
