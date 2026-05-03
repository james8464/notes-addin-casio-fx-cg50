#ifndef MAIN_H
#define MAIN_H
#define GEN_PRINT_BUFSIZE 1024-1 // must be < EDIT_LINE_MAX in console.h
#include "giacPCH.h"
#include "kdisplay.h"
extern int xcas_python_eval;
extern "C" int inexammode;
#ifndef MICROPY_LIB
extern "C" {
  void casiostatus(const char * menu,const char * shiftmenu,const char * alphamenu,int color);
  int select_item(const char ** ptr,const char * title,bool askfor1=true);
}
#else
extern "C" {
  void casiostatus(const char * menu,const char * shiftmenu,const char * alphamenu,int color);
  int do_file(const char *file);
  char * micropy_init(int stack_size,int heap_size);
  int micropy_eval(const char * line);
  void  mp_deinit();
  void mp_stack_ctrl_init();
  void process_freeze();
  extern int parser_errorline,parser_errorcol;
  void python_free();
  extern int pythonjs_stack_size,pythonjs_heap_size;
  extern char * python_heap,*pythonjs_static_heap;
  int python_init(int stack_size,int heap_size);
  void console_output(const char *,int );
  const char * console_input(const char * msg1,const char * msg2,bool numeric,int ypos);
  void c_draw_rectangle(int x,int y,int w,int h,int c);
  void c_fill_rect(int x,int y,int w,int h,int c);
  void c_draw_line(int x0,int y0,int x1,int y1,int c);
  void c_draw_circle(int xc,int yc,int r,int color,bool q1,bool q2,bool q3,bool q4);
  void c_draw_filled_circle(int xc,int yc,int r,int color,bool left,bool right);
  void c_draw_polygon(int * x,int *y ,int n,int color);
  void c_draw_filled_polygon(int * x,int *y, int n,int xmin,int xmax,int ymin,int ymax,int color);
  void c_draw_arc(int xc,int yc,int rx,int ry,int color,double theta1, double theta2);
  void c_draw_filled_arc(int x,int y,int rx,int ry,int theta1_deg,int theta2_deg,int color,int xmin,int xmax,int ymin,int ymax,bool segment);
  void c_set_pixel(int x,int y,int c);
  int c_draw_string(int x,int y,int c,int bg,const char * s,bool fake);
  int c_draw_string_small(int x,int y,int c,int bg,const char * s,bool fake);
  int c_draw_string_medium(int x,int y,int c,int bg,const char * s,bool fake);
  int select_item(const char ** ptr,const char * title,bool askfor1=true);
  // C conversion to gen from atomic data type 
  unsigned long long c_double2gen(double); 
  unsigned long long c_int2gen(int);
  // linalg on double matrices
  void doubleptr2matrice(double * x,int n,giac::matrice & m);
  bool matrice2doubleptr(const giac::matrice &M,double *x); // x must have enough space
  bool r_inv(double *,int n);
  bool r_rref(double *,int n,int m);
  double r_det(double *,int);
  struct double_pair {
    double r,i;
  } ;
  typedef struct double_pair c_complex;
  bool matrice2c_complexptr(const giac::matrice &M,c_complex *x);
  void c_complexptr2matrice(c_complex * x,int n,int m,giac::matrice & M);  
  bool c_inv(c_complex *,int n);
  bool c_rref(c_complex *,int n,int m);
  c_complex c_det(c_complex *,int);
  bool c_egv(c_complex * x,int n); // eigenvectors
  bool c_eig(c_complex * x,c_complex * d,int n); // x eigenvect, d reduced mat
  bool c_proot(c_complex * x,int n); // poly root
  bool c_pcoeff(c_complex * x,int n); // root->coeffs
  bool c_fft(c_complex * x,int n,bool inverse); // FFT
  void turtle_freeze();
  void console_freeze();
  void c_sprint_double(char * s,double d);
  void c_turtle_forward(double d);
  void c_turtle_left(double d);
  void c_turtle_up(int i);
  void c_turtle_goto(double x,double y);
  void c_turtle_cap(double x);
  void c_turtle_crayon(int i);
  void c_turtle_rond(int x,int y,int z);
  void c_turtle_disque(int x,int y,int z,int centre);
  void c_turtle_fill(int i);
  void c_turtle_fillcolor(double r,double g,double b,int entier);
  void c_turtle_getposition(double * x,double * y);
  void py_ck_ctrl_c();
  void c_turtle_color(int c);
  int c_turtle_getcolor();
  void c_turtle_clear(int clrpos); // clrpos ignored
  void c_turtle_show(int visible);
  void sync_screen();
  int kbd_filter(int key);  
  void c_turtle_towards(double x,double y);
  int c_turtle_getcap();
  void c_turtle_fillcolor1(int c); // FIXME fake function
}
int micropy_ck_eval(const char *line);
#endif

extern "C" int in_ckgetkey(int *keyptr,int waitforkey,const char * menu,const char *shiftmenu,const char * alphamenu,int menucolorbg);
extern "C" int ck_getkey(int * keyptr);
extern "C" bool oldalphastate,oldshiftstate; // status before last ck_getkey
void do_restart();
int get_free_memory();
extern giac::context * contextptr; 
bool eqws(char * s,bool eval); // from main.cc, s must be at least GEN_PRINT_BUFSIZE char
giac::gen eqw(const giac::gen & ge,bool editable);
int run_script(const char* filename) ;
void erase_script();
void save(const char * fname); // save session
int restore_session(const char * fname);
void displaylogo();
int select_interpreter();
ustl::string remove_path0(const ustl::string & st);
void edit_script(char * fname);
extern "C" char * c_load_script(const char * filename); // returned C string must be free-ed
int load_script(const char * filename,ustl::string & s);
int find_color(const char * s); 
ustl::string khicas_state();
void do_run(const char * s,giac::gen & g,giac::gen & ge);
ustl::string last_ans();
void run(const char * s,int do_logo_graph_eqw=7);
void save_session();
void check_do_graph(giac::gen & ge,const giac::gen & gs,int do_logo_graph_eqw,GIAC_CONTEXT) ;
bool textedit(char * s);
bool textedit(char * s,int bufsize,bool OKparse,const giac::context * contextptr,const char * filename=0);
bool textedit(char * s,int bufsize,const giac::context * contextptr);
bool stringtodouble(const string & s1,double & d);
//void create_data_folder() ;

#endif
