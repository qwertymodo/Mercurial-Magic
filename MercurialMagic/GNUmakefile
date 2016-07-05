include ../nall/GNUmakefile
include ../hiro/GNUmakefile

flags += -I. -I.. -O3
link +=

ifeq ($(platform),windows)
  ifeq ($(console),true)
    link += -mconsole
  else
    link += -mwindows
  endif
endif

objects := obj/hiro.o
objects += obj/MercurialMagic.o obj/export-settings.o
objects += $(if $(call streq,$(platform),windows),obj/resource.o)

all: $(objects)
	$(compiler) -o out/MercurialMagic $(objects) $(link) $(hirolink)

obj/hiro.o: ../hiro/hiro.cpp
	$(compiler) $(hiroflags) -o obj/hiro.o -c ../hiro/hiro.cpp

obj/MercurialMagic.o: MercurialMagic.cpp
	$(compiler) $(cppflags) $(flags) -o obj/MercurialMagic.o -c MercurialMagic.cpp

obj/export-settings.o: export-settings.cpp
	$(compiler) $(cppflags) $(flags) -o obj/export-settings.o -c export-settings.cpp

obj/resource.o:
	windres ../hiro/windows/hiro.rc obj/resource.o

clean:
	-@$(call delete,obj/*.o)
	-@$(call delete,out/*.exe)
