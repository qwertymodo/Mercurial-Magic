#include <MercurialMagic.hpp>

unique_pointer<Program> program;

#include "convert.cpp"

Program::Program(string_vector args) {
  program = this;
  Application::onMain({&Program::main, this});

  exportMethod = ExportMethod::GamePak;

  icarus = execute("icarus", "--name").output.strip() == "icarus";
  daedalus = execute("daedalus", "--name").output.strip() == "daedalus";

  layout.setMargin(5);
  setTitle("Mercurial Magic");
  setSize(layout.minimumSize());
  setResizable(false);

  packLabel.setText("MSU1 Pack path:");
  packPath.onChange([&] {
    outputName.setText(Location::prefix(packPath.text().transform("\\", "/")));
    valid = validatePack();
    exportButton.setEnabled(valid && outputName.text());
  });
  packChange.setText("Change ...").onActivate([&] {
    packPath.setText(BrowserDialog()
    .setTitle("Load MSU1 Pack")
    .setPath(Path::program())
    .setFilters(string{"MSU1 Pack|*.msu1"})
    .openFile());
    packPath.doChange();
  });

  romLabel.setText("ROM path:");
  romPath.onChange([&] {
    valid = validateROMPatch();
    exportButton.setEnabled(valid && outputName.text());
  });
  romChange.setText("Change ...").onActivate([&] {
    romPath.setText(BrowserDialog()
    .setTitle("Load Super Famicom ROM")
    .setPath(Path::real(packPath.text()))
    #if defined(PLATFORM_WINDOWS)
    .setFilters(string{"Super Famicom ROM|*.sfc:*.smc:*.SFC:*.SMC"})
    #else
    .setFilters(string{"Super Famicom ROM|*.sfc:*.smc"})
    #endif
    .openFile());
    romPath.doChange();
  });

  outputLabel.setText("Output name:");
  outputName.onChange([&] {
    exportButton.setEnabled(valid && outputName.text());
  });
  outputExtLabel.setText(".sfc/");

  selectLabel.setText("Export as...");

  ((RadioLabel*)&exportGroup.objects()[exportMethod])->setChecked();

  gamepak.setText("Game Pak (cartridge folder)").onActivate([&] {
    exportMethod = Program::ExportMethod::GamePak;
    manifest.setEnabled(icarus || daedalus);
    outputExtLabel.setText(".sfc/");
  });

  manifest.setEnabled(icarus || daedalus).setChecked(daedalus && !icarus);
  manifest.onToggle([&] {
    exportManifest = manifest.checked();
  });
  if(manifest.enabled()) {
    string higanMin = daedalus ? "v094" : "v096";
    string higanMax = icarus   ? "v104" : "v095";
    manifest.setText({"Export ", higanMin, "-", higanMax, " manifest"});
  } else {
    manifest.setText({"icarus and daedalus not found"});
  }

  sd2snes.setText("SD2SNES/Snes9x").onActivate([&] {
    exportMethod = Program::ExportMethod::SD2SNES;
    manifest.setEnabled(false);
    outputExtLabel.setText(".sfc");
  });

  exportButton.setText("Export").onActivate([&] {
    setEnabled(false);
    beginExport();
  });

  exitButton.setText("Exit").onActivate({&Program::quit, this});

  onClose({&Program::quit, this});

  args.takeLeft();  //ignore program location in argument parsing

  valid = false;
  if(args) {
    packPath.setText(args.takeLeft());
    valid = validatePack();
  }

  if(patch && args) {
    romPath.setText(args.takeLeft());
    valid = validateROMPatch();
  }

  exportButton.setEnabled(valid);

  setVisible(true);
}

auto Program::validatePack() -> bool {
  if(!packPath.text() || !file::exists(packPath.text())) return false;

  pack.open(packPath.text());

  if(!fetch("msu1.rom")) return false;
  if(!fetch("program.rom") && !fetch("patch.bps")) return false;

  fetch(patch);
  romLabel.setText(patch ? "ROM path:" : "No ROM needed");
  romPath.setEnabled(patch ? true : false);
  romChange.setEnabled(patch ? true : false);
  if(!patch) romPath.setText("");

  return !patch || validateROMPatch();
}

auto Program::validateROMPatch() -> bool {
  if(!romPath.text() || !file::exists(romPath.text())) return false;

  if(patch) patch->source(romPath.text());

  return true;
}

auto Program::fetch(string_view name) -> maybe<Decode::ZIP::File> {
  for(auto& file : pack.file) {
    if(file.name.match(name)) return file;
  }
  return nothing;
}

auto Program::fetch(unique_pointer<bpspatch>& patch) -> bool {
  if(patch) patch.reset();
  if(auto file = fetch("patch.bps")) {
    patch = new bpspatch;
    patchContents = pack.extract(file());
    patch->modify(patchContents.data(), patchContents.size());
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
    destination = {Location::dir(packPath.text()), outputName.text(), ".sfc/"};
    break;
  }

  case ExportMethod::SD2SNES: {
    destination = {Location::dir(packPath.text()), "SD2SNES-Snes9x/"};
    break;
  }

  }

  directory::create(destination);

  switch(exportMethod) {

  case ExportMethod::GamePak: {
    if(patch) {
      patch->target({destination, "program.rom"});
      patchResult = patch->apply();
    }

    break;
  }

  case ExportMethod::SD2SNES: {
    if(patch) {
      patch->target({destination, outputName.text(), ".sfc"});
      patchResult = patch->apply();
    } else {
      static string_vector roms = {
        "program.rom",
        "data.rom",
        "slot-*.rom",
        "*.boot.rom",
        "*.program.rom",
        "*.data.rom",
      };

      file rom({destination, outputName.text(), ".sfc"}, file::mode::write);
      for(string& romName : roms) {
        if(auto file = fetch(romName)) {
          rom.write(pack.extract(file()).data(), file().size);
        }
      }
      rom.close();
    }

    break;
  }

  }

  if(patch) {
    switch(patchResult) {
    case bpspatch::result::unknown:
      error("There was an unspecified problem in applying the BPS patch.");
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
      uint32_t expectedCRC32 = 0;
      for(uint i : range(4)) {
        expectedCRC32 |= patchContents[patchContents.size() - 12 + i] << (i << 3);
      }
      string response = warning({
        "The ROM's CRC32 does not match the patch's expected CRC32.\n",
        "The expected CRC32 is: ", hex(expectedCRC32, 8L).upcase(), "\n\n",

        "If you are applying multiple patches to a ROM, this is normal.\n",
        "Otherwise, make sure you select the correct ROM."
      }, {"Continue", "Cancel"});
      if(response == "Cancel") {
        setEnabled(true);
        information("Cancelled");
        return;
      }
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
    } else if(ext != ".bps") {
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
      path = {destination, outputName.text(), "-", trackID, ext};
    } else if(file.name == "msu1.rom") {
      path = {destination, outputName.text(), ".msu"};
    } else if(file.name == "program.rom") {
      path = {destination, outputName.text(), ".sfc"};
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
  switch(exportMethod) {

  case ExportMethod::GamePak: {
    if(exportManifest) {
      string icarusManifest;
      string daedalusManifest;

      if(auto manifest = execute("icarus", "--manifest", destination)) {
        icarusManifest = manifest.output;
      }
      if(auto manifest = execute("daedalus", "--manifest", destination)) {
        daedalusManifest = manifest.output;
        if(icarusManifest) {
          daedalusManifest = {daedalusManifest.split("\n\n")[0], "\n\n"};
        }
      }

      file::write({destination, "manifest.bml"}, {daedalusManifest, icarusManifest});
    }

    break;
  }

  case ExportMethod::SD2SNES: {
  }

  }

  if(patch) patch.reset();
  packPath.setText("");
  romPath.setText("");
  outputName.setText("");
  information("MSU1 pack exported!");
  progressBar.setPosition(0);

  setEnabled(true);
}

auto Program::information(const string& text) -> void {
  statusLabel.setText(text);
}

auto Program::warning(const string& text, const string_vector& buttons) -> string {
  return MessageDialog().setTitle("Mercurial Magic").setText(text).warning(buttons);
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
  outputName.setEnabled(enabled);
  gamepak.setEnabled(enabled);
  manifest.setEnabled(enabled);
  sd2snes.setEnabled(enabled);
  exportButton.setEnabled(enabled);
  exitButton.setText(enabled ? "Exit" : "Cancel");
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
