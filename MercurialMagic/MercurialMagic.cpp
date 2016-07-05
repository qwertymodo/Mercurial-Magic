#include <MercurialMagic.hpp>

unique_pointer<Program> program;

Program::Program(string_vector args) {
  program = this;
  Application::onMain({&Program::main, this});

  args.takeLeft();  //ignore program location in argument parsing

  if(args) {
    packPath = args.takeLeft();
  } else {
    packPath = BrowserDialog()
    .setTitle("Load MSU-1 Pack")
    .setPath(Path::program())
    .setFilters(string{"MSU-1 Pack|*.msu1"})
    .openFile();
  }

  if(!packPath) quit();

  pack.open(packPath);

  usesPatch = fetch(patch);

  if(usesPatch) {
    if(args) {
      romPath = args.takeLeft();
    } else {
      romPath = BrowserDialog()
      .setTitle("Load Super Famicom ROM")
      .setPath(Path::real(packPath))
      .setFilters(string{"Super Famicom ROM|*.sfc"})
      .openFile();
    }
  }

  if(!romPath) quit();
  if(usesPatch) patch.source(romPath);

  exportMethod = ExportMethod::GamePak;
  new ExportSettings;
  exportSettings->setVisible(true);
}

auto Program::information(const string& text) -> void {
  MessageDialog().setTitle("Mercurial Magic").setText(text).information();
}

auto Program::warning(const string& text) -> void {
  MessageDialog().setTitle("Mercurial Magic").setText(text).warning();
}

auto Program::error(const string& text) -> void {
  MessageDialog().setTitle("Mercurial Magic").setText(text).error();
  quit();
}

auto Program::fetch(bpspatch& patch) -> bool {
  bool valid = false;
  for(auto& file : pack.file) {
    if(file.name != "patch.bps") continue;
    valid = true;
    patchContents = pack.extract(file);
    patch.modify(patchContents.data(), patchContents.size());
  }
  return valid;
}

auto Program::exportPack() -> void {
  vector<uint8_t> contents;

  switch(exportMethod) {

  case ExportMethod::GamePak: {
    string gamepak = {dirname(packPath), prefixname(packPath), suffixname(romPath), "/"};
    directory::create(gamepak);

    if(usesPatch) patch.target({gamepak, "program.rom"});
    for(auto& file : pack.file) {
      if(suffixname(file.name) != ".rom" && suffixname(file.name) != ".pcm") continue;
      contents = pack.extract(file);
      file::write({gamepak, file.name}, contents);
    }
    break;
  }

  case ExportMethod::SD2SNES: {
    string sd2snes = {dirname(packPath), "SD2SNES/"};
    directory::create(sd2snes);

    if(usesPatch) patch.target({sd2snes, prefixname(packPath), suffixname(romPath)});
    uint trackID;
    for(auto& file : pack.file) {
      if(file.name == "msu1.rom") {
        contents = pack.extract(file);
        file::write({sd2snes, prefixname(packPath), ".msu"}, contents);
      } else if(suffixname(file.name) == ".pcm") {
        if(file.name.beginsWith("track-")) {  //Track name is /track-[0-9]\.pcm/
          trackID = string{file.name}.trimLeft("track-").trimRight(".pcm").natural();
        } else {  //Track name is /([0-9]+)([^0-9].*)\.pcm/, where $1 is the ID and $2 is the name
          uint length = 0;
          for(uint pos : range(file.name.size())) {
            if(file.name[pos] < '0' || file.name[pos] > '9') { length = pos; break; }
          }
          trackID = slice(file.name, 0, length).natural();
        }
        contents = pack.extract(file);
        file::write({sd2snes, prefixname(packPath), "-", trackID, ".pcm"}, contents);
      }
    }
    break;
  }

  }

  if(usesPatch) {
    uint result = patch.apply();
    switch(result) {
    case bpspatch::result::unknown:
    case bpspatch::result::target_too_small:
    case bpspatch::result::target_checksum_invalid:
      error("There was an unspecified problem in exporting the MSU-1 pack.");
      break;
    case bpspatch::result::patch_invalid_header:
      error("The BPS patch's header is invalid!");
      break;
    case bpspatch::result::patch_too_small:
    case bpspatch::result::patch_checksum_invalid:
      error("The BPS patch is corrupt!");
      break;
    case bpspatch::result::source_too_small:
      error({
        "This ROM is too small!\n",
        "Check that you are selecting the correct ROM, and try again."
      });
      break;
    case bpspatch::result::source_checksum_invalid:
      warning({
        "The ROM's checksum does not match the one in the patch.\n",
        "If you are applying multiple patches to a ROM, this is normal.\n",
        "Otherwise, make sure you select the correct ROM."
      });
      break;
    }
  }

  information("MSU-1 pack exported!");
}

auto Program::main() -> void {
  usleep(2000);
}

auto Program::quit() -> void {
  exportSettings->setVisible(false);
  Application::quit();
}

#include <nall/main.hpp>
auto nall::main(string_vector args) -> void {
  Application::setName("Mercurial Magic");
  new Program(args);
  Application::run();
}
