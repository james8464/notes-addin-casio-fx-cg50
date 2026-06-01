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
CXXFLAGS = -Os -mb -m4a-nofpu  -mhitachi -std=c++11 -fpermissive -flto -fno-use-cxa-atexit -fno-strict-aliasing -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -DHAVE_CONFIG_H -DTIMEOUT -DRELEASE -DCASCAS_DISABLE_PYTHON_TRANSLATE -DCASCAS_DISABLE_PLOT_RUNTIME  -I. -I/home/parisse/casiolocal/include/ustl -DKMALLOC  -DGINT_MALLOC #  #  -DNO_TEMPLATE_MULTGCD -DGIAC_GENERIC_CONSTANTS -fno-zero-initialized-in-bss -ffast-math


LDFLAGS = -g -static -nostdlib -Tprizm.ld -Wl,--gc-sections,--print-memory-usage

LIBS = -L. -L/home/parisse/casiolocal/lib -Wl,--start-group -lgui -lcas -ltommath -lustl -lm -lc -lgcc -Wl,--end-group

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

libgui.a: $(GUI_OBJS)
	$(RM) -f $@
	$(AR) cru $@ $^
	$(RANLIB) $@

libcas.a: $(CAS_OBJS)
	$(RM) -f $@
	$(AR) cru $@ $^
	$(RANLIB) $@

khicas.elf: libcas.a libgui.a catalogfr.o 
	$(CXX) $(LDFLAGS) -Wl,-Map=khicas.map catalogfr.o $(LIBS) -o $@
	sh3eb-elf-objdump -C -t khicas.elf | sort > dump_t

khicasen.elf: libcas.a libgui.a catalogen.o 
	$(CXX) $(LDFLAGS) -Wl,-Map=khicasen.map catalogen.o  $(LIBS) -o $@
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

CAS.g3a: khicasen.bin
	mkg3a -n basic:CAS -n internal:CAS -V 1.5.2 -i uns:khicasio.png -i sel:khicasio1.png $^ $@
	/bin/cp CAS.g3a ~/.wine/drive_c

clean:
	$(RM) -f *.o libcas.a libgui.a 
