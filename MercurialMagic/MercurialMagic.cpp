#include <MercurialMagic.hpp>

unique_pointer<Program> program;

#include "convert.cpp"

Program::Program(string_vector args) {
  program = this;
  Application::onMain({&Program::main, this});

  exportMethod = ExportMethod::GamePak;

  layout.setMargin(5);

  setTitle("Mercurial Magic");
  setSize(layout.minimumSize());
  setResizable(false);

  packLabel.setText("MSU-1 Pack path:");
  packChange.setText("Change ...");
  packChange.onActivate([&] {
    packPath.setText(BrowserDialog()
    .setTitle("Load MSU-1 Pack")
    .setPath(Path::program())
    .setFilters(string{"MSU-1 Pack|*.msu1"})
    .openFile());
    bool valid = validatePack();
    exportButton.setEnabled(valid);
  });

  romLabel.setText("ROM path:");
  romChange.setText("Change ...");
  romChange.onActivate([&] {
    romPath.setText(BrowserDialog()
    .setTitle("Load Super Famicom ROM")
    .setPath(Path::real(packPath.text()))
    .setFilters(string{"Super Famicom ROM|*.sfc"})
    .openFile());
    bool valid = validateROMPatch();
    exportButton.setEnabled(valid);
  });

  selectLabel.setText("Export as...");

  ((RadioLabel*)&exportGroup.objects()[exportMethod])->setChecked();

  gamepak.setText("Game Pak (cartridge folder)");
  gamepak.onActivate([&] {
    exportMethod = Program::ExportMethod::GamePak;
    manifest.setEnabled(true);
  });

  manifest.setText("Export manifest");
  manifest.onToggle([&] { exportManifest = manifest.checked(); });

  sd2snes.setText("SD2SNES/Snes9x");
  sd2snes.onActivate([&] {
    exportMethod = Program::ExportMethod::SD2SNES;
    manifest.setEnabled(false);
  });

  exportButton.setText("Export");
  exportButton.onActivate([&] {
    setEnabled(false);
    exitButton.setText("Cancel");
    beginExport();
  });

  exitButton.setText("Exit");
  exitButton.onActivate([&] { quit(); });

  onClose([&] { quit(); });

  args.takeLeft();  //ignore program location in argument parsing

  bool valid = false;
  if(args) {
    packPath.setText(args.takeLeft());
    valid = validatePack();
  }

  if(usesPatch && args) {
    romPath.setText(args.takeLeft());
    valid = validateROMPatch();
  }

  exportButton.setEnabled(valid);

  setVisible(true);
}

auto Program::validatePack() -> bool {
  pack.open(packPath.text());

  if(!fetch("msu1.rom")) return false;
  if(!fetch("program.rom") && !fetch("patch.bps")) return false;

  usesPatch = fetch(patch);
  romLabel.setText(usesPatch ? "ROM path:" : "No ROM needed");
  romPath.setEnabled(usesPatch);
  romChange.setEnabled(usesPatch);
  if(!usesPatch) romPath.setText("");

  return !usesPatch || validateROMPatch();
}

auto Program::validateROMPatch() -> bool {
  if(!romPath.text()) return false;

  if(usesPatch) patch.source(romPath.text());

  return true;
}

auto Program::fetch(string_view name) -> maybe<Decode::ZIP::File> {
  for(auto& file : pack.file) {
    if(file.name.match(name)) return file;
  }
  return nothing;
}

auto Program::fetch(bpspatch& patch) -> bool {
  if(auto file = fetch("patch.bps")) {
    patchContents = pack.extract(file());
    patch.modify(patchContents.data(), patchContents.size());
    return true;
  }
  return false;
}

auto Program::beginExport() -> void {
  zipIndex = 0;
  setProgress(0);

  uint patchResult;

  switch(exportMethod) {

  case ExportMethod::GamePak: {
    destination = {Location::dir(packPath.text()), Location::prefix(packPath.text()), ".sfc", "/"};

    directory::create(destination);

    if(usesPatch) {
      patch.target({destination, "program.rom"});
      patchResult = patch.apply();
    }

    break;
  }

  case ExportMethod::SD2SNES: {
    destination = {Location::dir(packPath.text()), "SD2SNES-Snes9x/"};

    directory::create(destination);

    if(usesPatch) {
      patch.target({destination, Location::prefix(packPath.text()), ".sfc"});
      patchResult = patch.apply();
    }

    static string_vector roms = {
      "program.rom",
      "data.rom",
      "slot-*.rom",
      "*.boot.rom",
      "*.program.rom",
      "*.data.rom",
    };

    file rom({destination, Location::prefix(packPath.text()), ".sfc"}, file::mode::write);
    for(string& romName : roms) {
      if(auto file = fetch(romName)) {
        rom.write(pack.extract(file()).data(), file().size);
      }
    }
    rom.close();

    break;
  }

  }

  if(usesPatch) {
    switch(patchResult) {
    case bpspatch::result::unknown:
      error("There was an unspecified problem in applying BPS patch.");
      break;
    case bpspatch::result::patch_invalid_header:
      error("The BPS patch's header is invalid!");
      break;
    case bpspatch::result::target_too_small:
    case bpspatch::result::target_checksum_invalid:
    case bpspatch::result::patch_too_small:
    case bpspatch::result::patch_checksum_invalid:
      error("The BPS patch is corrupt!");
      break;
    case bpspatch::result::source_too_small:
      error({
        "This ROM is too small!\n",
        "Check that you are selecting the correct ROM, and try again."
      });
      break;
    case bpspatch::result::source_checksum_invalid:
      warning({
        "The ROM's checksum does not match the one in the patch.\n",
        "If you are applying multiple patches to a ROM, this is normal.\n",
        "Otherwise, make sure you select the correct ROM."
      });
      break;
    }
  }

  thread::create([&](uintptr_t) -> void {
    while(zipIndex < pack.file.size()) {
      if(!iterateExport()) break;
    }
    finishExport();
  });
}

auto Program::iterateExport() -> bool {
  auto& file = pack.file[zipIndex++];
  information({"Exporting ", file.name, "..."});

  string ext = Location::suffix(file.name);
  string path;
  uint trackID;
  if(ext == ".pcm"
  || ext == ".wav"
  || ext == ".ogg"
  || ext == ".flac"
  || ext == ".mp3") {
    uint start = 0;
    uint length = 0;
    for(uint pos : range(0, file.name.size())) {
      if(file.name[pos] >= '0' && file.name[pos] <= '9') { start = pos; break; }
    }
    for(uint pos : range(start, file.name.size())) {
      if(file.name[pos] < '0' || file.name[pos] > '9') { length = pos - start; break; }
    }
    trackID = slice(file.name, start, length).natural();
  }

  switch(exportMethod) {

  case ExportMethod::GamePak: {
    if(ext == ".pcm"
    || ext == ".wav"
    || ext == ".ogg"
    || ext == ".flac"
    || ext == ".mp3") {
      path = {destination, "track-", trackID, ext};
    } else {
      path = {destination, file.name};
    }
    break;
  }

  case ExportMethod::SD2SNES: {
    string ext = Location::suffix(file.name);
    if(ext == ".pcm"
    || ext == ".wav"
    || ext == ".ogg"
    || ext == ".flac"
    || ext == ".mp3") {
      path = {destination, Location::prefix(packPath.text()), "-", trackID, ext};
    } else if(file.name == "msu1.rom") {
      path = {destination, Location::prefix(packPath.text()), ".msu"};
    } else if(file.name == "program.rom") {
      path = {destination, Location::prefix(packPath.text()), ".sfc"};
    }

    break;
  }

  }

  file::write(path, pack.extract(file));

  bool result = true;
  if(ext == ".wav"
  || ext == ".ogg"
  || ext == ".flac"
  || ext == ".mp3") result = convert(path);

  if(!result) {
    if(ext == ".wav") {
      string response = error({
        "Could not convert ", file.name, " to PCM!\n"
        "Please check if wav2msu is installed correctly.\n"
        #if defined(PLATFORM_WINDOWS)
        "Would you like to download wav2msu from SMW Central?"
        #else
        "Would you like to visit wav2msu's GitHub page?"
        #endif
      }, {"Yes", "No"});
      if(response == "Yes") {
        #if defined(PLATFORM_WINDOWS)
        invoke("https://www.smwcentral.net/?p=section&a=details&id=4872");
        #else
        invoke("https://github.com/jbaiter/wav2msu");
        #endif
      }
    } else if(ext == ".ogg") {
      error({
        "Could not convert ", file.name, " to PCM!\n"
        "Ogg Vorbis is not currently supported."
      });
    } else if(ext == ".flac") {
      error({
        "Could not convert ", file.name, " to PCM!\n"
        "FLAC is not currently supported."
      });
    } else if(ext == ".mp3") {
      error({
        "Could not convert ", file.name, " to PCM!\n"
        "MP3 is not currently supported."
      });
    }
    return false;
  }

  setProgress(zipIndex);
  return true;
}

auto Program::finishExport() -> void {
  if(exportManifest) {
    switch(exportMethod) {

    case ExportMethod::GamePak: {
      if(auto manifest = execute("icarus", "--manifest", destination)) {
        string legacyOutput = "";
        if(auto legacyManifest = execute("daedalus", "--manifest", destination)) {
          legacyOutput = {legacyManifest.output.split("\n\n")[0], "\n\n"};
        }
        file::write({destination, "manifest.bml"}, {legacyOutput, manifest.output});
      }

      break;
    }

    }
  }

  information("MSU-1 pack exported!");
  progressBar.setPosition(0);

  setEnabled(true);
  exitButton.setText("Exit");
}

auto Program::information(const string& text) -> void {
  statusLabel.setText(text);
}

auto Program::warning(const string& text) -> void {
  MessageDialog().setTitle("Mercurial Magic").setText(text).warning();
}

auto Program::error(const string& text) -> void {
  MessageDialog().setTitle("Mercurial Magic").setText(text).error();
  thread::exit();
}

auto Program::error(const string& text, const string_vector& buttons) -> string {
  return MessageDialog().setTitle("Mercurial Magic").setText(text).error(buttons);
}

auto Program::setProgress(uint files) -> void {
  progressBar.setPosition(files * 100 / pack.file.size());
}

auto Program::setEnabled(bool enabled) -> void {
  packPath.setEnabled(enabled);
  packChange.setEnabled(enabled);
  romPath.setEnabled(enabled);
  romChange.setEnabled(enabled);
  gamepak.setEnabled(enabled);
  manifest.setEnabled(enabled);
  sd2snes.setEnabled(enabled);
  exportButton.setEnabled(enabled);
}

auto Program::main() -> void {
  usleep(2000);
}

auto Program::quit() -> void {
  setVisible(false);
  Application::quit();
}

#include <nall/main.hpp>
auto nall::main(string_vector args) -> void {
  Application::setName("Mercurial Magic");
  new Program(args);
  Application::run();
}
