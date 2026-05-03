// -*- mode:C++ ; compile-command: "g++ -I.. -g -c Equation.cc" -*-
#ifndef _EQUATION_H
#define _EQUATION_H
#include "config.h"
#include "giacPCH.h"
#include "misc.h"
#include "textGUI.hpp"

#define STATUS_AREA_PX 24
#ifdef MPM
#define color_gris 42260 // rgb(160,160,160)
#else
#define color_gris 57083
#endif
extern "C" {
  int os_get_pixel(int x,int y);
  int os_draw_string(int X,int Y,int color,int bg,const char * buf,bool fake=false);
#define os_draw_string_medium os_draw_string
  int os_draw_string_small(int X,int Y,int color,int bg,const char * buf,bool fake=false);
  void statuslinemsg(const char * msg);
  void os_fill_rect(int x,int y,int w,int h,int c);
  void os_sync_screen();
  void os_set_pixel(int x0, int y0,unsigned short color) ;
  int file_exists(const char * filename);
  const char * read_file(const char * filename);
}

bool is_python_keyword(const char * s);
bool is_python_builtin(const char * s);

#ifndef NO_NAMESPACE_XCAS
namespace xcas {
#endif // ndef NO_NAMESPACE_XCAS
  void switch_to_micropy(bool ck,GIAC_CONTEXT);

  int displaygraph(const giac::gen & ge,const giac::gen & gs,GIAC_CONTEXT);
  int displaygraph(const giac::gen & ge,const giac::gen & gs);

  // maximum "size" of symbolics displayed in an Equation (pretty print)
  extern unsigned max_prettyprint_equation;
  // matrix select
  bool eqw_select(const giac::gen & eq,int l,int c,bool select,giac::gen & value);
  void Equation_select(giac::gen & eql,bool select);
  giac::gen Equation_copy(const giac::gen & g);
  int eqw_select_down(giac::gen & g);
  int eqw_select_up(giac::gen & g);

  giac::gen Equation_compute_size(const giac::gen & g,const giac::attributs & a,int windowhsize,const giac::context * contextptr);
  giac::eqwdata Equation_total_size(const giac::gen & g);  
  // Equation_translate(giac::gen & g,int deltax,int deltay);
  void Equation_vertical_adjust(int hp,int yp,int & h,int & y);
  bool Equation_find_vector_pos(giac::const_iterateur it,giac::const_iterateur itend,int & i,int &nrows);
  bool Equation_adjust_xy(giac::gen & g,int & xleft,int & ytop,int & xright,int & ybottom,giac::gen * & gsel,giac::gen * & gselparent,int &gselpos,std::vector<int> * gotosel=0);
  // select and set value from eqwdata in eql
  bool do_select(giac::gen & eql,bool select,giac::gen & value);

  class Equation {
    int _x,_y;
  public:
    giac::gen data,undodata; // of type eqwdata or undef if empty
    giac::attributs attr;
    int x() const { return _x;}
    int y() const { return _y;}
    Equation(int x_, int y_, const giac::gen & g);
  };

  void display(Equation &eq ,int x,int y);
  // replace selection in eq by tmp
  void replace_selection(Equation & eq,const giac::gen & tmp,giac::gen * gsel=0,const std::vector<int> * gotoptr=0);
  int eqw_select_leftright(xcas::Equation & g,bool left,int exchange=0);

  // typedef double float3d;
  typedef float float3d;
  
  struct double3 {
    double x,y,z;
    double3(double x_,double y_,double z_):x(x_),y(y_),z(z_){};
    double3():x(0),y(0),z(0){};
  };

  struct float3d3 {
    float3d x,y,z;
    float3d3(double x_,double y_,double z_):x(x_),y(y_),z(z_){};
    float3d3():x(0),y(0),z(0){};
  };

  struct int4 {
    int u,d,du,dd;
    int4(int u_,int d_,int du_,int dd_):u(u_),d(d_),du(du_),dd(dd_) {}
  };
  
  // quaternion struct for more intuitive rotations
  struct quaternion_double {
    double w,x,y,z;
    quaternion_double():w(1),x(0),y(0),z(0) {};
    quaternion_double(double theta_x,double theta_y,double theta_z);
    quaternion_double(double _w,double _x,double _y,double _z):w(_w),x(_x),y(_y),z(_z) {};
    double norm2() const { return w*w+x*x+y*y+z*z;}
  };

  quaternion_double operator * (const quaternion_double & q,const quaternion_double & q2);

  void get_axis_angle_deg(const quaternion_double & q,double &x,double &y,double & z, double &theta); // q must be a quaternion of unit norm, theta is in deg

  // Euler angle are given in degrees
  quaternion_double euler_deg_to_quaternion_double(double a,double b,double c);
  void quaternion_double_to_euler_deg(const quaternion_double & q,double & phi,double & theta, double & psi);

  struct int2 {
    int i,j;
    int2(int i_,int j_):i(i_),j(j_) {}
    int2(): i(0),j(0) {}
  };
  inline bool operator < (const int2 & a,const int2 & b){ if (a.i!=b.i) return a.i<b.i; return a.j<b.j;}
  inline bool operator == (const int2 & a,const int2 & b){ return a.i==b.i && a.j==b.j;}

  struct int2_double2 {
    int i,j;
    double arg,norm;
  };
  inline bool operator < (const int2_double2 & a,const int2_double2 & b){ if (a.arg!=b.arg) return a.arg<b.arg; return a.norm<b.norm;}

#define giac3d_default_upcolor 65535
#define giac3d_default_downcolor 12345
#define giac3d_default_downupcolor 18432 // 12297
#define giac3d_default_downdowncolor 22539
  
  enum {
	FL_PUSH=0,
	FL_MOVE=1,
	FL_DRAG=2,
	FL_RELEASE=3,
	FL_KEYBOARD=4,
  };
  
  giac::gen add_attributs(const giac::gen & g,int couleur_,const giac::context *) ;

  class Graph2d{
  public:
    double window_xmin,window_xmax,window_ymin,window_ymax,window_zmin,window_zmax,
      x_scale,y_scale,z_scale,x_tick,y_tick,z_tick;
    //double theta_x,theta_y,theta_z;
    quaternion_double q;
    float3d transform[16],invtransform[16];
    // only 12 used, last line [0,0,0,1], usual matrices, not transposed
    int display_mode,show_axes,show_edges,show_names,labelsize,lcdz,default_upcolor,default_downcolor,default_downupcolor,default_downdowncolor;
    short int precision,diffusionz,diffusionz_limit;
    bool is3d,doprecise,hide2nd,interval,solid3d,must_redraw;
    float3d Ai,Aj,Bi,Bj,Ci,Cj,Di,Dj,Ei,Ej,Fi,Fj,Gi,Gj,Hi,Hj; // visualization cube coordinates
    std::vector< std::vector< std::vector<float3d> > > surfacev;
    std::vector<float3d3> plan_pointv; // point in plan 
    std::vector<float3d3> plan_abcv; // plan equation z=a*x+b*y+c
    std::vector<bool> plan_filled;
    std::vector<float3d3> sphere_centerv;
    std::vector<float3d> sphere_radiusv;
    giac::vecteur sphere_quadraticv; // matrix of the transformed quad. form
    std::vector<bool> sphere_isclipped;
    std::vector< std::vector<float3d3> > polyedrev;
    std::vector<float3d3> polyedre_abcv;
    std::vector<float3d> polyedre_xyminmax;
    std::vector<bool> polyedre_faceisclipped,polyedre_filled;
    std::vector<float3d3> linev; // 2 float3d3 per object
    std::vector<short> linetypev;
    std::vector<const char *> lines; // legende
    std::vector< std::vector<float3d3> > curvev;
    std::vector<float3d3> pointv; 
    std::vector<const char *> points; // legende
    std::vector<int4> hyp_color,plan_color,sphere_color,polyedre_color,line_color,curve_color,point_color;
    giac::gen g,trace_x0,trace_x1,trace_x2,trace_y0,trace_y1,trace_y2,trace_parameq,trace_z1,trace_z2,trace_t; //  normal graph instructions, trace_* are storing cached derivatives for curve study
    const giac::context * contextptr;
    /* geometry data */
    double current_i,current_j;
    int mode=0; // 0 pointer, 1 1-arg, 2 2-args, etc.
    // plot_tmp=function_tmp(args_tmp) or function_final(args_tmp)
    // depends whether args.tmp.size()==mode
    giac::gen function_tmp,function_final,args_push; 
    giac::vecteur args_tmp; // WARNING should only contain numeric value
    unsigned args_tmp_push_size;
    std::vector<std::string> args_help;
    std::string title,x_axis_name,x_axis_unit,y_axis_name,y_axis_unit,z_axis_name,z_axis_unit,fcnfield,fcnvars;
    int npixels; // max # of pixels distance for click
    giac::vecteur plot_instructions,symbolic_instructions,animation_instructions,trace_instructions;
    double animation_dt; // rate for animated plot
    bool paused;
    double animation_last; // clock value at last display
    int animation_instructions_pos;
    int rotanim_type,rotanim_danim,rotanim_nstep;
    double rotanim_rx,rotanim_ry,rotanim_rz,rotanim_tstep;
    int couleur; // used for new point creation in geometry
    bool approx; // exact or approx click mouse?
    std::vector<int> selected; // all items selected in plot_instructions
    giac::gen drag_original_value,drag_name;
    int hp_pos; // Position in hp for modification
    textArea * hp; // null pointer if normal graph (not geometry)
    giac::gen title_tmp,plot_tmp;
    std::string modestr;
    double push_depth,current_depth;
    int push_i,push_j,cursor_point_type; // position of mouse push/drag
    bool pushed=false,moving=false,moving_frame=false,in_area=true;
    bool moving_param; double param_orig,param_value,param_min,param_max,param_step;
    int nparams;  
    int tracemode;
    int tracemode_n; double tracemode_i; string tracemode_add; giac::vecteur tracemode_disp; double tracemode_mark;
    /* end geometry data */
    giac::vecteur param(double d) const;
    void adjust_cursor_point_type();
    void autoname_plus_plus();
    void do_handle(const giac::gen & g);
    void redraw() { must_redraw=true; }
    void geometry_round(double x,double y,double z,double eps,giac::gen & tmp,const giac::context *) ;
    giac::gen geometry_round(double x,double y,double z,double eps,giac::gen & original,int & pos,bool selectfirstlevel=false,bool setscroller=false);
    giac::vecteur selection2vecteur(const std::vector<int> & v);
    void set_mode(const giac::gen & f_tmp,const giac::gen & f_final,int m,const std::string & help);
    void invert_tracemode();
    void tracemode_set(int operation=0); // operation==1 if user is setting the value of t on a parametric curve, operation==2 for root, operation==3 for extremum, operation==4 mark current position, operation=5 for area
    void init_tracemode();
    void curve_infos();
    void add_entry(int pos);
    double find_eps() const;
    void find_xyz(double i,double j,double k,double & x,double & y,double & z) const;
    void set_gen_value(int n,const giac::gen & g,bool exec=true); // set n-th entry value
    int geo_handle(int event,int key);
    int ui();
    giac::vecteur selected_names(bool allobjects,bool withdef) const;
    void find_title_plot(giac::gen & title_tmp,giac::gen & plot_tmp);
    void draw_decorations(const giac::gen & title_tmp);
    bool find_dxdy(double & dx, double & dy) const;
    void find_xy(double i,double j,double & x,double & y) const ;    
    void round_xy(double & x, double & y) const;
    void eval(int start=0); // eval symbolic_instructions to plot_instructions
    void update_g(); // geometry: plot_instructions, trace/animation -> g
    giac::vecteur get_current_animation() const;
    bool findij(const giac::gen & e0,double x_scale,double y_scale,double & i0,double & j0,const giac::context * ) const;
    void xyz2ij(const float3d3 & d,int &i,int &j) const; // d not transformed
    void xyz2ij(const float3d3 & d,float3d &i,float3d &j) const; // d not transformed
    void xyz2ij(const float3d3 & d,float3d &i,float3d &j,float3d3 & d3) const; // d not transformed, d3 is
    void XYZ2ij(const float3d3 & d,int &i,int &j) const; // d is transformed
    void addpolyg(vector<int2> & polyg,float3d x,float3d y,float3d z,int2 & IJmin) const ;
    void adddepth(vector<int2> & polyg,const float3d3 &A,const float3d3 &B,int2 & IJmin) const;
    void update_scales();
    void update();
    void update_rotation(); // update grot
    void zoomx(double d,bool round=false,bool doupdate=true);
    void zoomy(double d,bool round=false,bool doupdate=true);
    void zoomz(double d,bool round=false,bool doupdate=true);
    void zoom(double d,bool doupdate=true);
    void left(double d);
    void right(double d);
    void up(double d);
    void down(double d);
    void z_up(double d);
    void z_down(double d);
    void autoscale(bool fullview=false,bool doupdate=true);
    void orthonormalize(bool doupdate=true);
    void draw();
    bool glsurface(int w,int h,int lcdz,const giac::context*,int upcolor,int downcolor,int downupcolor,int downdowncolor) ;
    Graph2d(const giac::gen & g_,const giac::context * );
  };

  extern Graph2d * geoptr;
  
  struct Turtle {
    void draw();
#ifdef TURTLETAB
    giac::logo_turtle * turtleptr;
#else
    std::vector<giac::logo_turtle> * turtleptr;
#endif
    int turtlex,turtley; // Turtle translate
    double turtlezoom; // Zoom factor for turtle screen
    int maillage; // 0 (none), 1 (square), 2 (triangle), bit3 used for printing
  };
  
  struct tableur {
    giac::matrice m,clip,undo;
    giac::gen var;
    int nrows,ncols;
    int cur_row,cur_col,disp_row_begin,disp_col_begin;
    int sel_row_begin,sel_col_begin;
    std::string cmdline,filename;
    int cmd_pos,cmd_row,cmd_col,disp_start; // row/col of current cmdline, -1 if not active
    bool changed,recompute,matrix_fill_cells,movedown,keytooltip;
  } ;
  ustl::string print_tableur(const tableur & t,GIAC_CONTEXT);

  void geosave(textArea * text,const giac::context *,bool writeflash=false);
  int newgeo(const giac::context *);
  void cleargeo();
  int geoloop(Graph2d * geoptr);
  bool geoparse(textArea *text,const giac::context *);

#ifndef NO_NAMESPACE_XCAS
} // namespace xcas
#endif // ndef NO_NAMESPACE_XCAS
extern xcas::tableur * xcas_sheetptr;

int periodic_table(const char * & name,const char * & symbol,char * protons,char * nucleons,char * mass,char * electroneg);

giac::gen sheet(const giac::context *); // in kadd.cc
#endif // _EQUATION_H
