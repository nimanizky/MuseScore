# Sandikala Windows distribution

Target: Intel/AMD x64 Windows 10 and 11. Windows compatibility and audio must be tested on both systems before distribution.

## Application installer

Commit the current application source and the packaging files to a distribution branch. Do not commit soundfonts. Local framework changes are carried in `framework.patch` so the pinned upstream submodule does not need to be pushed to its upstream remote. Refresh this patch with `git -C muse diff --binary HEAD > buildscripts/packaging/Windows/Sandikala/framework.patch` if framework changes are made.

Push the distribution branch, open GitHub Actions, choose **Build Sandikala Windows installer**, and run it on that branch. Download the `Sandikala-Windows-x64-installer` artifact. This build is unsigned and does not contain the locally stored soundfonts.

## Sound pack

Copy the local `soundfont` directory to a Windows packaging machine. Install Inno Setup 6, then compile:

```powershell
& 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe' '/DSoundfontDir=C:\path\to\soundfont' buildscripts\packaging\Windows\Sandikala\SoundPack.iss
```

Distribute the sound-pack EXE and **all** its BIN slices together. Install the application MSI first, then install the sound pack into the same application directory. If the application install directory was changed, choose that same directory in the sound-pack installer. Samples are copied without audio conversion or quality loss.

## Verification before delivery

On a clean Windows installation, install both packages. Open a Kepatihan score, confirm each custom TSI selects its bundled soundfont, play it, export WAV, and reopen the saved score. Repeat on Windows 10 and 11. Confirm the program does not require the original Mac, an external drive, Qt, or source directories. Check the installed sound folder against the source soundfont filenames.

These scripts prepare packaging; a successful Windows runner build and installation tests are still required to produce and validate the distributable.
