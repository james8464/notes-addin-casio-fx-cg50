#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/c++/khicas/upstream/giac90_1addin"
OUT_DIR="${ROOT_DIR}/c++/khicas/host"
IMAGE_TAG="${CASIO_KHICAS_SOURCE_IMAGE:-casio-khicas-source:latest}"
HOST_GMP_H="${CASIO_HOST_GMP_H:-}"

if [[ -z "${HOST_GMP_H}" ]]; then
  for p in /opt/homebrew/include/gmp.h /usr/local/include/gmp.h /private/tmp/gmp.h; do
    if [[ -f "${p}" ]]; then
      HOST_GMP_H="${p}"
      break
    fi
  done
fi
if [[ -z "${HOST_GMP_H}" || ! -f "${HOST_GMP_H}" ]]; then
  echo "Missing gmp.h. Set CASIO_HOST_GMP_H=/path/to/gmp.h" >&2
  exit 1
fi
if [[ "${HOST_GMP_H}" != /private/tmp/* ]]; then
  cp "${HOST_GMP_H}" /private/tmp/cascas_khicas_gmp.h
  HOST_GMP_H=/private/tmp/cascas_khicas_gmp.h
fi

mkdir -p "${OUT_DIR}"
docker run --rm \
  --platform linux/amd64 \
  -v "${ROOT_DIR}:/work" \
  -v "${HOST_GMP_H}:/host_gmp/gmp.h:ro" \
  -w /work \
  "${IMAGE_TAG}" \
  bash -lc '
    set -euo pipefail
    SRC=/work/c++/khicas/upstream/giac90_1addin
    TMP=/tmp/khicas_host_src
    BUILD=/work/c++/khicas/host/build
    rm -rf "$TMP" "$BUILD"
    mkdir -p "$TMP" "$BUILD" /tmp/khicas_host_include/gint /tmp/khicas_host_include/fxcg
    cp -a "$SRC"/. "$TMP"/
    cp /host_gmp/gmp.h /tmp/khicas_host_include/gmp.h

    cat > /tmp/khicas_host_include/gint/kmalloc.h <<'"'"'H'"'"'
#pragma once
H
    cat > /tmp/khicas_host_include/fxcg/keyboard.h <<'"'"'H'"'"'
#pragma once
#define KEY_CTRL_EXIT 30001
#define KEY_CTRL_LEFT 30002
#define KEY_CTRL_RIGHT 30003
#define KEY_CTRL_UP 30004
#define KEY_CTRL_DOWN 30005
#define KEY_CTRL_EXE 30006
#define KEY_CTRL_AC 30007
#define KEY_CTRL_F1 30008
#define KEY_CTRL_F2 30009
#define KEY_CTRL_F3 30010
#define KEY_CTRL_F4 30011
#define KEY_CTRL_F5 30012
#define KEY_CTRL_F6 30013
extern "C" void ck_getkey(int *key);
extern "C" void GetKey(int *key);
extern "C" int GetKeyWait_OS(int*,int*,int,int,int,int*);
H
    cat > /tmp/khicas_host_include/fxcg/display.h <<'"'"'H'"'"'
#pragma once
#define LCD_WIDTH_PX 384
#define LCD_HEIGHT_PX 216
#define STATUS_AREA_PX 24
#define COLOR_BLACK 0
#define COLOR_WHITE 65535
#define TEXT_COLOR_BLACK 0
typedef unsigned short color_t;
extern "C" void *GetVRAMAddress();
extern "C" void Bdisp_AllClr_VRAM();
extern "C" void Bdisp_PutDisp_DD();
extern "C" void Bdisp_EnableColor(int);
extern "C" void Bdisp_MMPrint(int,int,const char*,int,unsigned int,int,int,int,int,int,int);
extern "C" void PrintXY(int,int,char*,int,int);
extern "C" void PrintMini(int*,int*,unsigned char*,int,unsigned int,int,int,int,int,int,int);
extern "C" void PrintMiniMini(int*,int*,const void*,int,int,int);
H
    cat > /tmp/khicas_host_include/fxcg/file.h <<'"'"'H'"'"'
#pragma once
#define READ 1
#define READWRITE 2
#define CREATEMODE_FILE 1
#define CREATEMODE_FOLDER 5
extern "C" void Bfile_StrToName_ncpy(unsigned short*, const unsigned char*, int);
extern "C" int Bfile_OpenFile_OS(const unsigned short*, int);
extern "C" int Bfile_CreateEntry_OS(const unsigned short*, int, int*);
extern "C" int Bfile_WriteFile_OS(int, const void*, int);
extern "C" int Bfile_CloseFile_OS(int);
H

    cat >> "$TMP/config.h" <<'"'"'H'"'"'
#ifdef CASCAS_HOST_RUNNER
#undef FXCG
#undef USTL
#undef std
#undef HAVE_LIBTOMMATH
#undef USE_GMP_REPLACEMENTS
#undef HAVE_NO_CTYPE_H
#undef HAVE_NO_PWD_H
#undef HAVE_NO_SYS_RESOURCE_WAIT_H
#undef HAVE_NO_HOME_DIRECTORY
#undef HAVE_NO_SYS_TIMES_H
#undef HAVE_NO_SYS_TIME_H
#undef HAVE_NO_SYS_IOCTL_H
#undef HAVE_NO_TERMIOS_H
#undef HAVE_NO_SYSLOG_H
#define stdostream std::ostream
namespace ustl = std;
namespace giac { class gen; class context; extern gen vx_var; extern volatile char ctrl_c; extern volatile char interrupted; bool msieve(const gen &,gen &,const context *); }
extern "C" void OS_InnerWait_ms(int);
extern "C" int RTC_GetTicks();
extern "C" void RTC_SetDateTime(unsigned char *);
void sprint_int(char *, int);
extern "C" void ck_getkey(int *);
#ifndef KEY_CTRL_EXIT
#define KEY_CTRL_EXIT 30001
#define KEY_CTRL_LEFT 30002
#define KEY_CTRL_RIGHT 30003
#define KEY_CTRL_UP 30004
#define KEY_CTRL_DOWN 30005
#define KEY_CTRL_F1 30008
#endif
#ifndef LCD_WIDTH_PX
#define LCD_WIDTH_PX 384
#define LCD_HEIGHT_PX 216
#define STATUS_AREA_PX 24
#define COLOR_BLACK 0
#define COLOR_WHITE 65535
#define TEXT_COLOR_BLACK 0
#endif
#ifndef READWRITE
#define READ 1
#define READWRITE 2
#define CREATEMODE_FILE 1
#define CREATEMODE_FOLDER 5
#endif
extern "C" void Bfile_StrToName_ncpy(unsigned short*, const unsigned char*, int);
extern "C" int Bfile_OpenFile_OS(const unsigned short*, int);
extern "C" int Bfile_CreateEntry_OS(const unsigned short*, int, int*);
extern "C" int Bfile_WriteFile_OS(int, const void*, int);
extern "C" int Bfile_CloseFile_OS(int);
#define vx_var() vx_var
#endif
H

    perl -0pi -e "s@#if 0 //ndef GIAC_VECTOR@#if 1 // CASCAS_HOST_RUNNER@" "$TMP/global.h"
    perl -0pi -e "s@#ifdef DOUBLEVAL\n#define define_alias_gen.*?#else // DOUBLEVAL@#ifdef DOUBLEVAL\n#define define_alias_gen(name,type,subtype,ptr) alias_gen name={ulonglong(ptr),0,subtype,type};\n#define define_alias_ref_symbolic(name,sommet,type,subtype,ptr) alias_ref_symbolic name={-1,(unary_function_eval *)sommet,ulonglong(ptr),0,subtype,type};\n#define define_alias_ref_fraction(name,numtype,numsubtype,numptr,dentype,densubtype,denptr) alias_ref_fraction name={-1,{ulonglong(numptr),0,numsubtype,numtype},{ulonglong(denptr),0,densubtype,dentype}};\n#define define_alias_ref_complex(name,retype,resubtype,reptr,imtype,imsubtype,imptr) alias_ref_complex name={-1,0,{ulonglong(reptr),0,resubtype,retype},{ulonglong(imptr),0,imsubtype,imtype}};\n#define define_tab2_alias_gen(name,retype,resubtype,reptr,imtype,imsubtype,imptr) alias_gen name[]={{ulonglong(reptr),0,resubtype,retype},{ulonglong(imptr),0,imsubtype,imtype}};\n#else // DOUBLEVAL@s" "$TMP/gen.h"
    perl -0pi -e "s/int res=inputline\\(\"EXE: ok, EXIT: break\",args\\.type==_STRNG\\?\\(\\*args\\._STRNGptr\\):\"input \\?\",s,false,142\\);/std::string prompt=args.type==_STRNG?(*args._STRNGptr):std::string(\"input ?\");\n    int res=inputline(\"EXE: ok, EXIT: break\",prompt.c_str(),s,false,142);/" "$TMP/kmisc.cc"
    perl -0pi -e "s/qsort\\(v\\.begin\\(\\),/qsort(\\&v.front(),/" "$TMP/kprog.cc"
    perl -0pi -e "s/Bfile_WriteFile_OS\\(hFile, res, res\\.size\\(\\)\\);/Bfile_WriteFile_OS(hFile, res.c_str(), res.size());/" "$TMP/kprog.cc"
    perl -0pi -e "s@    mp_digit C; \n    mp_mod_d\\(\\(mp_int \\*\\)&a,b,&C\\);\n    return C;@    return mpz_fdiv_ui(a,b);@s" "$TMP/kifactor.cc"
    perl -0pi -e "s/#if !defined RTOS_THREADX && !defined NSPIRE && !defined FXCG/#if !defined RTOS_THREADX && !defined NSPIRE && !defined FXCG && !defined CASCAS_HOST_RUNNER/" "$TMP/usual.h"
    perl -0pi -e "s@\\(\\(\\(size_t\\) _VECTptr->begin\\(\\)\\) &stack_mask\\)@(((size_t) \\&_VECTptr->front()) \\&stack_mask)@g" "$TMP/kgen.cc"
    perl -0pi -e "s@#ifndef NSPIRE\n  ostream \\& operator << \\(ostream \\& os,const gen \\& a\\)@#if !defined NSPIRE \\&\\& !defined CASCAS_HOST_RUNNER\n  ostream \\& operator << (ostream \\& os,const gen \\& a)@" "$TMP/kgen.cc"
    perl -0pi -e "s/=mpz_get_si\\(/=(longlong)mpz_get_si(/g; s/= mpz_get_si\\(/= (longlong)mpz_get_si(/g" "$TMP/kgen.cc"
    perl -0pi -e "s/gen vx_var\\(x__IDNT_e\\);/gen (vx_var)(x__IDNT_e);/" "$TMP/kidentificateur.cc"

    cp /work/c++/tools/khicas_host_runner/host_runner.cc "$TMP/host_runner.cc"

    CAS_OBJS="ksym2poly kgausspol kthreaded kcsturm kmaple krpn kmoyal kmisc kpermu kdesolve input_parser ksymbolic index kmodpoly kmodfactor kezgcd kderive ksolve kintg kintgab klin kseries ksubst kvecteur kglobal kifactor kalg_ext kgauss khelp kti89 kplot kprog kunary kusual kidentificateur kgen krisch input_lexer first"
    CXXFLAGS="-std=gnu++98 -fpermissive -DHAVE_CONFIG_H -O0 -fsanitize=address -fno-omit-frame-pointer -DCASCAS_HOST_RUNNER -I/tmp/khicas_host_include -I/tmp/khicas_host_src"
    for o in $CAS_OBJS; do
      echo "CXX $o"
      g++ $CXXFLAGS -c "$TMP/$o.cc" -o "$BUILD/$o.o"
    done
    g++ $CXXFLAGS -c "$TMP/host_runner.cc" -o "$BUILD/host_runner.o"
    g++ -fsanitize=address -Wl,--allow-multiple-definition "$BUILD"/*.o /usr/lib/x86_64-linux-gnu/libgmp.so.10 -lm -o "$BUILD/khicas_host_runner"
    "$BUILD/khicas_host_runner" "1+1" "diff(x^3,x)" "solve(x^2-1,x)"
  '

echo "Built Docker/Linux KhiCAS source runner: ${OUT_DIR}/build/khicas_host_runner"
