struct Daedalus {
  //core.cpp
  Daedalus();

  auto error() const -> string;
  auto success(string location) -> string;
  auto failure(string message) -> string;

  auto manifest(string location) -> string;
  auto import(string location) -> string;

  auto concatenate(vector<uint8_t>& output, string location) -> void;

  //super-famicom.cpp
  auto superFamicomManifest(string location) -> string;
  auto superFamicomManifest(vector<uint8_t>& buffer, string location, bool* firmwareAppended = nullptr) -> string;
  auto superFamicomManifestScan(vector<Markup::Node>& roms, Markup::Node node) -> void;
  auto superFamicomImport(vector<uint8_t>& buffer, string location) -> string;

  //bs-memory.cpp
  auto bsMemoryManifest(string location) -> string;
  auto bsMemoryManifest(vector<uint8_t>& buffer, string location) -> string;
  auto bsMemoryImport(vector<uint8_t>& buffer, string location) -> string;

  //sufami-turbo.cpp
  auto sufamiTurboManifest(string location) -> string;
  auto sufamiTurboManifest(vector<uint8_t>& buffer, string location) -> string;
  auto sufamiTurboImport(vector<uint8_t>& buffer, string location) -> string;

private:
  string errorMessage;
};
