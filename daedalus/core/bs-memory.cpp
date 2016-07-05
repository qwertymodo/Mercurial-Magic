auto Daedalus::bsMemoryManifest(string location) -> string {
  vector<uint8_t> buffer;
  concatenate(buffer, {location, "program.rom"});
  return bsMemoryManifest(buffer, location);
}

auto Daedalus::bsMemoryManifest(vector<uint8_t>& buffer, string location) -> string {
  string markup;
  string digest = Hash::SHA256(buffer.data(), buffer.size()).digest();

  if(!markup) {
    BSMemoryCartridge cartridge{buffer.data(), buffer.size()};
    if(markup = cartridge.markup) {
      markup.append("\n");
      markup.append("information\n");
      markup.append("  title:  ", Location::prefix(location), "\n");
      markup.append("  sha256: ", digest, "\n");
      markup.append("  note:   ", "heuristically generated by daedalus\n");
    }
  }

  return markup;
}

auto Daedalus::bsMemoryImport(vector<uint8_t>& buffer, string location) -> string {
  auto name = Location::prefix(location);
  auto source = Location::path(location);
  string target{settings["Library/Location"].text(), "BS Memory/", name, ".bs/"};
//if(directory::exists(target)) return failure("game already exists");

  auto markup = bsMemoryManifest(buffer, location);
  if(!markup) return failure("failed to parse ROM image");
  if(!directory::create(target)) return failure("library path unwritable");

  file::write({target, "manifest.bml"}, markup);
  file::write({target, "program.rom"}, buffer);
  return success(target);
}