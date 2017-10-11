#include <program.hpp>

unique_pointer<Program> program;

#include "export.cpp"
#include "convert.cpp"
#include "basic-settings.cpp"
#include "advanced-settings.cpp"

Program::Program(string_vector args) {
  program = this;
  Application::onMain({&Program::main, this});

  icarus = execute("icarus", "--name").output.strip() == "icarus";
  daedalus = execute("daedalus", "--name").output.strip() == "daedalus";

  exportMethod = ExportMethod::GamePak;
  createManifest = daedalus && !icarus;
  sd2snesForceManifest = false;
  violateBPS = false;

  basicTab.refresh();
  advancedTab.refresh();

  layout.setMargin(5);
  setTitle("Mercurial Magic");
  setSize({700, 300});
  setResizable(false);

  exportButton.setText("Export").onActivate([&] {
    setEnabled(false);
    beginExport();
  });

  exitButton.setText("Exit").onActivate({&Program::quit, this});

  onClose({&Program::quit, this});

  args.takeLeft();  //ignore program location in argument parsing

  valid = false;
  if(args) {
    basicTab.packPath.setText(args.takeLeft());
    valid = validatePack();
  }

  if(fetch("patch.bps") && args) {
    basicTab.romPath.setText(args.takeLeft());
    valid = validateROMPatch();
  }

  exportButton.setEnabled(valid);

  setVisible(true);
}

auto Program::packPath() -> string {
  return basicTab.packPath.text();
}

auto Program::romPath() -> string {
  return basicTab.romPath.text();
}

auto Program::outputName() -> string {
  return basicTab.outputName.text();
}

auto Program::validatePack() -> bool {
  if(!packPath() || !file::exists(packPath())) return false;

  pack.open(packPath());

  if(!fetch("msu1.rom")) return false;
  bool hasROM = !!fetch("program.rom");
  bool hasPatch = false;
  if(auto file = fetch("patch.bps")) {
    patchContents = pack.extract(file());
    hasPatch = true;
  }
  if(!hasROM && !hasPatch) return false;

  basicTab.romLabel.setText(hasPatch ? "ROM path:" : "No ROM needed");
  basicTab.romPath.setEnabled(hasPatch);
  basicTab.romChange.setEnabled(hasPatch);
  if(hasROM) basicTab.romPath.setText("");

  return hasROM || (hasPatch && validateROMPatch());
}

auto Program::validateROMPatch() -> bool {
  if(!romPath() || !file::exists(romPath())) return false;

  if(fetch("patch.bps")) {
    if(!violateBPS) {
      uint32_t sourceChecksum = Hash::CRC32(file::read(romPath())).value();
      uint32_t expectedChecksum = 0;
      for(uint i : range(4)) {
        expectedChecksum |= patchContents[patchContents.size() - 12 + i] << (i << 3);
      }
      if(sourceChecksum != expectedChecksum) {
        warning({
          "The patch is not compatible with this ROM.\n",
          "Expected CRC32: ", hex(expectedChecksum, 8L).upcase(), "\n\n",

          "If you are attempting to multi-patch (which violates the BPS spec), ",
          "check the Advanced Settings tab to do so."
        });
        return false;
      }
    }
  }

  return true;
}

auto Program::setDestination() -> void {
  switch(exportMethod) {

  case ExportMethod::GamePak: {
    destination = {Location::dir(packPath()), outputName(), ".sfc/"};
    break;
  }

  case ExportMethod::SD2SNES: {
    if(sd2snesForceManifest) {
      destination = {Location::dir(packPath()), outputName(), ".sfc/"};
    } else {
      destination = {Location::dir(packPath()), "SD2SNES-Snes9x/"};
    }
    break;
  }

  }
}

auto Program::fetch(string_view name) -> maybe<Decode::ZIP::File> {
  for(auto& file : pack.file) {
    if(file.name.match(name)) return file;
  }
  return nothing;
}

auto Program::setProgress(uint files) -> void {
  progressBar.setPosition(files * 100 / pack.file.size());
}

auto Program::setEnabled(bool enabled) -> void {
  basicTab.setEnabled(enabled);
  advancedTab.setEnabled(enabled);

  exportButton.setEnabled(enabled && validatePack());
  exitButton.setText(enabled ? "Exit" : "Cancel");
}

auto Program::reset() -> void {
  pack.close();
  pack = {};

  basicTab.packPath.setText("");
  basicTab.romPath.setText("");
  basicTab.outputName.setText("");

  trackIDs.reset();
  progressBar.setPosition(0);
  setEnabled(true);
}

auto Program::information(const string& text) -> void {
  statusLabel.setText(text);
}

auto Program::warning(const string& text) -> void {
  MessageDialog().setTitle("Mercurial Magic").setText(text).warning();
}

auto Program::warning(const string& text, const string_vector& buttons) -> string {
  return MessageDialog().setTitle("Mercurial Magic").setText(text).warning(buttons);
}

auto Program::error(const string& text) -> void {
  reset();
  statusLabel.setText("Error");
  MessageDialog().setTitle("Mercurial Magic").setText(text).error();
}

auto Program::error(const string& text, const string_vector& buttons) -> string {
  reset();
  statusLabel.setText("Error");
  return MessageDialog().setTitle("Mercurial Magic").setText(text).error(buttons);
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
