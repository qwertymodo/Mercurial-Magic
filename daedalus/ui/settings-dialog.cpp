SettingsDialog::SettingsDialog() {
  settingsDialog = this;

  layout.setMargin(5);
  locationLabel.setText("Library Location:");
  locationEdit.setEditable(false).setText(settings["Library/Location"].text());
  changeLocationButton.setText("Change ...").onActivate([&] {
    if(auto location = BrowserDialog().setParent(*this).setTitle("Select Library Location").selectFolder()) {
      settings["Library/Location"].setValue(location);
      locationEdit.setText(location);
    }
  });

  setTitle("daedalus Settings");
  setSize({480, layout.minimumSize().height()});
}
