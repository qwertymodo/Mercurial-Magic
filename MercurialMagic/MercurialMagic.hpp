#include <nall/nall.hpp>
#include <hiro/hiro.hpp>
using namespace nall;
using namespace hiro;

#include <nall/beat/patch.hpp>

struct Program : Window {
  Program(string_vector args);

  enum ExportMethod : uint {
    GamePak,
    SD2SNES,
  };

  auto validatePack() -> bool;
  auto validateROMPatch() -> bool;
  auto fetch(string_view) -> maybe<Decode::ZIP::File>;
  auto fetch(bpspatch&) -> bool;

  auto beginExport() -> void;
  auto iterateExport() -> void;
  auto finishExport() -> void;

  auto setProgress(uint files) -> void;
  auto setEnabled(bool enabled = true) -> void;

  auto information(const string&) -> void;
  auto warning(const string&) -> void;
  auto error(const string&) -> void;

  auto main() -> void;
  auto quit() -> void;

  VerticalLayout layout{this};
    HorizontalLayout packLayout{&layout, Size{~0, 0}};
      Label packLabel{&packLayout, Size{100, 0}};
      LineEdit packPath{&packLayout, Size{400, 0}};
      Button packChange{&packLayout, Size{80, 0}};
    HorizontalLayout romLayout{&layout, Size{~0, 0}};
      Label romLabel{&romLayout, Size{100, 0}};
      LineEdit romPath{&romLayout, Size{400, 0}};
      Button romChange{&romLayout, Size{80, 0}};
    Label selectLabel{&layout, Size{~0, 0}};
    HorizontalLayout gamepakLayout{&layout, Size{~0, 0}};
      RadioLabel gamepak{&gamepakLayout, Size{160, 0}};
      CheckLabel manifest{&gamepakLayout, Size{160, 0}};
    RadioLabel sd2snes{&layout, Size{160, 0}};
    Group exportGroup{&gamepak, &sd2snes};
    Label statusLabel{&layout, Size{~0, 0}};
    ProgressBar progressBar{&layout, Size{~0, 0}};
    HorizontalLayout buttonLayout{&layout, Size{~0, 0}};
      Button exportButton{&buttonLayout, Size{80, 0}};
      Button exitButton{&buttonLayout, Size{80, 0}};

  ExportMethod exportMethod;
  bool exportManifest;

  Decode::ZIP pack;
  vector<uint8_t> patchContents;
  bool usesPatch;
  bpspatch patch;

  uint zipIndex;
  string destination;
};

extern unique_pointer<Program> program;
