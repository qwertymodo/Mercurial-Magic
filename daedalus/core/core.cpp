Daedalus::Daedalus() {
}

auto Daedalus::error() const -> string {
  return errorMessage;
}

auto Daedalus::missing() const -> string_vector {
  return missingFiles;
}

auto Daedalus::success(string location) -> string {
  errorMessage = "";
  return location;
}

auto Daedalus::failure(string message) -> string {
  errorMessage = message;
  return {};
}

auto Daedalus::manifest(string location) -> string {
  location.transform("\\", "/").trimRight("/").append("/");
  if(!directory::exists(location)) return "";

  auto type = Location::suffix(location).downcase();
  if(type == ".sfc") return superFamicomManifest(location);
  if(type == ".bs") return bsMemoryManifest(location);
  if(type == ".st") return sufamiTurboManifest(location);

  return "";
}

auto Daedalus::import(string location) -> string {
  errorMessage = {};
  missingFiles = {};

  location.transform("\\", "/").trimRight("/");
  if(!file::exists(location)) return failure("file does not exist");
  if(!file::readable(location)) return failure("file is unreadable");

  auto name = Location::prefix(location);
  auto type = Location::suffix(location).downcase();
  if(!name || !type) return failure("invalid file name");

  auto buffer = file::read(location);
  if(!buffer) return failure("file is empty");

  if(type == ".zip") {
    Decode::ZIP zip;
    if(!zip.open(location)) return failure("ZIP archive is invalid");
    if(!zip.file) return failure("ZIP archive is empty");

    name = Location::prefix(zip.file[0].name);
    type = Location::suffix(zip.file[0].name).downcase();
    buffer = zip.extract(zip.file[0]);
  }

  if(type == ".sfc" || type == ".smc") return superFamicomImport(buffer, location);
  if(type == ".bs") return bsMemoryImport(buffer, location);
  if(type == ".st") return sufamiTurboImport(buffer, location);

  return failure("unrecognized file extension");
}

auto Daedalus::concatenate(vector<uint8_t>& output, string location) -> void {
  if(auto input = file::read(location)) {
    auto size = output.size();
    output.resize(size + input.size());
    memory::copy(output.data() + size, input.data(), input.size());
  }
}
