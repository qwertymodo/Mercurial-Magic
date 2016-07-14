Mercurial Magic v00r05
Author: hex_usr
Dependencies:
  nall (by byuu)
  hiro (by byuu)
  icarus (by byuu)
  daedalus (based on icarus by byuu, modified by hex_usr)
License: ISC

Mercurial Magic is an MSU-1 package manager that can export MSU-1 games to the
following environments:
higan v095
higan v096-v100 and up
SD2SNES

Each item in that list takes a different, incompatible format for MSU-1 games,
which is why Mercurial Magic exists.

===============================================================================
MSU-1 Pack specification

MSU-1 packages are ZIP archives with the extension .msu1. These ZIP archives
are only allowed to use 2 compression methods: Store (0x00) and Deflate (0x08).
This effectively means that no MSU-1 package can exceed 4GB in size, and no
individual file's uncompressed size can exceed 4GB either. The restriction is
enforced by nall.

They can follow 2 separate structures depending on the legality of the original
game:

MSU-1 Pack for Commercial Game.msu1
  patch.bps         Patch
  msu1.rom          Data track
  track-1.pcm       Audio tracks
  track-2.pcm
  track-3.pcm...

MSU-1 Pack for Homebrew Game.msu1
  program.rom       Game Program
  data.rom          (optional) SPC7110 Data
  dsp1.program.rom  (optional) Coprocessor firmware
  dsp1.data.rom
  msu1.rom          Data track
  track-1.pcm       Audio tracks
  track-2.pcm
  track-3.pcm...

Do not create packs with the latter format if they are for commercial games.
Doing so constitutes illegal distribution of copyrighted material.

===============================================================================
Export formats

There are 2 major export formats to fit different needs.

Game Pak:
  The cartridge folder format for higan versions v096-v100 and up, with an
  optional manifest for higan v095. All files except for patch.bps are exported
  exactly as they are. If patch.bps is present, the patch will be applied to a
  ROM, then the patched ROM will become "program.rom" in the Game Pak.
  The generated manifest, if any, is a hybrid manifest for higan versions v095
  and v096-v100 and up.
  Exporting manifests requires icarus to be in the same directory as Mercurial
  Magic. To generate a manifest for higan v095, daedalus is also required.

SD2SNES
  A format in which all files share the name of the game's title, as required
  by SD2SNES. All files except for patch.bps are renamed during export. If
  patch.bps is present, the patch will be applied to a ROM, then the patched
  ROM will become "<Game Name>.sfc" in the package. If this is a homebrew, and
  more than 1 ROM (such as SPC7110 data or coprocessor firmware) are bundled
  directly, they will be merged together into a single .sfc file. No manifest
  is exported.