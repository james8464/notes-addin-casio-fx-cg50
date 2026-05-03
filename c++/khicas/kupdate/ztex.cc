// -*- mode:C++ ; compile-command: "g++-3.4 -I.. -g -c tex.cc" -*-
#include "giacPCH.h"

/*
 *  Copyright (C) 2002,2014 B. Parisse, Institut Fourier, 38402 St Martin d'Heres
 *  Figure printing adapted from eukleides (c) 2002, Christian Obrecht
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
//#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __ANDROID__
using std::vector;
#endif

using namespace std;
#include "gen.h"
#include "gausspol.h"
#include "identificateur.h"
#include "symbolic.h"
#include "poly.h"
#include "usual.h"
#include "tex.h"
#include "prog.h"
#include "rpn.h"
#include "plot.h"
#include "giacintl.h"
#if defined GIAC_HAS_STO_38 || defined NSPIRE || defined NSPIRE_NEWLIB || defined FXCG || defined GIAC_GGB || defined USE_GMP_REPLACEMENTS || defined KHICAS
inline bool is_graphe(const giac::gen &g,std::string &disp_out,const giac::context *){ return false; }
#else
#include "graphtheory.h"
#endif

#if !defined NSPIRE && !defined FXCG && !defined GIAC_HAS_STO_38 && !defined KHICAS && !defined NSPIRE_NEWLIB
#include <fstream>
#endif

#ifndef NO_NAMESPACE_GIAC
namespace giac {
#endif // ndef NO_NAMESPACE_GIAC

  // find if a*x+b*y=c intersects the rectangle xmin..xmax,ymin..ymax
  // if true set x0,y0 and x1,y1 to the intersections
  bool is_clipped(double a,double xmin,double xmax,double b,double ymin,double ymax,double c,double & x0,double &y0,double & x1,double &y1){
    int found=0;
    // find the intersects with x=xmin/x=xmax
    double x,y;
    // test whether a or b is too small
    double theta=std::atan2(b,a);
    if (absdouble(M_PI/2-absdouble(theta))<1e-3){
      // line y=cst
      x0=xmin;
      x1=xmax;
      y0=y1=c/b;
      return y0>=ymin && y0<=ymax;
    }
    if (absdouble(theta)<1e-3 || (M_PI-absdouble(theta))<1e-3){
      // line x=cst
      y0=ymin;
      y1=ymax;
      x0=x1=c/a;
      return x0>=xmin && x0<=xmax;
    }
    y=(c-a*xmin)/b;
    if (y>=ymin && y<=ymax){
      ++found;
      x0=xmin;
      y0=y;
    }
    y=(c-a*xmax)/b;
    if (y>=ymin && y<=ymax){
      if (found){
	x1=xmax;
	y1=y;
	return true;
      }
      ++found;
      x0=xmax;
      y0=y;
    }
    x=(c-b*ymin)/a;
    if (x>=xmin && x<=xmax){
      if (found){
	x1=x;
	y1=ymin;
	return true;
      }
      ++found;
      x0=x;
      y0=ymin;
    }
    x=(c-b*ymax)/a;
    if (x>=xmin && x<=xmax){
      if (found){
	x1=x;
	y1=ymax;
	return true;
      }
      ++found;
      x0=x;
      y0=ymax;
    }
    return false;
  }

  bool in_rectangle(double ax,double ay,double xmin,double ymin,double xmax,double ymax){
    return (ax>=xmin && ax<=xmax && ay>=ymin && ay<=ymax);
  }

  // clip line or segment or halfline, result in xa,ya xb,yb
  // mode can be _LINE__VECT (line), _HALFLINE__VECT (halfline)
  // anything else is considered to be a segment
  // return true if there is something to draw
  bool clip_line(double x1,double y1,double x2,double y2,double xmin,double ymin,double xmax,double ymax,int mode,double & xa,double & ya,double & xb,double & yb){
    bool in1=in_rectangle(x1,y1,xmin,ymin,xmax,ymax);
    bool in2=in_rectangle(x2,y2,xmin,ymin,xmax,ymax);
    if (mode!=_LINE__VECT && mode !=_HALFLINE__VECT && in1 && in2){
      xa=x1; ya=y1; xb=x2; yb=y2;
      return true;
    }
    // Line equation is (y2-y1)*x-(x2-x1)*y=(y2*x1-x2*y1)
    double dy=y2-y1;
    double dx=x2-x1;
    double c=(y2*x1-x2*y1);
    bool a=false,b=false;
    // Find 2 intersections y of x=xmin and x=xmax with line
    // Check if they are inside clip region >=ymin and <=ymax
    if (dx){
      double y=(dy*xmin-c)/dx;
      if (y>=ymin && y<=ymax){
	xa=xmin;
	ya=y;
	a=true;
      }
      y=(dy*xmax-c)/dx;
      if (y>=ymin && y<=ymax){
	if (a){ xb=xmax; yb=y; b=true; } 
	else { xa=xmax; ya=y; a=true; }
      }
    }
    // Find 2 intersections of y=ymin and y=ymax with line
    if (!b && dy){
      double x=(dx*ymin+c)/dy;
      if (x>=xmin && x<=xmax){
	if (a){ xb=x; yb=ymin; b=true; }
	else { xa=x; ya=ymin; a=true;}
      }
      x=(dx*ymax+c)/dy;
      if (x>=xmin && x<=xmax){
	if (a){ xb=x; yb=ymax; b=true; }
	else { xa=x; ya=ymax; a=true;}
      }
    }
    if (a && b){ // if not true there is nothing to draw
      // First case is line, we are done
      if (mode==_LINE__VECT)
	return true;
      if (mode!=_HALFLINE__VECT){ // segment
	// we know that both points are not inside
	if (!in1 && !in2) // both are outside, it's like a line
	  return (xa-xmin)*(xb-xmin)<0;
	if (in1){ // 1 is in, 2 not, find if a is between 1 and 2
	  if ((xa-x1)*(x2-x1)>0 || (ya-y1)*(y2-y1)>0){
	    xb=x1; yb=y1;
	  }
	  else { 
	    xa=x1; ya=y1; 
	  }
	  return true;
	}
	if (in2){ // 2 is in, 1 not, find if a is between 2 and 1
	  if ((xa-x2)*(x1-x2)>0 || (ya-y2)*(y1-y2)>0){
	    xb=x2; yb=y2;
	  }
	  else { 
	    xa=x2; ya=y2; 
	  }
	  return true;
	}
      }
    }
    return false;
  }

  static void zoom(double &m,double & M,double d){
    double x_center=(M+m)/2;
    double dx=(M-m);
    double s=absdouble(m)+absdouble(M);
    if (dx<=1e-5*s){
      dx=s;
      if (dx<=1e-5)
	dx=1;
    }
    dx *= d/2;
    m = x_center - dx;
    M = x_center + dx;
  }

  void autoscaleminmax(vector<double> & v,double & m,double & M,bool fullview){
    int s=int(v.size());
    for (int i=0;i<s;++i){
      if (my_isinf(v[i])){
	v.erase(v.begin()+i);
	--s; --i;
      }
    }
    if (s==0){
      v.push_back(0);
      ++s;
    }
    if (s==1){
      v.push_back(v.front());
      ++s;
    }
    if (s>1){    
      sort(v.begin(),v.end());
      m=v[s/10];
      M=v[9*s/10];
      bool b=(M-m)<0.01*(v[s-1]-v[0]);
      if (fullview || 1.75*(M-m)>(v[s-1]-v[0]) || b){
	if (b)
	  zoom(m,M,3);
	else {
	  M=v[s-1];
	  m=v[0];
	  zoom(m,M,1.1);
	}
      }
      else
	zoom(m,M,1/0.8);
      // include origin if not too far
      if ( (m>-M/10 && m<M/2) || (m<-M/10 && m>M/2) )
	m=-M/10;
    }
  }

  // evalf_double of a and splt re/im
  void evalfdouble2reim(const gen & a,gen & e,gen & f0,gen & f1,GIAC_CONTEXT){
    if (a.type==_CPLX){
      f0=a._CPLXptr->evalf2double(1,contextptr);
      f1=(a._CPLXptr+1)->evalf2double(1,contextptr);
      if (a._CPLXptr->type==_DOUBLE_ && (a._CPLXptr+1)->type==_DOUBLE_)
	e=a;
      else
	e=gen(f0._DOUBLE_val,f1._DOUBLE_val);
      return ;
    }
#ifndef NO_STDEXCEPT
    try {
#endif
      e=a.evalf_double(1,contextptr); // FIXME? level 1 does not work for non 0 context
#ifndef NO_STDEXCEPT
    } catch (std::runtime_error & error ){
      last_evaled_argptr(contextptr)=NULL;
      CERR << error.what() << '\n';
    }
#endif
    if (e.type==_CPLX){
      f0=*e._CPLXptr;
      f1=*(e._CPLXptr+1);
    }
    else {
      f0=e;
      f1=0.0;
    }
  }

  static bool in_autoscale(const gen & g,vector<double> & vx,vector<double> & vy,vector<double> & vz,GIAC_CONTEXT,bool curve=false){
    if (g.type==_VECT && g.subtype==_POINT__VECT && g._VECTptr->size()==3){
      vecteur v=*evalf_double(g,1,contextptr)._VECTptr;
      if (v[2].type==_CPLX)
	v[2]=abs(v[2],contextptr);
      if (v[0].type==_DOUBLE_ && v[1].type==_DOUBLE_ && v[2].type==_DOUBLE_ ){
	vx.push_back(v[0]._DOUBLE_val);
	vy.push_back(v[1]._DOUBLE_val);
	vz.push_back(v[2]._DOUBLE_val);
	return false;
      }
    }
    if (g.type==_VECT ){
      if (!curve && g.subtype==_GROUP__VECT && !g._VECTptr->empty() && g._VECTptr->front().type!=_VECT){ // "compressed" hypersurface?
	vecteur v=*evalf_double(g,1,contextptr)._VECTptr;
	int s=v.size();
	if (s%3==0){
	  int i;
	  for (i=0;i<s;i+=3){
	    if (v[i].type!=_DOUBLE_ || v[i+1].type!=_DOUBLE_)
	      break;
	  }
	  if (i==s){
	    for (int i=0;i<s;i+=3){
	      vx.push_back(v[i]._DOUBLE_val);
	      vy.push_back(v[i+1]._DOUBLE_val);
	      if (v[i+2].type==_CPLX)
		vz.push_back(abs(v[i+2],contextptr)._DOUBLE_val);
	      else 
		vz.push_back(v[i+2]._DOUBLE_val);
	    }
	    return false;
	  }
	}
      }
      bool ortho=false;
      if (g.subtype==_POLYEDRE__VECT)
	ortho=true;
      const_iterateur it=g._VECTptr->begin(),itend=g._VECTptr->end();
      for (;it!=itend;++it){
	ortho = ortho | in_autoscale(*it,vx,vy,vz,contextptr);
      }
      return ortho;
    }
    if (g.is_symb_of_sommet(at_curve)){
      gen & gf=g._SYMBptr->feuille;
      if (gf.type==_VECT && gf._VECTptr->size()>1){
	gen tmp=(*gf._VECTptr)[1];
	if (is_undef(tmp)){
	  tmp=(*gf._VECTptr)[0];
	  if (tmp.type==_VECT && tmp._VECTptr->size()>4)
	    tmp=(*tmp._VECTptr)[4];
	}
	in_autoscale(tmp,vx,vy,vz,contextptr,true);
      }
      return false;
    }
    if (g.is_symb_of_sommet(at_hypersurface)){
      gen & g0=g._SYMBptr->feuille;
      if (g0.type==_VECT && g0._VECTptr->size()>2){
	gen & gf=g0._VECTptr->front();
	if (gf.type==_VECT && gf._VECTptr->size()>4){
	  in_autoscale((*gf._VECTptr)[4],vx,vy,vz,contextptr);
	}
      }	
      return false;
    }
    if (g.is_symb_of_sommet(at_hyperplan)){
      gen & g0=g._SYMBptr->feuille;
      if (g0.type==_VECT && g0._VECTptr->size()>=2){
	gen n=evalf_double(g0._VECTptr->front(),1,contextptr);
	gen P=evalf_double(g0._VECTptr->back(),1,contextptr);
	if (n.type==_VECT && n._VECTptr->size()==3 && P.type==_VECT && P._VECTptr->size()==3){
	  vecteur & N=*n._VECTptr;
	  vecteur & M=*P._VECTptr;
	  if (M[0].type==_DOUBLE_ && M[1].type==_DOUBLE_ && M[2].type==_DOUBLE_ && N[0].type==_DOUBLE_ && N[1].type==_DOUBLE_ && N[2].type==_DOUBLE_){
	    double d=M[0]._DOUBLE_val,r=(fabs(N[0]._DOUBLE_val)+fabs(N[1]._DOUBLE_val)+fabs(N[2]._DOUBLE_val))/3;
	    vx.push_back(d-r); vx.push_back(d+r);
	    d=M[1]._DOUBLE_val;
	    vy.push_back(d-r); vy.push_back(d+r);
	    d=M[2]._DOUBLE_val;
	    vz.push_back(d-r); vz.push_back(d+r);
	    return false;
	  }
	}
      }	
      return false;
    }
    if (g.is_symb_of_sommet(at_cercle)){
      gen c,r;
      centre_rayon(g,c,r,false,contextptr);
      if (is_zero(r)) r=1;
      vecteur v;
      if (g._SYMBptr->feuille.type==_VECT && g._SYMBptr->feuille._VECTptr->size()>=3){
	v=*g._SYMBptr->feuille._VECTptr;
	gen v2=evalf_double(v[2],1,contextptr);
	gen v1=evalf_double(v[1],1,contextptr);
	gen delta=v2-v1;
	v=makevecteur(c,c+r*exp(cst_i*(v1-0.1*delta),contextptr),
		      c+r*exp(cst_i*v1,contextptr),
		      c+r*exp(cst_i*(3*v1+v2)/gen(4),contextptr),
		      c+r*exp(cst_i*(v1+v2)/gen(2),contextptr),
		      c+r*exp(cst_i*(v1+3*v2)/gen(4),contextptr),
		      c+r*exp(cst_i*v2,contextptr),
		      c+r*exp(cst_i*(v2+0.1*delta),contextptr)); 
      }
      else
	v=makevecteur(c-r,c+r,c-cst_i*r,c+cst_i*r);
      in_autoscale(v,vx,vy,vz,contextptr);
      return true;
    }
    if (g.is_symb_of_sommet(at_hypersphere)){
      vecteur & v=*g._SYMBptr->feuille._VECTptr;
      gen c=evalf_double(v[0],1,contextptr),R=evalf_double(v[1],1,contextptr);
      if (c.type==_VECT && c._VECTptr->size()==3){
	vecteur & M=*c._VECTptr;
	if (M[0].type==_DOUBLE_ && M[1].type==_DOUBLE_ && M[2].type==_DOUBLE_ && R.type==_DOUBLE_){
	  double r=R._DOUBLE_val;
	  double d=M[0]._DOUBLE_val;
	  vx.push_back(d-r); vx.push_back(d+r);
	  d=M[1]._DOUBLE_val;
	  vy.push_back(d-r); vy.push_back(d+r);
	  d=M[2]._DOUBLE_val;
	  vz.push_back(d-r); vz.push_back(d+r);
	  return true;
	}
      }
    }
    // FIXME sphere etc.
    if (g.type!=_VECT){
      gen e,f0,f1;
      evalfdouble2reim(g,e,f0,f1,contextptr);
      if (f0.type==_DOUBLE_ && f1.type==_DOUBLE_){
	vx.push_back(f0._DOUBLE_val);
	vy.push_back(f1._DOUBLE_val);
      }
    }
    return false;
  }

  bool autoscaleg(const gen & g,vector<double> & vx,vector<double> & vy,vector<double> & vz,GIAC_CONTEXT){
    if (g.type==_VECT){
      bool ortho=false;
      const_iterateur it=g._VECTptr->begin(),itend=g._VECTptr->end();
      for (;it!=itend;++it)
	ortho = ortho | autoscaleg(*it,vx,vy,vz,contextptr);
      return ortho;
    }
    if (g.is_symb_of_sommet(at_pnt)){
      gen & gf=g._SYMBptr->feuille;
      if (gf.type==_VECT && !gf._VECTptr->empty()){
	gen & f=gf._VECTptr->front();
	return in_autoscale(f,vx,vy,vz,contextptr);
      }
    }
    if (g.is_symb_of_sommet(at_equal) && g._SYMBptr->feuille.type==_VECT && g._SYMBptr->feuille._VECTptr->size()==2 && g._SYMBptr->feuille._VECTptr->front()==_GL_ORTHO && !is_zero(g._SYMBptr->feuille._VECTptr->back()))
      return true;
    return false;
  }

#ifndef NO_NAMESPACE_GIAC
} // namespace giac
#endif // ndef NO_NAMESPACE_GIAC
