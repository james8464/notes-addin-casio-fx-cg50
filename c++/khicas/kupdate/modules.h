#ifndef MODULES_H
#define MODULES_H
#include <string>

extern "C" {
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
  int os_get_pixel(int x,int y);
  int c_draw_string(int x,int y,int c,int bg,const char * s,bool fake);
  int c_draw_string_small(int x,int y,int c,int bg,const char * s,bool fake);
  int c_draw_string_medium(int x,int y,int c,int bg,const char * s,bool fake);
  struct double_pair {
    double r,i;
    double_pair operator +=(const double_pair &);
    double_pair operator -=(const double_pair &);
  } ;
  typedef struct double_pair c_complex;
  bool c_inv(c_complex *,int n);
  bool c_rref(c_complex *,int n,int m);
  c_complex c_det(c_complex *,int);
  bool c_egv(c_complex * x,int n); // eigenvectors
  bool c_eig(c_complex * x,c_complex * d,int n); // x eigenvect, d reduced mat
  bool c_proot(c_complex * x,int n); // poly root
  bool c_pcoeff(c_complex * x,int n); // root->coeffs
  bool c_fft(c_complex * x,int n,bool inverse); // FFT
  void c_sprint_double(char * s,double d);
  void c_turtle_forward(double d);
  void c_turtle_left(double d);
  void c_turtle_up(int i);
  void c_turtle_goto(double x,double y);
  void c_turtle_cap(double x);
  int c_turtle_crayon(int i);
  void c_turtle_rond(int x,int y,int z);
  void c_turtle_disque(int x,int y,int z,int centered);
  void c_turtle_fill(int i);
  void c_turtle_fillcolor(double r,double g,double b,int entier);
  void c_turtle_getposition(double * x,double * y);
  void c_turtle_clear(int clrpos);
  void c_turtle_show(int visible);
  int c_turtle_getcap();
  void c_turtle_towards(double x,double y);
  int c_turtle_getcolor();
  void c_turtle_color(int);
  void c_turtle_fillcolor1(int c);
  void py_ck_ctrl_c();

}
#endif // MODULES_H
