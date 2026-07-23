CXX	= sh3eb-elf-g++
OBJCOPY = sh3eb-elf-objcopy
RM	= /bin/rm

CXXFLAGS = -Os -mb -m4a-nofpu -mhitachi -std=c++11 -fno-use-cxa-atexit \
  -fno-threadsafe-statics -fomit-frame-pointer -fno-strict-aliasing \
  -fno-rtti -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -ffunction-sections -fdata-sections \
  -I/home/parisse/casiolocal/include \
  -I/home/parisse/casiolocal/include/fxcg \
  -I/home/parisse/casiolocal/include/ustl

LDFLAGS = -static -nostdlib -Tprizm.ld \
  -Wl,--gc-sections,--relax,--print-memory-usage,--allow-multiple-definition

LIBS = -L/home/parisse/casiolocal/lib \
  -Wl,--start-group -lfxcg -lc -lgcc -Wl,--end-group

LINK_TMP = /tmp/casio-link

notes_app.o: notes_app.cc casio_suite_ui.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

NOTES.elf: notes_app.o
	mkdir -p $(LINK_TMP)
	$(CXX) $(LDFLAGS) -Wl,-Map=$(LINK_TMP)/NOTES.map notes_app.o $(LIBS) -o $(LINK_TMP)/$@
	cp $(LINK_TMP)/$@ $@
	cp $(LINK_TMP)/NOTES.map NOTES.map

NOTES.bin: NOTES.elf
	$(OBJCOPY) -O binary -R .bss -R .gint_bss -R .debug_info -R .debug_abbrev \
	  -R .debug_loc -R .debug_aranges -R .debug_ranges -R .debug_line \
	  -R .debug_str -R .comment -R debug_frame NOTES.elf NOTES.bin

NOTES.g3a: NOTES.bin notes_icon.png notes_icon_selected.png
	mkg3a -n basic:Notes -n internal:NOTES -V 1.0.0 \
	  -i uns:notes_icon.png -i sel:notes_icon_selected.png NOTES.bin $@

clean:
	$(RM) -f *.o *.a *.elf *.bin *.g3a *.map
