auto Program::beginExport() -> void {
  setDestination();
  directory::create(destination);

  zipIndex = 0;
  setProgress(0);

  uint patchResult;

  switch(exportMethod) {

  case ExportMethod::GamePak: {
    if(patch) {
      patch->target({destination, "program.rom"});
      patchResult = patch->apply();
    }

    break;
  }

  case ExportMethod::SD2SNES: {
    string filename = exportManifest ? "program.rom" : string{outputName.text(), ".sfc"};
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

    if(patch) {
      patch->target({destination, filename});  //temporary name
      patchResult = patch->apply();
    }

    break;
  }

  }

  if(patch) {
    switch(patchResult) {
    case bpspatch::result::unknown:
      error("There was an unspecified problem in applying the BPS patch.");
      break;
    case bpspatch::result::patch_invalid_header:
      error("The BPS patch's header is invalid!");
      break;
    case bpspatch::result::target_too_small:
    case bpspatch::result::target_checksum_invalid:
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
      uint32_t expectedCRC32 = 0;
      for(uint i : range(4)) {
        expectedCRC32 |= patchContents[patchContents.size() - 12 + i] << (i << 3);
      }
      string response = warning({
        "The ROM's CRC32 does not match the patch's expected CRC32.\n",
        "The expected CRC32 is: ", hex(expectedCRC32, 8L).upcase(), "\n\n",

        "If you are applying multiple patches to a ROM, this is normal.\n",
        "Otherwise, make sure you select the correct ROM."
      }, {"Continue", "Cancel"});
      if(response == "Cancel") {
        setEnabled(true);
        information("Cancelled");
        return;
      }
      break;
    }
  }

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
      path = {destination, outputName.text(), "-", trackID, ext};
    } else if(file.name == "msu1.rom") {
      path = {destination, exportManifest ? "msu1.rom" : string{outputName.text(), ".msu"}};
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
    } else if(ext == ".ogg") {
      error({
        "Could not convert ", file.name, " to PCM!\n"
        "Ogg Vorbis is not currently supported."
      });
    } else if(ext == ".flac") {
      error({
        "Could not convert ", file.name, " to PCM!\n"
        "FLAC is not currently supported."
      });
    } else if(ext == ".mp3") {
      error({
        "Could not convert ", file.name, " to PCM!\n"
        "MP3 is not currently supported."
      });
    }
    return false;
  }

  setProgress(zipIndex);
  return true;
}

auto Program::finishExport() -> void {
  if(patch) patch.reset();

  string icarusManifest;
  string daedalusManifest;
  if(exportManifest) {
    if(auto manifest = execute("icarus", "--manifest", destination)) {
      icarusManifest = manifest.output;
    }
    if(auto manifest = execute("daedalus", "--manifest", destination)) {
      daedalusManifest = manifest.output;
      if(icarusManifest) {
        daedalusManifest = {daedalusManifest.split("\n\n").left(), "\n\n"};
      }
    }
  }

  switch(exportMethod) {

  case ExportMethod::GamePak: {
    if(exportManifest) file::write({destination, "manifest.bml"}, {daedalusManifest, icarusManifest});
    break;
  }

  case ExportMethod::SD2SNES: {
    if(exportManifest) {
      static auto substitute = [&](string& manifest) -> void {
        manifest.
          replace("program.rom", {"\"", outputName.text(), ".sfc\""}).
          replace("save.ram",    {"\"", outputName.text(), ".srm\""}).
          replace("msu1.rom",    {"\"", outputName.text(), ".msu\""});
      };
      substitute(icarusManifest);
      substitute(daedalusManifest);
      string tracks = "";
      trackIDs.sort();
      for(uint trackID : trackIDs) {
        tracks.append("\n    track number=", trackID, " name=\"", outputName.text(), "-", trackID, ".pcm\"");
      }
      string_vector sections;
      sections = daedalusManifest.split("\n\n");
      sections[0].append(tracks);
      daedalusManifest = sections.merge("\n\n");
      sections = icarusManifest.split("\n\n");
      sections[0].append(tracks);
      icarusManifest = sections.merge("\n\n");
      file::write({destination, "manifest.bml"}, {daedalusManifest, icarusManifest});
      file::rename({destination, "program.rom"}, {destination, outputName.text(), ".sfc"});
      file::rename({destination, "msu1.rom"}, {destination, outputName.text(), ".msu"});
    }
    break;
  }

  }

  packPath.setText("");
  romPath.setText("");
  outputName.setText("");
  trackIDs.reset();
  information("MSU1 pack exported!");
  progressBar.setPosition(0);

  setEnabled(true);
}
