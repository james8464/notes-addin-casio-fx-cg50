#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <unistd.h>

#include "giacPCH.h"
#include "prog.h"
#include "cocoa.h"
#include "mathml.h"
#include "tinymt32.h"

static unsigned short fake_vram[384 * 216];
int line_start = 0;

void sprint_int(char *s, int i) { std::sprintf(s, "%d", i); }
int confirm(const char *, const char *, bool) { return KEY_CTRL_F1; }
void tinymt32_init(tinymt32_t *random, uint32_t seed) {
  if (random)
    random->status[0] = seed;
}

namespace giac {
std::map<std::string, context *> *context_names = 0;
const unary_function_ptr *const at_archive = 0;
const unary_function_ptr *const at_mathml = 0;
const unary_function_ptr *const at_Kronecker = 0;

void clear_abort() {}
void set_abort() {}
bool msieve(const gen &, gen &, const context *) { return false; }
vecteur *keywords_vecteur_ptr() {
  static vecteur v;
  return &v;
}
gen _mathml(const gen &g, GIAC_CONTEXT) { return g; }
bool gbasis8(const vectpoly &, order_t &, vectpoly &, environment *, bool, bool,
             int &, GIAC_CONTEXT, bool) {
  return false;
}
bool greduce8(const vectpoly &, const vectpoly &, order_t &, vectpoly &,
              environment *, GIAC_CONTEXT) {
  return false;
}
bool is_graphe(const gen &, std::string &, GIAC_CONTEXT) { return false; }

real_object::real_object() { mpf_init(inf); }
real_object::real_object(double d) { mpf_init_set_d(inf, d); }
real_object::real_object(const mpf_t &d) { mpf_init_set(inf, d); }
real_object::real_object(const gen &) { mpf_init(inf); }
real_object::real_object(const gen &, unsigned int) { mpf_init(inf); }
real_object::real_object(const real_object &g) { mpf_init_set(inf, g.inf); }
real_object &real_object::operator=(const real_object &g) {
  if (this != &g)
    mpf_set(inf, g.inf);
  return *this;
}
std::string real_object::print(GIAC_CONTEXT) const {
  char *s = 0;
  gmp_asprintf(&s, "%Ff", inf);
  std::string out = s ? s : "0";
  if (s)
    std::free(s);
  return out;
}
gen real_object::addition(const gen &, GIAC_CONTEXT) const { return gen(0); }
gen real_object::multiply(const gen &, GIAC_CONTEXT) const { return gen(0); }
gen real_object::divide(const gen &, GIAC_CONTEXT) const { return gen(0); }
gen real_object::substract(const gen &, GIAC_CONTEXT) const { return gen(0); }
gen real_object::operator+(const real_object &) const { return gen(0); }
gen real_object::operator*(const real_object &) const { return gen(0); }
gen real_object::operator/(const real_object &) const { return gen(0); }
gen real_object::operator-(const real_object &) const { return gen(0); }
gen real_object::operator-() const { return gen(0); }
gen real_object::inv() const { return gen(0); }
gen real_object::sqrt() const { return gen(0); }
gen real_object::abs() const { return gen(0); }
gen real_object::exp() const { return gen(0); }
gen real_object::log() const { return gen(0); }
gen real_object::sin() const { return gen(0); }
gen real_object::cos() const { return gen(0); }
gen real_object::tan() const { return gen(0); }
gen real_object::sinh() const { return gen(0); }
gen real_object::cosh() const { return gen(0); }
gen real_object::tanh() const { return gen(0); }
gen real_object::asin() const { return gen(0); }
gen real_object::acos() const { return gen(0); }
gen real_object::atan() const { return gen(0); }
gen real_object::asinh() const { return gen(0); }
gen real_object::acosh() const { return gen(0); }
gen real_object::atanh() const { return gen(0); }
bool real_object::is_zero() const { return mpf_sgn(inf) == 0; }
bool real_object::maybe_zero() const { return is_zero(); }
bool real_object::is_inf() const { return false; }
bool real_object::is_nan() const { return false; }
int real_object::is_positive() const { return mpf_sgn(inf) > 0; }
double real_object::evalf_double() const { return mpf_get_d(inf); }
gen::gen(const real_object &) { *this = gen(0); }
} // namespace giac

extern "C" void dConsolePut(const char *) {}
extern "C" void dConsolePutChar(char) {}
extern "C" void dConsoleRedraw() {}
extern "C" void OS_InnerWait_ms(int) {}
extern "C" int RTC_GetTicks() { return 1; }
extern "C" void RTC_SetDateTime(unsigned char *) {}
extern "C" void ck_getkey(int *key) {
  if (key)
    *key = KEY_CTRL_EXIT;
}
extern "C" void GetKey(int *key) {
  if (key)
    *key = KEY_CTRL_EXIT;
}
extern "C" int GetKeyWait_OS(int *, int *, int, int, int, int *key) {
  if (key)
    *key = KEY_CTRL_EXIT;
  return 0;
}
extern "C" void *GetVRAMAddress() { return fake_vram; }
extern "C" void Bdisp_AllClr_VRAM() {}
extern "C" void Bdisp_PutDisp_DD() {}
extern "C" void Bdisp_EnableColor(int) {}
extern "C" void Bdisp_MMPrint(int, int, const char *, int, unsigned int, int,
                              int, int, int, int, int) {}
extern "C" void PrintXY(int, int, char *, int, int) {}
extern "C" void PrintMini(int *, int *, unsigned char *, int, unsigned int,
                          int, int, int, int, int, int) {}
extern "C" void PrintMiniMini(int *, int *, const void *, int, int, int) {}
extern "C" void Bfile_StrToName_ncpy(unsigned short *dst,
                                      const unsigned char *src, int n) {
  for (int i = 0; i < n; i++)
    dst[i] = src[i];
}
extern "C" int Bfile_OpenFile_OS(const unsigned short *, int) { return -1; }
extern "C" int Bfile_CreateEntry_OS(const unsigned short *, int, int *) {
  return -1;
}
extern "C" int Bfile_WriteFile_OS(int, const void *, int) { return -1; }
extern "C" int Bfile_CloseFile_OS(int) { return 0; }

int inputline(const char *, const char *, std::string &, bool, int) { return 0; }

int main(int argc, char **argv) {
  giac::context *ctx = new giac::context;
  giac::python_compat(true, ctx);
  for (int i = 1; i < argc; i++) {
    giac::gen g(argv[i], ctx);
    giac::gen ge = giac::eval(giac::equaltosto(g, ctx), 1, ctx);
    std::cout << ge.print(ctx) << "\n";
  }
  std::cout.flush();
  _exit(0);
}
