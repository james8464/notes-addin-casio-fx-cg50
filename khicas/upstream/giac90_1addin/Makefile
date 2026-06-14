# debug dans emacs
# lancer l'emulateur
# wine "C:\Program Files (x86)\CASIO\fx-CG Manager PLUS Subscription for fx-CG50series\fx-CG_Manager_PLUS_Subscription_for_fx-CG50series.exe" /n"fx-CG Manager PLUS Subscription for fx-CG50series"
# sh3eb-elf-gdb -i=mi -ex "target remote localhost:31188" khicas.elf
CXX	= sh3eb-elf-g++
AR	= sh3eb-elf-gcc-ar
RANLIB	= sh3eb-elf-gcc-ranlib
OBJCOPY = sh3eb-elf-objcopy

RM	= /bin/rm

# if -DGINT_MALLOC, define GINT_MALLOC in stdlib.c (or Makefile there) and rm kgen.o main.o
CXXFLAGS = -Os -mb -m4a-nofpu  -mhitachi -std=c++11 -fpermissive -flto -fno-inline-functions-called-once -fno-partial-inlining -fno-ipa-cp-clone -fno-use-cxa-atexit -fno-threadsafe-statics -fomit-frame-pointer -fno-strict-aliasing -fno-rtti -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -fmerge-all-constants -ffunction-sections -fdata-sections -DNDEBUG -DHAVE_CONFIG_H -DTIMEOUT -DRELEASE -DCASCAS_DISABLE_PLOT_RUNTIME -DCASCAS_DISABLE_DESOLVE_RUNTIME -DCASCAS_DISABLE_PYTHON_RUNTIME -DCASCAS_DISABLE_MATRIX_EIGEN_RUNTIME -DCASCAS_DISABLE_QUADRATIC_FORM_RUNTIME -DCASCAS_DISABLE_RANDOM_RUNTIME -DCASCAS_DISABLE_RANDOM_MATRIX_RUNTIME -DCASCAS_DISABLE_TURTLE_DRAW_RUNTIME -DCASCAS_DISABLE_FFT_RUNTIME -DCASCAS_DISABLE_VECTOR_CALC_RUNTIME -DCASCAS_DISABLE_UNUSED_PUBLIC_RUNTIME  -I. -I/home/parisse/casiolocal/include/ustl -DFILEICON -DKMALLOC  -DGINT_MALLOC -DTURTLETAB #  #  -DNO_TEMPLATE_MULTGCD -DGIAC_GENERIC_CONSTANTS -fno-zero-initialized-in-bss -ffast-math


LDFLAGS = -static -nostdlib -Tprizm.ld -Wl,--gc-sections,--relax,--print-memory-usage

LIBS = -L. -L/home/parisse/casiolocal/lib -Wl,--start-group -lgui -lcas -ltommath -lustl -lm -lc -lgcc -Wl,--end-group
RUNMAT_CXXFLAGS = -Os -mb -m4a-nofpu -mhitachi -std=c++11 -fno-use-cxa-atexit -fno-threadsafe-statics -fomit-frame-pointer -fno-strict-aliasing -fno-rtti -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -I/home/parisse/casiolocal/include -I/home/parisse/casiolocal/include/fxcg
RUNMAT_LDFLAGS = -static -nostdlib -Tprizm.ld -Wl,--gc-sections,--relax,--print-memory-usage,--allow-multiple-definition
RUNMAT_LIBS = -L/home/parisse/casiolocal/lib -Wl,--start-group -lfxcg -lc -lgcc -Wl,--end-group
LINK_TMP = /tmp/casio-link

CAS_OBJS = ksym2poly.o kgausspol.o kthreaded.o kcsturm.o kmaple.o krpn.o kmoyal.o kmisc.o kpermu.o kdesolve.o input_parser.o ksymbolic.o index.o kmodpoly.o kmodfactor.o kezgcd.o kderive.o ksolve.o kintg.o kintgab.o klin.o kseries.o ksubst.o kvecteur.o kglobal.o kifactor.o kalg_ext.o kgauss.o khelp_min.o kti89.o kplot.o kprog.o kunary.o kusual.o kidentificateur.o kgen.o krisch.o input_lexer.o first.o # memmgr.o mem.o

# old console: kmisc.cc should be compiled with -DOLD_CONSOLE
#GUI_OBJS = fileGUI.o inputGUI.o menuGUI.o textGUI.o fileProvider.o graphicsProvider.o stringsProvider.o history.o kdisplay.o dConsole.o dmain.o
# new console
GUI_OBJS = fileGUI.o menuGUI.o textGUI.o fileProvider.o graphicsProvider.o stringsProvider.o kdisplay.o console.o cascas_working.o main.o

.PRECIOUS: libcas.a libgui.a

all: khicasen.g3a khicas.g3a  # pour la version console, supprimer le lien iostream -> iostream.new

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

runmat_mock.o: runmat_mock.cc
	$(CXX) $(RUNMAT_CXXFLAGS) -c $<

casio_p3_app.o: p3_app.cc p3_engine.hpp casio_suite_ui.hpp
	$(CXX) $(RUNMAT_CXXFLAGS) -c $< -o $@

p3_engine.o: p3_engine.cpp p3_engine.hpp
	$(CXX) $(RUNMAT_CXXFLAGS) -c $< -o $@

casio_cscalc_app.o: cscalc_app.cc cscalc_engine.hpp casio_suite_ui.hpp
	$(CXX) $(RUNMAT_CXXFLAGS) -c $< -o $@

cscalc_engine.o: cscalc_engine.cpp cscalc_engine.hpp
	$(CXX) $(RUNMAT_CXXFLAGS) -c $< -o $@

casio_notes_app.o: notes_app.cc casio_suite_ui.hpp
	$(CXX) $(RUNMAT_CXXFLAGS) -c $< -o $@

libgui.a: $(GUI_OBJS)
	$(RM) -f $@
	$(AR) cru $@ $^
	$(RANLIB) $@

libcas.a: $(CAS_OBJS)
	$(RM) -f $@
	$(AR) cru $@ $^
	$(RANLIB) $@

khicas.elf: libcas.a libgui.a catalogfr.o 
	mkdir -p $(LINK_TMP)
	$(CXX) $(LDFLAGS) -Wl,-Map=$(LINK_TMP)/khicas.map catalogfr.o $(LIBS) -o $(LINK_TMP)/$@
	cp $(LINK_TMP)/$@ $@
	cp $(LINK_TMP)/khicas.map khicas.map
	sh3eb-elf-objdump -C -t khicas.elf | sort > dump_t

khicasen.elf: libcas.a libgui.a catalogen.o 
	mkdir -p $(LINK_TMP)
	$(CXX) $(LDFLAGS) -Wl,-Map=$(LINK_TMP)/khicasen.map catalogen.o  $(LIBS) -o $(LINK_TMP)/$@
	cp $(LINK_TMP)/$@ $@
	cp $(LINK_TMP)/khicasen.map khicasen.map
	sh3eb-elf-objdump -C -t khicasen.elf | sort > dumpen_t

OBJFLAGS= -O binary -R .bss -R .gint_bss -R .debug_info -R .debug_abbrev -R .debug_loc -R .debug_aranges -R .debug_ranges -R .debug_line -R .debug_str -R.comment -R  debug_frame
khicas.bin: khicas.elf
	$(OBJCOPY) $(OBJFLAGS) khicas.elf khicas.bin

khicasen.bin: khicasen.elf
	$(OBJCOPY) $(OBJFLAGS)  khicasen.elf khicasen.bin

khicas.g3a: khicas.bin
	mkg3a -n basic:Khicas -n internal:KHICAS -V 1.5.2 -i uns:khicasio.png -i sel:khicasio1.png $^ $@
	/bin/cp khicas.g3a ~/.wine/drive_c

khicasen.g3a: khicasen.bin
	mkg3a -n basic:Khicasen -n internal:KHICASEN -V 1.5.2 -i uns:khicasio.png -i sel:khicasio1.png $^ $@
	/bin/cp khicasen.g3a ~/.wine/drive_c

CAS.g3a: khicasen.bin cas_icon.png cas_icon_selected.png
	mkg3a -n basic:CAS -n internal:CAS -V 1.5.2 -i uns:cas_icon.png -i sel:cas_icon_selected.png khicasen.bin $@
	/bin/cp CAS.g3a ~/.wine/drive_c

runmat_mock.elf: runmat_mock.o
	mkdir -p $(LINK_TMP)
	$(CXX) $(RUNMAT_LDFLAGS) -Wl,-Map=$(LINK_TMP)/runmat_mock.map runmat_mock.o $(RUNMAT_LIBS) -o $(LINK_TMP)/$@
	cp $(LINK_TMP)/$@ $@
	cp $(LINK_TMP)/runmat_mock.map runmat_mock.map

runmat_mock.bin: runmat_mock.elf
	$(OBJCOPY) $(OBJFLAGS) runmat_mock.elf runmat_mock.bin

RUNMAT.g3a: runmat_mock.bin runmat_icon.png runmat_icon_selected.png
	mkg3a -n basic:RunMat -n internal:RUNMAT -V 1.0.0 -i uns:runmat_icon.png -i sel:runmat_icon_selected.png runmat_mock.bin $@
	/bin/cp RUNMAT.g3a ~/.wine/drive_c

CASP3.elf: casio_p3_app.o p3_engine.o
	mkdir -p $(LINK_TMP)
	$(CXX) $(RUNMAT_LDFLAGS) -Wl,-Map=$(LINK_TMP)/CASP3.map casio_p3_app.o p3_engine.o $(RUNMAT_LIBS) -o $(LINK_TMP)/$@
	cp $(LINK_TMP)/$@ $@
	cp $(LINK_TMP)/CASP3.map CASP3.map

CASP3.bin: CASP3.elf
	$(OBJCOPY) $(OBJFLAGS) CASP3.elf CASP3.bin

CASP3.g3a: CASP3.bin casp3_icon.png casp3_icon_selected.png
	mkg3a -n basic:CASP3 -n internal:CASP3 -V 1.0.0 -i uns:casp3_icon.png -i sel:casp3_icon_selected.png CASP3.bin $@
	/bin/cp CASP3.g3a ~/.wine/drive_c

CSCALC.elf: casio_cscalc_app.o cscalc_engine.o
	mkdir -p $(LINK_TMP)
	$(CXX) $(RUNMAT_LDFLAGS) -Wl,-Map=$(LINK_TMP)/CSCALC.map casio_cscalc_app.o cscalc_engine.o $(RUNMAT_LIBS) -o $(LINK_TMP)/$@
	cp $(LINK_TMP)/$@ $@
	cp $(LINK_TMP)/CSCALC.map CSCALC.map

CSCALC.bin: CSCALC.elf
	$(OBJCOPY) $(OBJFLAGS) CSCALC.elf CSCALC.bin

CSCALC.g3a: CSCALC.bin cscalc_icon.png cscalc_icon_selected.png
	mkg3a -n basic:CSCalc -n internal:CSCALC -V 1.0.0 -i uns:cscalc_icon.png -i sel:cscalc_icon_selected.png CSCALC.bin $@
	/bin/cp CSCALC.g3a ~/.wine/drive_c

NOTES.elf: casio_notes_app.o
	mkdir -p $(LINK_TMP)
	$(CXX) $(RUNMAT_LDFLAGS) -Wl,-Map=$(LINK_TMP)/NOTES.map casio_notes_app.o $(RUNMAT_LIBS) -o $(LINK_TMP)/$@
	cp $(LINK_TMP)/$@ $@
	cp $(LINK_TMP)/NOTES.map NOTES.map

NOTES.bin: NOTES.elf
	$(OBJCOPY) $(OBJFLAGS) NOTES.elf NOTES.bin

NOTES.g3a: NOTES.bin notes_icon.png notes_icon_selected.png
	mkg3a -n basic:Notes -n internal:NOTES -V 1.0.0 -i uns:notes_icon.png -i sel:notes_icon_selected.png NOTES.bin $@
	/bin/cp NOTES.g3a ~/.wine/drive_c

clean:
	$(RM) -f *.o libcas.a libgui.a 
