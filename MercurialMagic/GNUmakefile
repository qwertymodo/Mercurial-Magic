build := release
include ../nall/GNUmakefile
include ../hiro/GNUmakefile

name := MercurialMagic
flags += -I. -I..
link +=

ifeq ($(platform),windows)
  link += $(if $(call streq,$(console),true),-mconsole,-mwindows)
endif

objects := obj/hiro.o
objects += obj/program.o
objects += $(if $(call streq,$(platform),windows),obj/resource.o)

all: $(objects)
	$(strip $(compiler) -o out/$(name) $(objects) $(link) $(hirolink))
ifeq ($(platform),macos)
	@if [ -d out/$(name).app ]; then rm -r out/$(name).app; fi
	mkdir -p out/$(name).app/Contents/MacOS/
	mkdir -p out/$(name).app/Contents/Resources/
	mv out/$(name) out/$(name).app/Contents/MacOS/$(name)
	cp data/$(name).plist out/$(name).app/Contents/Info.plist
	sips -s format icns data/$(name).png --out out/$(name).app/Contents/Resources/$(name).icns
endif

obj/hiro.o: ../hiro/hiro.cpp
	$(compiler) $(hiroflags) -o obj/hiro.o -c ../hiro/hiro.cpp

obj/program.o: *.cpp
	$(compiler) $(cppflags) $(flags) -o obj/program.o -c program.cpp

obj/resource.o:
	$(windres) data/$(name).rc obj/resource.o

clean:
	-@$(call delete,obj/*.o)
	-@$(call delete,out/*.exe)

install:
ifeq ($(platform),macos)
	cp -R out/$(name).app /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/$(name)/Database/
	if [ -f out/$(name) ]; then cp out/$(name) $(prefix)/bin/$(name); fi
	cp -R Database/* $(prefix)/share/$(name)/Database/
	cp data/$(name).desktop $(prefix)/share/applications/$(name).desktop
	cp data/$(name).png $(prefix)/share/icons/$(name).png
endif

uninstall:
ifeq ($(platform),macos)
	if [ -d /Applications/$(name).app ]; then rm -r /Applications/$(name).app; fi
else ifneq ($(filter $(platform),linux bsd),)
	if [ -f $(prefix)/bin/$(name) ]; then rm $(prefix)/bin/$(name); fi
	if [ -f $(prefix)/share/applications/$(name).desktop ]; then rm $(prefix)/share/applications/$(name).desktop; fi
	if [ -f $(prefix)/share/icons/$(name).png ]; then rm $(prefix)/share/icons/$(name).png; fi
endif
