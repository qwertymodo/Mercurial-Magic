AdvancedTab::AdvancedTab(TabFrame* parent) : TabFrameItem(parent) {
  setText("Advanced Settings");
  layout.setMargin(5);

  sd2snesForceManifest.setText("SD2SNES/Snes9x only: Force manifest creation (for testing)").onToggle([&] {
    program->sd2snesForceManifest = sd2snesForceManifest.checked();
    program->basicTab.outputExtLabel.setText({".sfc", program->sd2snesForceManifest ? "/" : ""});
  });

  violateBPS.setText("Violate BPS and allow multi-patching").onToggle([&] {
    program->violateBPS = violateBPS.checked();
    program->valid = program->validateROMPatch();
    program->exportButton.setEnabled(program->valid && program->outputName());
  });
}

auto AdvancedTab::refresh() -> void {
  sd2snesForceManifest.setVisible(program->icarus && program->daedalus);
  sd2snesForceManifest.setChecked(program->sd2snesForceManifest);
  violateBPS.setChecked(program->violateBPS);
}

auto AdvancedTab::setEnabled(bool enabled) -> void {
  sd2snesForceManifest.setEnabled(enabled && sd2snesForceManifest.visible());
  violateBPS.setEnabled(enabled);
}
