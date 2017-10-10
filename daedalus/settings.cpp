struct Settings : Markup::Node {
  Settings();
  auto save() -> void;
};

Settings::Settings() {
  Markup::Node::operator=(BML::unserialize(string::read(locate("settings.bml"))));

  auto set = [&](const string& name, const string& value) {
    //create node and set to default value only if it does not already exist
    if(!operator[](name)) operator()(name).setValue(value);
  };

  set("Library/Location", {Path::user(), "Emulation/"});

  set("daedalus/Path", Path::user());
}

auto Settings::save() -> void {
  file::write(locate("settings.bml"), BML::serialize(*this));
}
