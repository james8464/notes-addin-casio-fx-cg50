/* -*- mode:C++ ; compile-command: "g++-3.4 -I.. -I../include -g -c threaded.cc -DHAVE_CONFIG_H -DIN_GIAC  -D_I386_" -*- */
#include "giacPCH.h"
/*  Copyright (C) 2000,2007 B. Parisse, Institut Fourier, 38402 St Martin d'Heres
 *
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
using namespace std;
#include "threaded.h"
#include "sym2poly.h"
#include "gausspol.h"
#include "usual.h"
#include "monomial.h"
#include "modpoly.h"
#include "giacintl.h"
#include "input_parser.h"
#ifdef FXCG
extern "C" {
#include <system.h>
}
#endif

#ifndef NO_NAMESPACE_GIAC
namespace giac {
#endif // ndef NO_NAMESPACE_GIAC

  bool gcd(const vector< T_unsigned<gen,hashgcd_U> > & p_orig,const vector< T_unsigned<gen,hashgcd_U> > & q_orig,vector< T_unsigned<gen,hashgcd_U> > & d, vector< T_unsigned<gen,hashgcd_U> > & pcofactor, vector< T_unsigned<gen,hashgcd_U> > & qcofactor,const std::vector<hashgcd_U> & vars, bool compute_cofactors,int nthreads){
    return false;
  }

  // v <- v*k % m
  void mulmod(vector<int> & v,int k,int m){
    if (k==1)
      return;
    vector<int>::iterator it=v.begin(),itend=v.end();
    for (;it!=itend;++it){
      type_operator_times_reduce(*it,k,*it,m);
      // *it = ((*it)*k)%m;
    }
  }

  bool gcd_ext(const vector< T_unsigned<gen,hashgcd_U> > & p_orig,const vector< T_unsigned<gen,hashgcd_U> > & q_orig,vector< T_unsigned<gen,hashgcd_U> > & d, vector< T_unsigned<gen,hashgcd_U> > & pcofactor, vector< T_unsigned<gen,hashgcd_U> > & qcofactor,const std::vector<hashgcd_U> & vars, bool compute_cofactors,int nthreads){
    return false;
  }

  gen _debug_infolevel(const gen & g0,GIAC_CONTEXT){
    if ( g0.type==_STRNG && g0.subtype==-1) return  g0;
    gen g=evalf_double(g0,1,contextptr);
    if (g.type!=_DOUBLE_)
      return debug_infolevel;
    return debug_infolevel=int(g._DOUBLE_val);
  }
  static const char _debug_infolevel_s []="debug_infolevel";
  static define_unary_function_eval (__debug_infolevel,&_debug_infolevel,_debug_infolevel_s);
  define_unary_function_ptr5( at_debug_infolevel ,alias_at_debug_infolevel,&__debug_infolevel,0,T_DIGITS);



  void wait_1ms(int ms){
    OS_InnerWait_ms(ms);
  }
  

  bool is_zero(const vector<int> & v){
    vector<int>::const_iterator it=v.begin(),itend=v.end();
    for (;it!=itend;++it){
      if (*it)
	return false;
    }
    return true;
  }

  inline int make_unit(int ){
    return 1;
  }
  
  inline vector<int> make_unit(const vector<int>){
    vector<int> v(1,1);
    return v;
  }


  bool mod_gcd(const vector< T_unsigned<vector<int>,hashgcd_U> > & p_orig,const vector< T_unsigned<vector<int>,hashgcd_U> > & q_orig,const vector<int> * pminptr,int modulo,const std::vector<hashgcd_U> & vars,
	      vector< T_unsigned<vector<int>,hashgcd_U> > & d,
	      vector< T_unsigned<vector<int>,hashgcd_U> > & pcof,vector< T_unsigned<vector<int>,hashgcd_U> > & qcof,bool compute_cof,
	      int nthreads){
    return false;
  }

  bool mod_gcd(const vector< T_unsigned<int,hashgcd_U> > & p_orig,const vector< T_unsigned<int,hashgcd_U> > & q_orig,const vector<int> * pminptr,int modulo,const std::vector<hashgcd_U> & vars, vector< T_unsigned<int,hashgcd_U> > & d, vector< T_unsigned<int,hashgcd_U> > & pcofactor, vector< T_unsigned<int,hashgcd_U> > & qcofactor,bool compute_cofactor,int nthreads){
    return false;
  }

  bool mod_gcd(const std::vector< T_unsigned<int,hashgcd_U> > & p_orig,const std::vector< T_unsigned<int,hashgcd_U> > & q_orig,int modulo,std::vector< T_unsigned<int,hashgcd_U> > & d, std::vector< T_unsigned<int,hashgcd_U> > & pcofactor, std::vector< T_unsigned<int,hashgcd_U> > & qcofactor,const std::vector<hashgcd_U> & vars, bool compute_pcofactor,bool compute_qcofactor,int nthreads){
    return false;
  }

  bool mod_gcd(const std::vector< T_unsigned<int,hashgcd_U> > & p_orig,const std::vector< T_unsigned<int,hashgcd_U> > & q_orig,int modulo,std::vector< T_unsigned<int,hashgcd_U> > & d, std::vector< T_unsigned<int,hashgcd_U> > & pcofactor, std::vector< T_unsigned<int,hashgcd_U> > & qcofactor,const std::vector<hashgcd_U> & vars, bool compute_cofactors,int nthreads){
    return false;
  }

  // Beware: overflow may occur if INT128 not defined
  // if min(degree)*modulo^2>=2^63
  void mulsmall(const vector<int>::const_iterator & ita0,const vector<int>::const_iterator & ita_end,const vector<int>::const_iterator & itb0,const vector<int>::const_iterator & itb_end,int modulo,vector<int> & new_coord){  
    new_coord.clear();
    if (ita0==ita_end || itb0==itb_end)
      return;
    new_coord.reserve((ita_end-ita0)+(itb_end-itb0)-1);
    vector<int>::const_iterator ita_begin=ita0,ita=ita0,itb=itb0;
#ifdef INT128
    if ( giacmin(itb_end-itb0,ita_end-ita0)*double(modulo)*modulo >= (1ULL<<63) ){
      for ( ; ita!=ita_end; ++ita ){
	vector<int>::const_iterator ita_cur=ita,itb_cur=itb;
	int128_t res=0;
	for (;itb_cur!=itb_end;--ita_cur,++itb_cur) {
	  res += longlong(*ita_cur) * *itb_cur ;
	  if (ita_cur==ita_begin)
	    break;
	}
	int R=res%modulo;
	R += (unsigned(R)>>31)*modulo; // make positive
	R -= (unsigned((modulo>>1)-R)>>31)*modulo;
	new_coord.push_back(R);// smod(R,modulo);
      }
      --ita;
      ++itb;
      for ( ; itb!=itb_end;++itb){
	int128_t res=0;
	vector<int>::const_iterator ita_cur=ita,itb_cur=itb;
	for (;;) {
	  res += longlong(*ita_cur) * *itb_cur ;
	  ++itb_cur;
	  if (ita_cur==ita_begin || itb_cur==itb_end)
	    break;
	  --ita_cur;
	}
	int R=res%modulo;
	R += (unsigned(R)>>31)*modulo; // make positive
	R -= (unsigned((modulo>>1)-R)>>31)*modulo;
	new_coord.push_back(R);// smod(R,modulo);
      }
      return;
    }
#endif
    for ( ; ita!=ita_end; ++ita ){
      vector<int>::const_iterator ita_cur=ita,itb_cur=itb;
      longlong res=0;
      for (;itb_cur!=itb_end;--ita_cur,++itb_cur) {
	res += longlong(*ita_cur) * *itb_cur ;
	if (ita_cur==ita_begin)
	  break;
      }
      new_coord.push_back(smod(res,modulo));
    }
    --ita;
    ++itb;
    for ( ; itb!=itb_end;++itb){
      longlong res=0;
      vector<int>::const_iterator ita_cur=ita,itb_cur=itb;
      for (;;) {
	res += longlong(*ita_cur) * *itb_cur ;
	++itb_cur;
	if (ita_cur==ita_begin || itb_cur==itb_end)
	  break;
	--ita_cur;
      }
      new_coord.push_back(smod(res,modulo));
    }
  }

  
#ifndef NO_NAMESPACE_GIAC
} // namespace giac
#endif // ndef NO_NAMESPACE_GIAC

