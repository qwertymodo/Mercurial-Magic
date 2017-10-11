BasicTab::BasicTab(TabFrame* parent) : TabFrameItem(parent) {
  setText("Basic Settings");
  layout.setMargin(5);

  packLabel.setText("MSU1 Pack path:");
  packPath.onChange([&] {
    outputName.setText(Location::prefix(packPath.text().transform("\\", "/")));
    program->valid = program->validatePack();
    program->exportButton.setEnabled(program->valid && outputName.text());
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
    program->valid = program->validateROMPatch();
    program->exportButton.setEnabled(program->valid && outputName.text());
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
    program->exportButton.setEnabled(program->valid && outputName.text());
  });
  outputExtLabel.setText(".sfc/");

  selectLabel.setText("Export as...");

  gamepakExport.setText("Game Pak (cartridge folder)").onActivate([&] {
    program->exportMethod = Program::ExportMethod::GamePak;
    gamepakCreateManifest.setEnabled(program->icarus || program->daedalus);
    outputExtLabel.setText(".sfc/");
  });

  gamepakCreateManifest.onToggle([&] {
    program->createManifest = gamepakCreateManifest.checked();
  });

  sd2snesExport.setText("SD2SNES/Snes9x").onActivate([&] {
    program->exportMethod = Program::ExportMethod::SD2SNES;
    gamepakCreateManifest.setEnabled(false);
    outputExtLabel.setText({".sfc", program->createManifest ? "/" : ""});
  });
}

auto BasicTab::refresh() -> void {
  gamepakCreateManifest.setEnabled(program->icarus || program->daedalus);
  gamepakCreateManifest.setChecked(program->createManifest).onToggle([&] {
    program->createManifest = gamepakCreateManifest.checked();
  });
  if(program->icarus || program->daedalus) {
    string higanMin = program->daedalus ? "v094" : "v096";
    string higanMax = program->icarus   ? "v104" : "v095";
    gamepakCreateManifest.setText({"Create ", higanMin, "-", higanMax, " manifest"});
  } else {
    gamepakCreateManifest.setText({"icarus and daedalus not found"});
  }
}

auto BasicTab::setEnabled(bool enabled) -> void {
  packPath.setEnabled(enabled);
  packChange.setEnabled(enabled);
  romPath.setEnabled(enabled);
  romChange.setEnabled(enabled);
  outputName.setEnabled(enabled);
  gamepakExport.setEnabled(enabled);
  gamepakCreateManifest.setEnabled(enabled && gamepakExport.checked() && (program->icarus || program->daedalus));
  sd2snesExport.setEnabled(enabled);
}
