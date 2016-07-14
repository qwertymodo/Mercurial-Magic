#include <MercurialMagic.hpp>

unique_pointer<ExportSettings> exportSettings;

ExportSettings::ExportSettings() {
  exportSettings = this;

  layout.setMargin(5);

  setTitle("Mercurial Magic");
  setSize(layout.minimumSize());
  setResizable(false);

  selectLabel.setText("Export as...");

  ((RadioLabel*)&exportGroup.objects()[program->exportMethod])->setChecked();

  gamepak.setText("Game Pak (cartridge folder)");
  gamepak.onActivate([&] { program->exportMethod = Program::ExportMethod::GamePak; });

  manifest.setText("Export Manifest");
  manifest.onToggle([&] { program->exportManifest = manifest.checked(); });

  sd2snes.setText("SD2SNES");
  sd2snes.onActivate([&] { program->exportMethod = Program::ExportMethod::SD2SNES; });

  exportButton.setText("Export");
  exportButton.onActivate([&] {
    program->beginExport();
    exportButton.setEnabled(false);
    gamepak.setEnabled(false);
    manifest.setEnabled(false);
    sd2snes.setEnabled(false);
  });

  cancelButton.setText("Cancel");
  cancelButton.onActivate([&] { program->quit(); });

  onClose([&] { program->quit(); });
}

auto ExportSettings::setFilename(const string& filename) -> void {
  filenameLabel.setText({"Exporting ", filename, "..."});
}

auto ExportSettings::setProgress(uint files, uint fileCount) -> void {
  progressBar.setPosition(files * 100 / fileCount);
}
