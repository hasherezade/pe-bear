# Update check

Tells the user when a newer PE-bear release exists. That is the whole feature:
nothing is downloaded and nothing is installed by PE-bear itself. The dialog
offers to open the release page in the browser, where the packages are.

## What it does

1. Five seconds after the main window is shown -- never before, so startup is
   unaffected and a machine without connectivity notices nothing -- asks
   `https://api.github.com/repos/<owner>/<name>/releases/latest` for the
   latest stable release, at most once every 24 hours. *Help → Check for
   Updates...* asks right away.
2. Rejects drafts, prereleases, prerelease-looking tags (`-rc1`, `+build`),
   malformed JSON, responses over 2 MiB, redirects to anything but a known
   GitHub host, and any TLS error. Nothing of these ever interrupts the user:
   an automatic check that fails is silent.
3. Compares the release version with the running build's. Newer: the dialog
   opens with both versions and a button for the release page. Not newer:
   "up to date", shown only when the user asked.
4. *Skip this version* silences that one version for automatic checks; a
   later release is offered again, and a manual check always answers.

## What it sends and where

One request, over HTTPS with certificate validation, to `api.github.com`. The
`User-Agent` is `PE-bear/<version>`; nothing else about the user, the machine
or the files being analysed is sent. The only URL ever opened in the browser
is the release's `html_url` as returned by the API, and only if it points at
`github.com`.

## Layout

| File | Role |
|---|---|
| `Version.*` | strict numeric release versions; the number itself lives only in `rebear_ver_short.h` |
| `ReleaseClient.*` | the bounded GitHub Releases API client |
| `UpdateSettings.*` | `AutoCheck`, `CheckIntervalHours`, `LastCheck`, `SkippedVersion` in the `Updates` group of the existing `QSettings` |
| `UpdateChecker.*` | compare and decide; takes any `IReleaseSource`, so it is tested without a network |
| `UpdateTypes.*` | states, errors and their messages, the release structs |
| `gui/UpdateDialog.*`, `gui/UpdateCoordinator.*` | the one dialog and the once-a-day timer; the only QtWidgets code |
| `tests/` | `tst_version`, `tst_releaseclient` (fixtures, no network), `tst_updatesettings`, `tst_updatechecker`, and a check that the core library links no QtWidgets |

`pebear_update_core` links Qt Core and Qt Network only; the GUI part is
compiled into the PE-bear target.

## Build options

| Option | Default | Meaning |
|---|---|---|
| `PEBEAR_ENABLE_UPDATER` | `ON` | build the update check at all (forced off on Qt4) |
| `PEBEAR_BUILD_UPDATER_TESTS` | `OFF` | build and register the unit tests (needs Qt Test) |
| `PEBEAR_UPDATE_REPOSITORY` | *(empty)* | GitHub repository to check, `owner/name`; empty means `hasherezade/pe-bear` |
