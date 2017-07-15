#include <ramus/audio/wave.hpp>

auto Program::convert(string path) -> bool {
  if(Location::suffix(path) == ".wav") {
    /*
    auto data = file::read(path);
    ramus::Audio::Wave audio(data);
    if(!audio) return false;

    if(audio.frequency != 44100) return false;

    uint loop = 0;

    file fp;
    if(fp.open({Location::path(path), Location::prefix(path), ".pcm"}, file::mode::write)) {
      fp.writes("MSU1");
      fp.writel(loop, 4);
      while(audio.remainingSamples) {
        auto sample = audio.sample();
        fp.writel(sample[0] * 32768.0, 2);
        fp.writel(sample[1] * 32768.0, 2);
      }
      fp.close();
    } else {
      error({Location::path(path), Location::prefix(path), ".pcm"});
      return false;
    }
    */

    uint loop = 0;
    execute("wav2msu", path, "-l", string{loop});
  }

  if(file::exists({Location::path(path), Location::prefix(path), ".pcm"})) {
    file::remove(path);
    return true;
  } else {
    return false;
  }
}
