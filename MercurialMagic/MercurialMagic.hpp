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

  auto information(const string&) -> void;
  auto warning(const string&) -> void;
  auto error(const string&) -> void;

  auto fetch(bpspatch&) -> bool;
  auto exportPack() -> void;

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

  VerticalLayout layout{this};
    Label exportLabel{&layout, Size{~0, 0}};
    RadioLabel gamepak{&layout, Size{~0, 0}};
    RadioLabel sd2snes{&layout, Size{~0, 0}};
    Group exportGroup{&gamepak, &sd2snes};
    HorizontalLayout buttonLayout{&layout, Size{~0, 0}};
      Button exportButton{&buttonLayout, Size{80, 0}};
      Button cancelButton{&buttonLayout, Size{80, 0}};
};

extern unique_pointer<Program> program;
extern unique_pointer<ExportSettings> exportSettings;
