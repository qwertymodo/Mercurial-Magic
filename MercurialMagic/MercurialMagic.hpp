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
  auto fetch(bpspatch&) -> bool;
  auto exportPack() -> void;

  auto information(const string&) -> void;
  auto warning(const string&) -> void;
  auto error(const string&) -> void;

  auto main() -> void;
  auto quit() -> void;

  string packPath;
  string romPath;
  ExportMethod exportMethod;

  Decode::ZIP pack;
  vector<uint8_t> patchContents;
  bool usesPatch;
  bpspatch patch;
};

struct ExportSettings : Window {
  ExportSettings();

  HorizontalLayout layout{this};
    VerticalLayout selectLayout{&layout, Size{160, ~0}};
      Label selectLabel{&selectLayout, Size{~0, 0}};
      RadioLabel gamepak{&selectLayout, Size{~0, 0}};
      RadioLabel sd2snes{&selectLayout, Size{~0, 0}};
      Group exportGroup{&gamepak, &sd2snes};
    VerticalLayout buttonLayout{&layout, Size{80, ~0}};
      Button exportButton{&buttonLayout, Size{~0, 0}};
      Button cancelButton{&buttonLayout, Size{~0, 0}};
};

extern unique_pointer<Program> program;
extern unique_pointer<ExportSettings> exportSettings;
