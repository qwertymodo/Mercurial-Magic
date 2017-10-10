#include <nall/nall.hpp>
using namespace nall;

#include <hiro/hiro.hpp>
using namespace hiro;

auto locate(string name) -> string {
  string location = {Path::program(), name};
  if(inode::exists(location)) return location;

  location = {Path::config(), "daedalus/", name};
  if(inode::exists(location)) return location;

  directory::create({Path::local(), "daedalus/"});
  return {Path::local(), "daedalus/", name};
}

#include "settings.cpp"
Settings settings;

#include "heuristics/super-famicom.cpp"
#include "heuristics/bs-memory.cpp"
#include "heuristics/sufami-turbo.cpp"

#include "core/core.hpp"
#include "core/core.cpp"
#include "core/super-famicom.cpp"
#include "core/bs-memory.cpp"
#include "core/sufami-turbo.cpp"

#if !defined(DAEDALUS_LIBRARY)

Daedalus daedalus;
#include "ui/ui.hpp"
#include "ui/scan-dialog.cpp"
#include "ui/settings-dialog.cpp"
#include "ui/import-dialog.cpp"
#include "ui/error-dialog.cpp"

#include <nall/main.hpp>
auto nall::main(string_vector args) -> void {
  Application::setName("daedalus");

  if(args.size() == 2 && args[1] == "--name") {
    return print("daedalus");
  }

  if(args.size() == 3 && args[1] == "--manifest" && directory::exists(args[2])) {
    return print(daedalus.manifest(args[2]));
  }

  if(args.size() == 3 && args[1] == "--import" && file::exists(args[2])) {
    if(string target = daedalus.import(args[2])) {
      return print(target, "\n");
    }
    return;
  }

  if(args.size() == 2 && args[1] == "--import") {
    if(string source = BrowserDialog()
    .setTitle("Load ROM Image")
    .setPath(settings["daedalus/Path"].text())
    .setFilters("ROM Files|"
      "*.sfc:*.smc:"
      "*.bs:"
      "*.st:"
      "*.zip"
    ).openFile()) {
      if(string target = daedalus.import(source)) {
        settings["daedalus/Path"].setValue(Location::path(source));
        return print(target, "\n");
      }
    }
    return;
  }

  new ScanDialog;
  new SettingsDialog;
  new ImportDialog;
  new ErrorDialog;
  #if defined(PLATFORM_MACOS)
  Application::Cocoa::onAbout([&] {
    MessageDialog().setTitle("About daedalus").setText({
      "daedalus\n\n"
      "Baased on ananke and icarus by byuu"
      "Author: hex_usr\n"
      "License: GPLv3\n"
      "Website: https://board.byuu.org/viewtopic.php?f=8&t=1157\n"
    }).information();
  });
  Application::Cocoa::onPreferences([&] {
    scanDialog->settingsButton.doActivate();
  });
  Application::Cocoa::onQuit([&] {
    Application::quit();
  });
  #endif
  scanDialog->show();
  Application::run();
  settings.save();
}

#endif
