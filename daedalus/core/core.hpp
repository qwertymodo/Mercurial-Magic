struct Daedalus {
  virtual auto create(const string& pathname) -> bool {
    return directory::create(pathname);
  }

  virtual auto exists(const string& filename) -> bool {
    return file::exists(filename);
  }

  virtual auto copy(const string& target, const string& source) -> bool {
    return file::copy(target, source);
  }

  virtual auto write(const string& filename, const uint8_t* data, uint size) -> bool {
    return file::write(filename, data, size);
  }

  auto write(const string& filename, const vector<uint8_t>& buffer) -> bool {
    return write(filename, buffer.data(), buffer.size());
  }

  auto write(const string& filename, const string& text) -> bool {
    return write(filename, (const uint8_t*)text.data(), text.size());
  }

  //core.cpp
  Daedalus();

  auto error() const -> string;
  auto missing() const -> string_vector;
  auto success(string location) -> string;
  auto failure(string message) -> string;

  auto manifest(string location) -> string;
  auto import(string location) -> string;

  auto concatenate(vector<uint8_t>& output, string location) -> void;

  //super-famicom.cpp
  auto superFamicomManifest(string location) -> string;
  auto superFamicomManifest(vector<uint8_t>& buffer, string location) -> string;
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
  string_vector missingFiles;
};
