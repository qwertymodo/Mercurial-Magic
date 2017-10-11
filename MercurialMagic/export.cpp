#include <ramus/patch/bps-ignore-size.hpp>

auto Program::beginExport() -> void {
  setDestination();
  directory::create(destination);

  zipIndex = 0;
  setProgress(0);

  bpspatch* patch = nullptr;
  ramus::bpspatch_ignore_size* patch_ignore_size = nullptr;
  if(patchContents) {
    if(!violateBPS) {
      patch = new bpspatch;
      patch->modify(patchContents.data(), patchContents.size());
      patch->source(romPath());
    } else {
      patch_ignore_size = new ramus::bpspatch_ignore_size;
      patch_ignore_size->modify(patchContents.data(), patchContents.size());
      patch_ignore_size->source(romPath());
    }
  }

  string targetPath;

  switch(exportMethod) {

  case ExportMethod::GamePak: {
    targetPath = {destination, "program.rom"};

    break;
  }

  case ExportMethod::SD2SNES: {
    string filename = sd2snesForceManifest ? "program.rom" : string{outputName(), ".sfc"};
    static string_vector roms = {
      "program.rom",
      "data.rom",
      "slot-*.rom",
      "*.boot.rom",
      "*.program.rom",
      "*.data.rom",
    };

    file rom({destination, filename}, file::mode::write);
    for(string& romName : roms) {
      if(auto file = fetch(romName)) {
        rom.write(pack.extract(file()).data(), file().size);
      }
    }
    rom.close();

    targetPath = {destination, filename};

    break;
  }

  }

  if(patch) {
    patch->target(targetPath);
    uint patchResult = patch->apply();
    switch(patchResult) {
    case bpspatch::result::unknown:
      return error("There was an unspecified problem in applying the BPS patch.");
    case bpspatch::result::patch_invalid_header:
      return error("The BPS patch's header is invalid!");
    case bpspatch::result::target_too_small:
    case bpspatch::result::target_checksum_invalid:
    case bpspatch::result::patch_too_small:
    case bpspatch::result::patch_checksum_invalid:
      return error("The BPS patch is corrupt!");
    case bpspatch::result::source_too_small:
      return error({
        "This ROM is too small!\n",
        "Check that you are selecting the correct ROM, and try again."
      });
    case bpspatch::result::source_checksum_invalid:
      uint32_t expectedCRC32 = 0;
      for(uint i : range(4)) {
        expectedCRC32 |= patchContents[patchContents.size() - 12 + i] << (i << 3);
      }
      return error({
        "The patch is not compatible with this ROM.\n",
        "Expected CRC32: ", hex(expectedCRC32, 8L).upcase()
      });
    }
  } else if(patch_ignore_size) {
    patch_ignore_size->target(targetPath);
    uint patchResult = patch_ignore_size->apply();
    switch(patchResult) {
    case ramus::bpspatch_ignore_size::result::unknown:
      return error("There was an unspecified problem in applying the BPS patch.");
    case ramus::bpspatch_ignore_size::result::patch_invalid_header:
      return error("The BPS patch's header is invalid!");
    case ramus::bpspatch_ignore_size::result::target_too_small:
    case ramus::bpspatch_ignore_size::result::patch_too_small:
    case ramus::bpspatch_ignore_size::result::patch_checksum_invalid:
      return error("The BPS patch is corrupt!");
    case ramus::bpspatch_ignore_size::result::source_too_small:
      return error({
        "This ROM is too small!\n",
        "Check that you are selecting the correct ROM, and try again."
      });
    }
  }

  if(patch) delete patch;
  if(patch_ignore_size) delete patch_ignore_size;

  thread::create([&](uintptr_t) -> void {
    while(zipIndex < pack.file.size()) {
      if(!iterateExport()) break;
    }
    finishExport();
  });
}

auto Program::iterateExport() -> bool {
  auto& file = pack.file[zipIndex++];
  information({"Exporting ", file.name, "..."});

  string ext = Location::suffix(file.name);
  string path;
  uint16_t trackID;
  if(ext == ".pcm"
  || ext == ".wav"
  || ext == ".ogg"
  || ext == ".flac"
  || ext == ".mp3") {
    uint start = 0;
    uint length = 0;
    for(uint pos : range(0, file.name.size())) {
      if(file.name[pos] >= '0' && file.name[pos] <= '9') { start = pos; break; }
    }
    for(uint pos : range(start, file.name.size())) {
      if(file.name[pos] < '0' || file.name[pos] > '9') { length = pos - start; break; }
    }
    trackID = slice(file.name, start, length).natural();
    trackIDs.append(trackID);
  }

  switch(exportMethod) {

  case ExportMethod::GamePak: {
    if(ext == ".pcm"
    || ext == ".wav"
    || ext == ".ogg"
    || ext == ".flac"
    || ext == ".mp3") {
      path = {destination, "track-", trackID, ext};
    } else if(ext != ".bps") {
      path = {destination, file.name};
    }
    break;
  }

  case ExportMethod::SD2SNES: {
    string ext = Location::suffix(file.name);
    if(ext == ".pcm"
    || ext == ".wav"
    || ext == ".ogg"
    || ext == ".flac"
    || ext == ".mp3") {
      path = {destination, outputName(), "-", trackID, ext};
    } else if(file.name == "msu1.rom") {
      path = {destination, sd2snesForceManifest ? "msu1.rom" : string{outputName(), ".msu"}};
    } else if(file.name.endsWith("program.rom")) {
      path = "";
    } else if(file.name.endsWith("data.rom")) {
      path = "";
    }

    break;
  }

  }

  if(path) file::write(path, pack.extract(file));

  bool result = true;
  if(ext == ".wav"
  || ext == ".ogg"
  || ext == ".flac"
  || ext == ".mp3") result = convert(path);

  if(!result) {
    if(ext == ".wav") {
      string response = error({
        "Could not convert ", file.name, " to PCM!\n"
        "Please check if wav2msu is installed correctly.\n"
        #if defined(PLATFORM_WINDOWS)
        "Would you like to download wav2msu from SMW Central?"
        #else
        "Would you like to visit wav2msu's GitHub page?"
        #endif
      }, {"Yes", "No"});
      if(response == "Yes") {
        #if defined(PLATFORM_WINDOWS)
        invoke("https://www.smwcentral.net/?p=section&a=details&id=4872");
        #else
        invoke("https://github.com/jbaiter/wav2msu");
        #endif
      }
      thread::exit();
    } else if(ext == ".ogg") {
      error({
        "Could not convert ", file.name, " to PCM!\n"
        "Ogg Vorbis is not currently supported."
      });
      thread::exit();
    } else if(ext == ".flac") {
      error({
        "Could not convert ", file.name, " to PCM!\n"
        "FLAC is not currently supported."
      });
      thread::exit();
    } else if(ext == ".mp3") {
      error({
        "Could not convert ", file.name, " to PCM!\n"
        "MP3 is not currently supported."
      });
      thread::exit();
    }
    return false;
  }

  setProgress(zipIndex);
  return true;
}

auto Program::finishExport() -> void {
  string icarusManifest;
  string daedalusManifest;

  switch(exportMethod) {

  case ExportMethod::GamePak: {
    if(createManifest) {
      if(auto manifest = execute("icarus", "--manifest", destination)) {
        icarusManifest = manifest.output;
      }
      if(auto manifest = execute("daedalus", "--manifest", destination)) {
        daedalusManifest = manifest.output;
        if(icarusManifest) {
          daedalusManifest = {daedalusManifest.split("\n\n").left(), "\n\n"};
        }
      }
      file::write({destination, "manifest.bml"}, {daedalusManifest, icarusManifest});
    }
    break;
  }

  case ExportMethod::SD2SNES: {
    if(sd2snesForceManifest) {
      if(auto manifest = execute("icarus", "--manifest", destination)) {
        icarusManifest = manifest.output;
      }
      if(auto manifest = execute("daedalus", "--manifest", destination)) {
        daedalusManifest = manifest.output;
        if(icarusManifest) {
          daedalusManifest = {daedalusManifest.split("\n\n").left(), "\n\n"};
        }
      }

      static auto substitute = [&](string& manifest) -> void {
        manifest.
          replace("program.rom", {"\"", outputName(), ".sfc\""}).
          replace("save.ram",    {"\"", outputName(), ".srm\""}).
          replace("msu1.rom",    {"\"", outputName(), ".msu\""});
      };
      substitute(icarusManifest);
      substitute(daedalusManifest);

      string tracks = "";
      trackIDs.sort();
      for(uint trackID : trackIDs) {
        tracks.append("\n    track number=", trackID, " name=\"", outputName(), "-", trackID, ".pcm\"");
      }

      string_vector sections;
      sections = daedalusManifest.split("\n\n");
      sections[0].append(tracks);
      daedalusManifest = sections.merge("\n\n");
      sections = icarusManifest.split("\n\n");
      sections[0].append(tracks);
      icarusManifest = sections.merge("\n\n");

      file::write({destination, "manifest.bml"}, {daedalusManifest, icarusManifest});
      file::rename({destination, "program.rom"}, {destination, outputName(), ".sfc"});
      file::rename({destination, "msu1.rom"}, {destination, outputName(), ".msu"});
    }
    break;
  }

  }

  information("MSU1 pack exported!");
  reset();
}
