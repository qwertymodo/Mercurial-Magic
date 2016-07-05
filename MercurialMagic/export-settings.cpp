#include <MercurialMagic.hpp>

unique_pointer<ExportSettings> exportSettings;

ExportSettings::ExportSettings() {
  exportSettings = this;

  layout.setMargin(5);

  setTitle("Mercurial Magic");
  //compensate for Label which is not counted
  setSize({layout.minimumSize().width(), layout.minimumSize().height() + 16});
  setResizable(false);

  selectLabel.setText("Export as...");

  ((RadioLabel*)&exportGroup.objects()[program->exportMethod])->setChecked();

  gamepak.setText("Game Pak (cartridge folder)");
  gamepak.onActivate([&] { program->exportMethod = Program::ExportMethod::GamePak; });

  sd2snes.setText("SD2SNES");
  sd2snes.onActivate([&] { program->exportMethod = Program::ExportMethod::SD2SNES; });

  exportButton.setText("Export");
  exportButton.onActivate([&] {
    program->exportPack();
    program->quit();
  });

  cancelButton.setText("Cancel");
  cancelButton.onActivate([&] {
    program->quit();
  });

  onClose([&] { program->quit(); });
}
