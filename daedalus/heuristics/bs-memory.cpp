struct BSMemoryCartridge {
  BSMemoryCartridge(const uint8_t* data, uint size);

  string markup;
};

BSMemoryCartridge::BSMemoryCartridge(const uint8_t* data, uint size) {
  markup.append("cartridge\n");
  markup.append("  rom name=program.rom size=0x", hex(size), " type=FlashROM\n");
}
