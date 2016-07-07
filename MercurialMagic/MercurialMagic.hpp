#include <nall/nall.hpp>
#include <hiro/hiro.hpp>
using namespace nall;
using namespace hiro;

#include <nall/beat/patch.hpp>

struct Program {
  Program(string_vector args);

  enum ExportMethod : uint {
    GamePak,
    SD2SNES,
  };

  auto validate() -> bool;
  auto fetch(string_view) -> maybe<Decode::ZIP::File>;
  auto fetch(bpspatch&) -> bool;

  auto beginExport() -> void;
  auto iterateExport() -> void;
  auto finishExport() -> void;

  auto information(const string&) -> void;
  auto warning(const string&) -> void;
  auto error(const string&) -> void;

  auto main() -> void;
  auto quit() -> void;

  string packPath;
  string romPath;
  ExportMethod exportMethod;
  bool exportManifest;

  Decode::ZIP pack;
  vector<uint8_t> patchContents;
  bool usesPatch;
  bpspatch patch;

  bool exporting;
  uint zipIndex;
  string destination;
};

struct ExportSettings : Window {
  ExportSettings();
  auto setProgress(uint files, uint fileCount) -> void;

  HorizontalLayout layout{this};
    VerticalLayout selectLayout{&layout, Size{~0, ~0}};
      Label selectLabel{&selectLayout, Size{~0, 0}};
      HorizontalLayout gamepakLayout{&selectLayout, Size{~0, 0}};
        RadioLabel gamepak{&gamepakLayout, Size{160, 0}};
        CheckLabel manifest{&gamepakLayout, Size{160, 0}};
      RadioLabel sd2snes{&selectLayout, Size{160, 0}};
      Group exportGroup{&gamepak, &sd2snes};
      ProgressBar progressBar{&selectLayout, Size{~0, 0}};
    VerticalLayout buttonLayout{&layout, Size{80, ~0}};
      Button exportButton{&buttonLayout, Size{~0, 0}};
      Button cancelButton{&buttonLayout, Size{~0, 0}};
};

extern unique_pointer<Program> program;
extern unique_pointer<ExportSettings> exportSettings;
