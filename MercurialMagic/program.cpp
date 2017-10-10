#include <program.hpp>

unique_pointer<Program> program;

#include "export.cpp"
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

  gamepakExport.setText("Game Pak (cartridge folder)").onActivate([&] {
    exportMethod = Program::ExportMethod::GamePak;
    gamepakCreateManifest.setEnabled(icarus || daedalus);
    sd2snesForceManifest.setEnabled(false);
    exportManifest = gamepakCreateManifest.enabled() && gamepakCreateManifest.checked();
    outputExtLabel.setText(".sfc/");
  });

  gamepakCreateManifest.setEnabled(icarus || daedalus).setChecked(daedalus && !icarus).onToggle([&] {
    exportManifest = gamepakCreateManifest.checked();
  });
  if(icarus || daedalus) {
    string higanMin = daedalus ? "v094" : "v096";
    string higanMax = icarus   ? "v104" : "v095";
    gamepakCreateManifest.setText({"Create ", higanMin, "-", higanMax, " manifest"});
  } else {
    gamepakCreateManifest.setText({"icarus and daedalus not found"});
  }

  sd2snesExport.setText("SD2SNES/Snes9x").onActivate([&] {
    exportMethod = Program::ExportMethod::SD2SNES;
    gamepakCreateManifest.setEnabled(false);
    sd2snesForceManifest.setEnabled(icarus && daedalus);
    exportManifest = sd2snesForceManifest.enabled() && sd2snesForceManifest.checked();
    outputExtLabel.setText({".sfc", exportManifest ? "/" : ""});
  });

  sd2snesForceManifest.setText({"Force manifest creation (for testing)"});
  sd2snesForceManifest.setEnabled(false).setVisible(icarus && daedalus).onToggle([&] {
    exportManifest = sd2snesForceManifest.checked();
    outputExtLabel.setText({".sfc", exportManifest ? "/" : ""});
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

auto Program::setDestination() -> void {
  switch(exportMethod) {

  case ExportMethod::GamePak: {
    destination = {Location::dir(packPath.text()), outputName.text(), ".sfc/"};
    break;
  }

  case ExportMethod::SD2SNES: {
    if(exportManifest) {
      destination = {Location::dir(packPath.text()), outputName.text(), ".sfc/"};
    } else {
      destination = {Location::dir(packPath.text()), "SD2SNES-Snes9x/"};
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

auto Program::setProgress(uint files) -> void {
  progressBar.setPosition(files * 100 / pack.file.size());
}

auto Program::setEnabled(bool enabled) -> void {
  packPath.setEnabled(enabled);
  packChange.setEnabled(enabled);
  romPath.setEnabled(enabled);
  romChange.setEnabled(enabled);
  outputName.setEnabled(enabled);
  gamepakExport.setEnabled(enabled);
  gamepakCreateManifest.setEnabled(enabled && gamepakExport.checked() && (icarus || daedalus));
  sd2snesExport.setEnabled(enabled);
  sd2snesForceManifest.setEnabled(enabled && sd2snesExport.checked() && (icarus && daedalus));
  exportButton.setEnabled(enabled && validatePack());
  exitButton.setText(enabled ? "Exit" : "Cancel");
}

auto Program::reset() -> void {
  if(patch) patch.reset();
  pack.close();
  pack = {};
  packPath.setText("");
  romPath.setText("");
  outputName.setText("");
  trackIDs.reset();
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
