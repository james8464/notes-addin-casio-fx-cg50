// -*- mode:C++ ; compile-command: "g++-3.4 -I.. -g -c ifactor.cc -DHAVE_CONFIG_H -DIN_GIAC" -*-
#include "giacPCH.h"
#if !defined __MINGW_H && !defined FXCG
#define GIAC_MPQS // define if you want to use giac for sieving 
#endif



#include "path.h"
/*
 *  Copyright (C) 2003,14 R. De Graeve & B. Parisse, 
 *  Institut Fourier, 38402 St Martin d'Heres
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
#ifdef GIAC_HAS_STO_38
//#undef clock
//#undef clock_t
#else
#include <fstream>
//#include <unistd.h> // For reading arguments from file
#include "ifactor.h"
#include "pari.h"
#include "usual.h"
#include "sym2poly.h"
#include "rpn.h"
#include "prog.h"
#include "misc.h"
#include "giacintl.h"
#endif

#ifdef GIAC_HAS_STO_38
#define BESTA_OS
#endif
// Trying to make ifactor(2^128+1) work on ARM
#if defined(RTOS_THREADX) || defined(BESTA_OS) || defined NSPIRE
//#define OLD_AFACT
#define GIAC_ADDITIONAL_PRIMES 16// if defined, additional primes are used in sieve
#else
#define GIAC_ADDITIONAL_PRIMES 32// if defined, additional primes are used in sieve
#endif


#ifndef NO_NAMESPACE_GIAC
namespace giac {
#endif // ndef NO_NAMESPACE_GIAC

  const short int giac_primes[]={2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,617,619,631,641,643,647,653,659,661,673,677,683,691,701,709,719,727,733,739,743,751,757,761,769,773,787,797,809,811,821,823,827,829,839,853,857,859,863,877,881,883,887,907,911,919,929,937,941,947,953,967,971,977,983,991,997};
  static const int giac_last_prime=giac_primes[sizeof(giac_primes)/sizeof(short)-1];
#ifdef HAVE_LIBTOMMATH
  int modulo(const mpz_t & a,unsigned b){
    if (mpz_cmp_ui(a,0)<0){
      mpz_neg(*(mpz_t *)&a,a);
      int res=modulo(a,b);
      mpz_neg(*(mpz_t *)&a,a);
      return b-res;
    }
    mp_digit C; 
    mp_mod_d((mp_int *)&a,b,&C);
    return C;
  }
#else
  int modulo(const mpz_t & a,unsigned b){
    return mpz_fdiv_ui(a,b);
  }
#endif

  int modulo(const gen & a,unsigned b){
    if (a.type==_INT_)
      return a.val % b;
    return modulo(*a._ZINTptr,b);
  }

  // Pollard-rho algorithm
  const int POLLARD_GCD=64;
#ifdef GIAC_MPQS 
#if defined(RTOS_THREADX) // !defined(BESTA_OS)
  const int POLLARD_MAXITER=3000;
#else
  const int POLLARD_MAXITER=15000;
#endif
#else
  const int POLLARD_MAXITER=15000;
#endif  

  static gen pollard(gen n, gen k,GIAC_CONTEXT){
    k.uncoerce();
    n.uncoerce();
    int maxiter=POLLARD_MAXITER;
    double nd=evalf_double(n,1,contextptr)._DOUBLE_val;
#if defined(RTOS_THREADX) || defined(BESTA_OS) || defined NSPIRE || defined FXCG
    int nd1=int(2000*(std::log10(nd)-34));
#else
    int nd1=int(1500*std::pow(16.,(std::log10(nd)-40)/10));
#endif
    if (nd1>maxiter)
      maxiter=nd1;
    int m,m1,a,a1,j;
    m1=m=2;
    a1=a=1;
    int c=0;
    mpz_t g,x,x1,x2,x2k,y,y1,p,q,tmpq,alloc1,alloc2,alloc3,alloc4,alloc5;
    mpz_init_set_si(g,1); // ? mp_init_size to specify size
    mpz_init_set_si(x,2);
    mpz_init_set_si(x1,2);
    mpz_init_set_si(y,2);
    mpz_init(y1);
    mpz_init(x2);
    mpz_init(x2k);
    mpz_init_set_si(p,1);
    mpz_init(q);
    mpz_init(tmpq);
    mpz_init(alloc1);
    mpz_init(alloc2);
    mpz_init(alloc3);
    mpz_init(alloc4);
    mpz_init(alloc5);
    while (!ctrl_c && !interrupted && mpz_cmp_si(g,1)==0) {
#ifdef TIMEOUT
      control_c();
#endif
      a=2*a+1;//a=2^(e+1)-1=2*l(m)-1 
      while (!ctrl_c && !interrupted && mpz_cmp_si(g,1)==0 && a>m) { // ok
#ifdef TIMEOUT
	control_c();
#endif
	// x=f(x,k,n,q);
#ifdef HAVE_LIBTOMMATH
	mp_sqr(&x,&x2);
	mpz_add(x2k,x2,*k._ZINTptr);
	if (mpz_cmp(x2k,*n._ZINTptr)>0){
	  mp_grow(&alloc1,x2k.used+2);
	  mpz_set_ui(alloc1,0);
	  alloc1.used = x2k.used +2 ;
	  mpz_set(alloc2,x2k);
	  mpz_set(alloc3,*n._ZINTptr);
	  // mpz_set_si(alloc4,0);
	  // mpz_set_si(alloc5,0);
	  alloc_mp_div(&x2k,n._ZINTptr,&tmpq,&x,&alloc1,&alloc2,&alloc3,&alloc4,&alloc5);
	}
	else
	  mpz_set(x,x2k);
#else 
	mpz_mul(x2,x,x);
	mpz_add(x2k,x2,*k._ZINTptr);
	mpz_tdiv_r(x,x2k,*n._ZINTptr);
#endif
	m += 1;
	if (debug_infolevel && ((m % 
#if defined(RTOS_THREADX) || defined(BESTA_OS) || defined NSPIRE || defined FXCG
				 (1<<10)
#else
				 (1<<18)
#endif
				 )==0))
	  *logptr(contextptr) << CLOCK() << gettext(" Pollard-rho try ") << m << endl;
	if (m > maxiter ){
	  if (0 && debug_infolevel)	  
	    *logptr(contextptr) << CLOCK() << gettext(" Pollard-rho failure, ntries ") << m << endl;
	  mpz_clear(alloc5);
	  mpz_clear(alloc4);
	  mpz_clear(alloc3);
	  mpz_clear(alloc2);
	  mpz_clear(alloc1);
	  mpz_clear(tmpq);
	  mpz_clear(x);
	  mpz_clear(x1);
	  mpz_clear(x2);
	  mpz_clear(x2k);
	  mpz_clear(y);
	  mpz_clear(y1);
	  mpz_clear(p);
	  mpz_clear(q);
	  return -1;
	}
	// p=irem(p*(x1-x),n,q);
	mpz_sub(q,x1,x);
	mpz_mul(x2,p,q);
#if 0 // def USE_GMP_REPLACEMENTS
	if (mpz_cmp(x2,*n._ZINTptr)>0){
	  mp_grow(&alloc1,x2.used+2);
	  mpz_set_ui(alloc1,0);
	  alloc1.used = x2.used +2 ;
	  mpz_set(alloc2,x2);
	  mpz_set(alloc3,*n._ZINTptr);
	  // mpz_set_si(alloc4,0);
	  // mpz_set_si(alloc5,0);
	  alloc_mp_div(&x2,n._ZINTptr,&tmpq,&p,&alloc1,&alloc2,&alloc3,&alloc4,&alloc5);
	}
	else 
	  mpz_set(p,x2);
#else
	mpz_tdiv_r(p,x2,*n._ZINTptr);
#endif
	c += 1;
	if (c==POLLARD_GCD) {
	  // g=gcd(abs(p,context0),n); 
	  mpz_abs(q,p);
	  my_mpz_gcd(g,q,*n._ZINTptr);
	  if (mpz_cmp_si(g,1)==0) {
	    mpz_set(y,x); // y=x;
	    mpz_set(y1,x1); // y1=x1;
	    mpz_set_si(p,1); // p=1;
	    a1=a;
	    m1=m;
	    c=0;
	  }
	}
      }//m=a=2^e-1=l(m)
      if (mpz_cmp_si(g,1)==0) {
	mpz_set(x1,x); // x1=x;//x1=x_m=x_l(m)-1
	j=3*(a+1)/2; // j=3*iquo(a+1,2);
	for (long i=m+1;i<=j;i++){
	  // x=f(x,k,n,q);
	  mpz_mul(x2,x,x);
	  mpz_add(x2k,x2,*k._ZINTptr);
#if 0 // def USE_GMP_REPLACEMENTS
	  if (mpz_cmp(x2k,*n._ZINTptr)>0){
	    mp_grow(&alloc1,x2k.used+2);
	    mpz_set_ui(alloc1,0);
	    alloc1.used = x2k.used +2 ;
	    mpz_set(alloc2,x2k);
	    mpz_set(alloc3,*n._ZINTptr);
	    // mpz_set_si(alloc4,0);
	    // mpz_set_si(alloc5,0);
	    alloc_mp_div(&x2k,n._ZINTptr,&tmpq,&x,&alloc1,&alloc2,&alloc3,&alloc4,&alloc5);
	  }
	  else 
	    mpz_set(x,x2);
#else
	  mpz_tdiv_r(x,x2k,*n._ZINTptr);
#endif
	}
	m=j;
      }
    }
    //g<>1 ds le paquet de POLLARD_GCD
    if (0 && debug_infolevel>5)
      CERR << CLOCK() << " Pollard-rho nloops " << m << endl;
    mpz_set(x,y); // x=y;
    mpz_set(x1,y1); // x1=y1;
    mpz_set_si(g,1); // g=1;
    a=(a1-1)/2; // a=iquo(a1-1,2);
    m=m1;
    while (!ctrl_c && !interrupted && mpz_cmp_si(g,1)==0) {
#ifdef TIMEOUT
      control_c();
#endif
      a=2*a+1;
      while (!ctrl_c && !interrupted && mpz_cmp_si(g,1)==0 && a>m) { // ok
#ifdef TIMEOUT
	control_c();
#endif
	// x=f(x,k,n,q);
	mpz_mul(x2,x,x);
	mpz_add(x2k,x2,*k._ZINTptr);
#if 0 // def USE_GMP_REPLACEMENTS
	if (mpz_cmp(x2k,*n._ZINTptr)>0){
	  mp_grow(&alloc1,x2k.used+2);
	  mpz_set_ui(alloc1,0);
	  alloc1.used = x2k.used +2 ;
	  mpz_set(alloc2,x2k);
	  mpz_set(alloc3,*n._ZINTptr);
	  alloc_mp_div(&x2k,n._ZINTptr,&tmpq,&x,&alloc1,&alloc2,&alloc3,&alloc4,&alloc5);
	}
	else
	  mpz_set(x,x2k);	  
#else
	mpz_tdiv_r(x,x2k,*n._ZINTptr);
#endif
	m += 1;
	if (m > maxiter ){
	  mpz_clear(alloc5);
	  mpz_clear(alloc4);
	  mpz_clear(alloc3);
	  mpz_clear(alloc2);
	  mpz_clear(alloc1);
	  mpz_clear(tmpq);
	  mpz_clear(x);
	  mpz_clear(x1);
	  mpz_clear(x2);
	  mpz_clear(x2k);
	  mpz_clear(y);
	  mpz_clear(y1);
	  mpz_clear(p);
	  mpz_clear(q);
	  return -1;
	}
	// p=irem(x1-x,n,q);
	mpz_sub(q,x1,x);
#if 0 // def USE_GMP_REPLACEMENTS
	if (mpz_cmp(q,*n._ZINTptr)>0){
	  mp_grow(&alloc1,q.used+2);
	  mpz_set_ui(alloc1,0);
	  alloc1.used = q.used +2 ;
	  mpz_set(alloc2,q);
	  mpz_set(alloc3,*n._ZINTptr);
	  // mpz_set_si(alloc4,0);
	  // mpz_set_si(alloc5,0);
	  alloc_mp_div(&q,n._ZINTptr,&tmpq,&p,&alloc1,&alloc2,&alloc3,&alloc4,&alloc5);
	}
	else 
	  mpz_set(p,q);
#else
	mpz_tdiv_r(p,q,*n._ZINTptr);
#endif
	// g=gcd(abs(p,context0),n);  // ok
	mpz_abs(q,p);
	my_mpz_gcd(g,q,*n._ZINTptr);
      }
      if (mpz_cmp_si(g,1)==0) {
	mpz_set(x1,x); // x1=x;
	j=3*(a+1)/2; // j=3*iquo(a+1,2);
	for (long i=m+1;j>=i;i++){
	  // x=f(x,k,n,q);
	  mpz_mul(x2,x,x);
	  mpz_add(x2k,x2,*k._ZINTptr);
	  mpz_tdiv_qr(tmpq,x,x2k,*n._ZINTptr);
	}
	m=j;
      }
    }
    mpz_clear(alloc5);
    mpz_clear(alloc4);
    mpz_clear(alloc3);
    mpz_clear(alloc2);
    mpz_clear(alloc1);
    mpz_clear(tmpq);
    mpz_clear(x);
    mpz_clear(x1);
    mpz_clear(x2);
    mpz_clear(x2k);
    mpz_clear(y);
    mpz_clear(y1);
    mpz_clear(p);
    mpz_clear(q);
#ifdef TIMEOUT
    control_c();
#endif
    if (ctrl_c || interrupted){
      mpz_clear(g);
      return 0;
    }
    if (mpz_cmp(g,*n._ZINTptr)==0) {
      if (k==1) {
	mpz_clear(g);
	return(pollard(n,-1,contextptr)); 
      }
      else {
	if (k*k==1){
	  mpz_clear(g);
	  return(pollard(n,3,contextptr));
	}
	else {
	  if (is_greater(k,50,contextptr)){
#if 1
	    return -1;
#else
	    ref_mpz_t * ptr=new ref_mpz_t;
	    mpz_init_set(ptr->z,g);
	    mpz_clear(g);
	    return ptr;
#endif
	  }
	  else {
	    mpz_clear(g);
	    return(pollard(n,k+2,contextptr));
	  }
	} 
      }
    } 
    ref_mpz_t * ptr=new ref_mpz_t;
    mpz_init_set(ptr->z,g);
    mpz_clear(g);
    return ptr;
  }

  // const short int giac_primes[]={2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,617,619,631,641,643,647,653,659,661,673,677,683,691,701,709,719,727,733,739,743,751,757,761,769,773,787,797,809,811,821,823,827,829,839,853,857,859,863,877,881,883,887,907,911,919,929,937,941,947,953,967,971,977,983,991,997};

  bool eratosthene(double n,vector<bool> * & v){
    static vector<bool> *ptr=0;
    if (!ptr)
      ptr=new vector<bool>;
    vector<bool> &erato=*ptr;
    v=ptr;
    if (n+1>erato.size()){
      unsigned N=int(n);
      ++N;
#if defined BESTA_OS 
      if (N>2e6)
	return false;
#else
      if (N>2e9)
	return false;
#endif
      N = (N*11)/10;
      erato=vector<bool>(N+1,true); 
      // insure that we won't recompute all again from start for ithprime(i+1)
      for (unsigned p=2;;++p){
	while (!erato[p]) // find next prime
	  ++p;
	if (p*p>N) // finished
	  return true;
	for (unsigned i=2*p;i<=N;i+=p) 
	  erato[i]=false; // remove p multiples
      }
    }
    return true;
  }

  bool eratosthene2(double n,vector<bool> * & v){
    static vector<bool> *ptr=0;
    if (!ptr)
      ptr=new vector<bool>;
    vector<bool> &erato=*ptr;
    v=ptr;
    if (n/2>=erato.size()){
      unsigned N=int(n);
      ++N;
#if defined BESTA_OS 
      if (N>4e6)
	return false;
#else
      if (N>2e9)
	return false;
#endif
      // 11/20 insures that we won't recompute all again from start for ithprime(i+1)
      N = (N*11)/20; // keep only odd numbers in sieve
      erato=vector<bool>(N+1,true); //erato[i] stands for 2*i+1 <-> n corresponds to erato[n/2]
      for (unsigned p=3;;p+=2){
	while (!erato[p/2]) // find next prime (first one is p==3)
	  p+=2;
	if (p*p>2*N+1) // finished
	  return true;
	// p is prime, set p*p, (p+2)*p, etc. to be non prime
	for (unsigned i=(p*p)/2;i<=N;i+=p) 
	  erato[i]=false; // remove p multiples
      }
    }
    return true;
  }

  // ithprime(n) is approx invli(n)+invli(sqrt(n))/4 where invli is reciproc.
  // of Li(x)=Ei(ln(x))
  // For fast code, cf. https://github.com/kimwalisch/primecount
  static const char _ithprime_s []="ithprime";
  static symbolic symb_ithprime(const gen & args){
    return symbolic(at_ithprime,args);
  }
  static gen ithprime(const gen & g_,GIAC_CONTEXT){
    gen g(g_);
    if (!is_integral(g))
      return gentypeerr(contextptr);
    if (g.type!=_INT_)
      return gensizeerr(contextptr); // symb_ithprime(g);
    int i=g.val;
    if (i<0)
      return gensizeerr(contextptr);
    if (i==0)
      return 1;
    if (i<=int(sizeof(giac_primes)/sizeof(short int)))
      return giac_primes[i-1];
    vector<bool> * vptr=0;
#if 1
    if (!eratosthene2(i*std::log(double(i))*1.1,vptr))
      return gensizeerr(contextptr);
    unsigned count=2;
    unsigned s=unsigned(vptr->size());
    for (unsigned k=2;k<s;++k){
      if ((*vptr)[k]){
	++count;
	if (i==count)
	  return int(2*k+1);
      }
    }
    return undef;
#else
    if (!eratosthene(i*std::log(double(i))*1.1,vptr))
      return gensizeerr(contextptr);
    unsigned count=2;
    unsigned s=vptr->size();
    for (unsigned k=4;k<s;++k){
      if ((*vptr)[k]){
	++count;
	if (i==count)
	  return int(k);
      }
    }
    return undef;
#endif
  }
  gen _ithprime(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT)
      return apply(args,_ithprime,contextptr);
    return ithprime(args,contextptr);
  }
  static define_unary_function_eval (__ithprime,&_ithprime,_ithprime_s);
  define_unary_function_ptr5( at_ithprime ,alias_at_ithprime,&__ithprime,0,true);

  static const char _nprimes_s []="nprimes";
  static gen nprimes(const gen & g_,GIAC_CONTEXT){
    gen g(g_);
    if (!is_integral(g))
      return gentypeerr(contextptr);
    if (g.type!=_INT_)
      return gensizeerr(contextptr); // symb_ithprime(g);
    int i=g.val;
    if (i<0)
      return gensizeerr(contextptr);
    if (i<2)
      return 0;
    vector<bool> * vptr=0;
    if (!eratosthene2(i+2,vptr))
      return gensizeerr(contextptr);
    unsigned count=1; // 2 is prime, then count odd primes
    i=(i-1)/2;
    for (int k=1;k<=i;++k){
      if ((*vptr)[k])
	++count;
    }
    return int(count);
  }
  gen _nprimes(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT)
      return apply(args,_nprimes,contextptr);
    return nprimes(args,contextptr);
  }
  static define_unary_function_eval (__nprimes,&_nprimes,_nprimes_s);
  define_unary_function_ptr5( at_nprimes ,alias_at_nprimes,&__nprimes,0,true);

  bool is_divisible_by(const gen & n,unsigned long a){
    if (n.type==_ZINT){
#ifdef HAVE_LIBTOMMATH
      mp_digit c;
      mp_mod_d(n._ZINTptr, a, &c);
      return c==0;
#else
      return mpz_divisible_ui_p(*n._ZINTptr,a);
#endif
    }
    return n.val%a==0;
  }

  // find trivial factors of n, 
  // if add_last is true the remainder is put in the vecteur,
  // otherwise n contains the remainder
  vecteur pfacprem(gen & n,bool add_last,GIAC_CONTEXT){
    gen a;
    gen q;
    int p,i,prime;
    vecteur v(2);
    vecteur u;
    if (is_zero(n))
      return u;
    if (n.type==_ZINT){
      ref_mpz_t * cur = new ref_mpz_t;
      mpz_t div,q,r,alloc1,alloc2,alloc3,alloc4,alloc5;
      mpz_set(cur->z,*n._ZINTptr);
      mpz_init_set(q,*n._ZINTptr);
      mpz_init(r);
      mpz_init(div);
      mpz_init(alloc1);
      mpz_init(alloc2);
      mpz_init(alloc3);
      mpz_init(alloc4);
      mpz_init(alloc5);
      for (i=0;i<int(sizeof(giac_primes)/sizeof(short int));++i){
	if (mpz_cmp_si(cur->z,1)==0) 
	  break;
	prime=giac_primes[i];
	mpz_set_ui(div,prime);
#ifdef HAVE_LIBTOMMATH
	for (p=0;;p++){
	  mp_grow(&alloc1,cur->z.used+2);
	  mpz_set_ui(alloc1,0);
	  alloc1.used = cur->z.used +2 ;
	  mpz_set(alloc2,cur->z);
	  mpz_set(alloc3,div);
	  alloc_mp_div(&cur->z,&div,&q,&r,&alloc1,&alloc2,&alloc3,&alloc4,&alloc5);
	  // mpz_tdiv_qr(q,r,cur->z,div);
	  if (mpz_cmp_si(r,0))
	    break;
	  mp_exch(&cur->z,&q);
	}
	// *logptr(contextptr) << "Factor " << prime << " " << p << endl;
	if (p){
	  u.push_back(prime);
	  u.push_back(p);
	}
#else
	if (mpz_divisible_ui_p(cur->z,prime)){
	  mpz_set_ui(div,prime);
	  for (p=0;;p++){
	    mpz_tdiv_qr(q,r,cur->z,div);
	    if (mpz_cmp_si(r,0))
	      break;
	    mpz_swap(cur->z,q);
	  }
	  // *logptr(contextptr) << "Factor " << prime << " " << p << endl;
	  u.push_back(prime);
	  u.push_back(p);
	}
#endif
      } // end for on smal primes
      mpz_clear(alloc5);
      mpz_clear(alloc4);
      mpz_clear(alloc3);
      mpz_clear(alloc2);
      mpz_clear(alloc1);
      mpz_clear(div); mpz_clear(r); mpz_clear(q);
      n=cur;
    }
    else {
      for (i=0;i<int(sizeof(giac_primes)/sizeof(short int));++i){
	if (n==1) 
	  break;
	a.val=giac_primes[i];
	p=0;
	while (is_divisible_by(n,a.val)){ // while (irem(n,a,q)==0){
	  n=iquo(n,a); 
	  p=p+1;
	}
	if (p!=0){
	  // *logptr(contextptr) << "Factor " << a << " " << p << endl;
	  u.push_back(a);
	  u.push_back(p);
	}
      }
    }
    if (add_last && i==int(sizeof(giac_primes)/sizeof(short int)) && !is_one(n)){
      // hack: check if n is a perfect square
      double nf=evalf_double(n,1,contextptr)._DOUBLE_val;
      nf=std::sqrt(nf);
      gen n2=_round(nf,contextptr);
      if (n2*n2==n){
	u.push_back(n2);
	u.push_back(2);	
      }
      else {
	u.push_back(n);
	u.push_back(1);
      }
      n=1;
    }
    //v[0]=n;
    //v[1]=u;
    
    return(u);
  }

#ifdef USE_GMP_REPLACEMENTS
  static gen inpollardsieve(const gen &a,gen k,bool & do_pollard,GIAC_CONTEXT){
    gen b=do_pollard?pollard(a,k,contextptr):-1;
#ifdef TIMEOUT
    control_c();
#endif
#ifdef GIAC_MPQS
    if (b==-1 && !ctrl_c && !interrupted){ 
      do_pollard=false;
      if (msieve(a,b,contextptr)) return b; else return -1; }
#endif
    return b;
  }
  static gen pollardsieve(const gen &a,gen k,bool & do_pollard,GIAC_CONTEXT){
#if defined( GIAC_HAS_STO_38) || defined(EMCC) || defined NSPIRE
    int debug_infolevel_=debug_infolevel;
#if defined RTOS_THREADX || defined NSPIRE || defined FXCG
    debug_infolevel=2;
    if (do_pollard)
      *logptr(contextptr) << gettext("Pollard-rho on ") << a << endl; 
#else
    debug_infolevel=0;
#endif
#endif
    gen res=inpollardsieve(a,k,do_pollard,contextptr);
#if defined( GIAC_HAS_STO_38) || defined(EMCC) || defined NSPIRE
    debug_infolevel=debug_infolevel_;
#ifdef GIAC_HAS_STO_38
    Calc->Terminal.MakeUnvisible();
#endif
#endif
    return res;
  }
#else // USE_GMP_REPLACEMENTS
  static gen pollardsieve(const gen &a,gen k,bool & do_pollard,GIAC_CONTEXT){
    gen b=do_pollard?pollard(a,k,contextptr):-1;
#ifdef TIMEOUT
    control_c();
#endif
#ifdef GIAC_MPQS
    if (b==-1 && !ctrl_c && !interrupted){ 
      do_pollard=false;
      if (msieve(a,b,contextptr)) return b; else return -1; }
#endif
    if (b==-1)
      b=a;
    return b;
  }
#endif // USE_GMP_REPLACEMENTS

  static gen ifactor2(const gen & n,vecteur & v,bool & do_pollard,GIAC_CONTEXT){
    if (is_greater(giac_last_prime*giac_last_prime,n,contextptr) || is_probab_prime_p(n) ){
      v.push_back(n);
      return 1;
    }
    // Check for power of integer: arg must be > 1e4, n*ln(arg)=d => n<d/ln(1e4)
    double d=evalf_double(n,1,contextptr)._DOUBLE_val;
    int maxpow=int(std::ceil(std::log(d)/std::log(1e4)));
    for (int i=2;i<=maxpow;++i){
      if ( (i>2 && i%2==0) ||
	   (i>3 && i%3==0) ||
	   (i>5 && i%5==0) ||
	   (i>7 && i%7==0) )
	continue;
      gen u;
      if (i==2)
	u=isqrt(n);
      else {
	double x=std::pow(d,1./i);
	u=gen(longlong(x));
      }
      if (pow(u,i,contextptr)==n){
	vecteur w;
	do_pollard=true;
	ifactor2(u,w,do_pollard,contextptr);
	for (int j=0;j<i;j++)
	  v=mergevecteur(v,w);
	return v;
      }
    }
    gen a=pollardsieve(n,1,do_pollard,contextptr);
    if (a==-1)
      return a;
#ifdef TIMEOUT
    control_c();
#endif
    if (ctrl_c || interrupted)
      return gensizeerr("Interrupted");
    gen ba=n/a;
    if (a!=n)
      a=ifactor2(a,v,do_pollard,contextptr);
    else {
      a=1;
      v.push_back(n);
    }
    if (is_strictly_greater(ba,1,contextptr))
      a=ifactor2(ba,v,do_pollard,contextptr);
    return a;
  }

  static vecteur facprem(gen & n,GIAC_CONTEXT){
    vecteur v;    
    if (n==1) { return v; }
    if ( (n.type==_INT_ && n.val<giac_last_prime*giac_last_prime) || is_probab_prime_p(n)) {
      v.push_back(n);
      n=1;
      return v;
    }
    if (0 && debug_infolevel>5)
      CERR << "Pollard begin " << CLOCK() << endl;
    bool do_pollard=true;
    gen a=ifactor2(n,v,do_pollard,contextptr);
    if (a==-1)
      return makevecteur(gensizeerr(gettext("Quadratic sieve failure, perhaps number too large")));
    if (is_zero(a))
      return makevecteur(gensizeerr(gettext("Stopped by user interruption")));
    n=1;
    return v;
  }

  void mergeifactors(const vecteur & f,const vecteur &g,vecteur & h){
    h=f;
    for (unsigned i=0;i<g.size();i+=2){
      unsigned j=0;
      for (;j<f.size();j+=2){
	if (f[j]==g[i])
	  break;
      }
      if (j<f.size())
	h[j+1] += g[i+1];
      else {
	h.push_back(g[i]);
	h.push_back(g[i+1]);
      }
    }
  }

  static vecteur giac_ifactors(const gen & n0,GIAC_CONTEXT){
    if (!is_integer(n0) || is_zero(n0))
      return vecteur(1,gensizeerr(gettext("ifactors")));
    if (is_one(n0))
      return vecteur(0);
    gen n(n0);
    vecteur f;
    vecteur g;
    vecteur u;
    // First find if |n-k^d|<=1 for d = 2, 3, 5 or 7
    double nd=evalf_double(n,1,contextptr)._DOUBLE_val;
    double nd2=std::floor(std::sqrt(nd)+.5);
    if (absdouble(1-nd2*nd2/nd)<1e-10){
      gen n2=isqrt(n+1);
      if (n==n2*n2){
	f=ifactors(n2,contextptr);
	iterateur it=f.begin(),itend=f.end();
	for (;it!=itend;++it){
	  ++it;
	  *it = 2 * *it;
	}
	return f;
      }
      if (n==n2*n2-1){
	f=ifactors(n2-1,contextptr);
	g=ifactors(n2+1,contextptr);
	mergeifactors(f,g,u);
	return u;
      }
    }
    for (int k=3;;){
      nd2=std::floor(std::pow(nd,1./k)+.5);
      if (absdouble(1-std::pow(nd2,k)/nd)<1e-10){
	gen n2=_floor(nd2,contextptr),nf=n2*n2;
	for (int j=2;j<k;j++)
	  nf=nf*n2;
	if (n==nf){
	  f=ifactors(n2,contextptr);
	  iterateur it=f.begin(),itend=f.end();
	  for (;it!=itend;++it){
	    ++it;
	    *it = k * *it;
	  }
	  return f;
	}
	if (n==nf-1){ // n2^k-1
	  f=ifactors(n2-1,contextptr);
	  g=ifactors(n/(n2-1),contextptr);
	  mergeifactors(f,g,u);
	  return u;
	}
	if (n==nf+1){ // n2^k+1
	  f=ifactors(n2+1,contextptr);
	  g=ifactors(n/(n2+1),contextptr);
	  mergeifactors(f,g,u);
	  return u;
	}
      }
      if (k==11) break;
      if (k==7) break; // k=11;
      if (k==5) k=7;
      if (k==3) k=5;
    }
    //f=pfacprem(n,false,contextptr);
    //cout<<n<<" "<<f<<endl;
    while (n!=1) {
      g=facprem(n,contextptr);
      if (is_undef(g))
	return g;
      islesscomplexthanf_sort(g.begin(),g.end());
      gen last=0; int p=0;
      for (unsigned i=0;i<g.size();++i){
	if (g[i]==last)
	  ++p;
	else {
	  if (last!=0){
	    u.push_back(last);
	    u.push_back(p);
	  }
	  last=g[i];
	  p=1;
	}
      }
      u.push_back(last);
      u.push_back(p);
    }   
    g=mergevecteur(f,u);
    return g;
  }

  static vecteur ifactors1(const gen & n0,GIAC_CONTEXT){
    if (is_greater(1e71,n0,contextptr))
      return giac_ifactors(n0,contextptr);
    if (n0.type==_VECT && !n0._VECTptr->empty())
      return giac_ifactors(n0._VECTptr->front(),contextptr);
#ifdef HAVE_LIBPARI
#ifdef __APPLE__
    return vecteur(1,gensizeerr(gettext("(Mac OS) Large number, you can try pari(); pari_factor(")+n0.print(contextptr)+")"));
#endif
    if (!is_integer(n0) || is_zero(n0))
      return vecteur(1,gensizeerr(gettext("ifactors")));
    if (is_one(n0))
      return vecteur(0);
    gen g(pari_ifactor(n0),contextptr); 
    if (g.type==_VECT){
      matrice m(mtran(*g._VECTptr));
      vecteur res;
      const_iterateur it=m.begin(),itend=m.end();
      for (;it!=itend;++it){
	if (it->type!=_VECT) return vecteur(1,gensizeerr(gettext("ifactor.cc/ifactors")));
	res.push_back(it->_VECTptr->front());
	res.push_back(it->_VECTptr->back());
      }
      return res;
    }
#endif // LIBPARI
    return giac_ifactors(n0,contextptr);
  }

  vecteur ifactors(const gen & n0,GIAC_CONTEXT){
    gen n(n0);
    vecteur f=pfacprem(n,false,contextptr);
    if (is_undef(f))
      return f;
    vecteur g=ifactors1(n,contextptr);
    if (is_undef(g))
      return g;
    return mergevecteur(f,g);
  }


  gen ifactors(const gen & args,int maplemode,GIAC_CONTEXT){
    if ( (args.type==_INT_) || (args.type==_ZINT)){
      if (is_zero(args)){
	if (maplemode==1)
	  return makevecteur(args,vecteur(0));
	else
	  return makevecteur(args);
      }
      vecteur v(ifactors(abs(args,contextptr),contextptr)); // ok
      if (!v.empty() && is_undef(v.front()))
	return v.front();
      if (maplemode!=1){
	if (is_positive(args,context0))
	  return v;
	return mergevecteur(makevecteur(minus_one,plus_one),v);
      }
      vecteur res;
      const_iterateur it=v.begin(),itend=v.end();
      for (;it!=itend;it+=2){
	res.push_back(makevecteur(*it,*(it+1)));
      }
      if (is_positive(args,context0))
	return makevecteur(plus_one,res);
      else
	return makevecteur(minus_one,res);	
    }
    return gentypeerr(gettext("ifactors"));
  }

  gen _ifactors(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT)
      return apply(args,_ifactors,contextptr);
    gen g(args);
    if (!is_integral(g))
      return gensizeerr(contextptr);
    if (calc_mode(contextptr)==1){ // ggb returns factors repeted instead of multiplicites
      vecteur res;
      gen in=ifactors(g,0,contextptr);
      if (in.type==_VECT){
	for (unsigned i=0;i<in._VECTptr->size();i+=2){
	  gen f=in[i],m=in[i+1];
	  if (m.type==_INT_){
	    for (int j=0;j<m.val;++j)
	      res.push_back(f);
	  }
	}
	return res;
      }
    }
    return ifactors(g,0,contextptr);
  }
  static const char _ifactors_s []="ifactors";
  static define_unary_function_eval (__ifactors,&_ifactors,_ifactors_s);
  define_unary_function_ptr5( at_ifactors ,alias_at_ifactors,&__ifactors,0,true);

  static const char _facteurs_premiers_s []="facteurs_premiers";
  static define_unary_function_eval (__facteurs_premiers,&_ifactors,_facteurs_premiers_s);
  define_unary_function_ptr5( at_facteurs_premiers ,alias_at_facteurs_premiers,&__facteurs_premiers,0,true);

  static const char _maple_ifactors_s []="maple_ifactors";
  gen _maple_ifactors(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT)
      return apply(args,_maple_ifactors,contextptr);
    return ifactors(args,1,contextptr);
  }
  static define_unary_function_eval (__maple_ifactors,&_maple_ifactors,_maple_ifactors_s);
  define_unary_function_ptr5( at_maple_ifactors ,alias_at_maple_ifactors,&__maple_ifactors,0,true);

  static vecteur in_factors(const gen & gf,GIAC_CONTEXT){
    if (gf.type!=_SYMB)
      return makevecteur(gf,plus_one);
    unary_function_ptr & u=gf._SYMBptr->sommet;
    if (u==at_inv){
      vecteur v=in_factors(gf._SYMBptr->feuille,contextptr);
      iterateur it=v.begin(),itend=v.end();
      for (;it!=itend;it+=2)
	*(it+1)=-*(it+1);
      return v;
    }
    if (u==at_neg){
      vecteur v=in_factors(gf._SYMBptr->feuille,contextptr);
      v.push_back(minus_one);
      v.push_back(plus_one);
      return v;
    }
    if ( (u==at_pow) && (gf._SYMBptr->feuille._VECTptr->back().type==_INT_) ){
      vecteur v=in_factors(gf._SYMBptr->feuille._VECTptr->front(),contextptr);
      gen k=gf._SYMBptr->feuille._VECTptr->back();
      iterateur it=v.begin(),itend=v.end();
      for (;it!=itend;it+=2)
	*(it+1)=k* *(it+1);
      return v;
    }
    if (u!=at_prod)
      return makevecteur(gf,plus_one);
    vecteur res;
    const_iterateur it=gf._SYMBptr->feuille._VECTptr->begin(),itend=gf._SYMBptr->feuille._VECTptr->end();
    for (;it!=itend;++it){
      res=mergevecteur(res,in_factors(*it,contextptr));
    }
    return res;
  }
  static vecteur in_factors1(const vecteur & res,GIAC_CONTEXT){
    gen coeff(1);
    vecteur v;
    const_iterateur it=res.begin(),itend=res.end();
    for (;it!=itend;it+=2){
      if (lidnt(*it).empty())
	coeff=coeff*(pow(*it,*(it+1),contextptr));
      else
	v.push_back(makevecteur(*it,*(it+1)));
    }
    return makevecteur(coeff,v);
  }
  vecteur factors(const gen & g,const gen & x,GIAC_CONTEXT){
    gen gf=factor(g,x,false,contextptr);
    vecteur res=in_factors(gf,contextptr);
    if (xcas_mode(contextptr)!=1)
      return res;
    return in_factors1(res,contextptr);
  }
  vecteur sqff_factors(const gen & g,GIAC_CONTEXT){
    gen gf=_sqrfree(g,contextptr);
    return in_factors(gf,contextptr);
  }
  static const char _factors_s []="factors";
  gen _factors(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT && args.subtype==_SEQ__VECT && args._VECTptr->size()==2){
      gen j=args._VECTptr->back();
      gen res=_factors(args._VECTptr->front()*j,contextptr);
      if (res.type==_VECT && xcas_mode(contextptr)!=1)
	res=in_factors1(*res._VECTptr,contextptr);
      if (res.type==_VECT && res._VECTptr->size()==2){
	res._VECTptr->front()=recursive_normal(res._VECTptr->front()/j,contextptr);
	if (xcas_mode(contextptr)!=1){
	  if (is_one(res._VECTptr->front()))
	    res=res._VECTptr->back();
	  else {
	    j=res._VECTptr->front();
	    res=res._VECTptr->back();
	    if (res.type==_VECT)
	      res=mergevecteur(makevecteur(j,1),*res._VECTptr);
	  }
	  vecteur v;
	  aplatir(*res._VECTptr,v,contextptr);
	  res=v;
	}
      }
      return res;
    }
    if (args.type==_VECT)
      return apply(args,_factors,contextptr);
    return factors(args,vx_var,contextptr);
  }
  static define_unary_function_eval (__factors,&_factors,_factors_s);
  define_unary_function_ptr5( at_factors ,alias_at_factors,&__factors,0,true);

  static gen ifactors2ifactor(const vecteur & l,bool quote){
    int s;
    s=int(l.size());
    gen r;
    vecteur v(s/2);
    for (int j=0;j<s;j=j+2){
      if (!is_one(l[j+1]))
	v[j/2]=symb_pow(l[j],l[j+1]);
      else
	v[j/2]=l[j];
    }
    if (v.size()==1){
#if defined(GIAC_HAS_STO_38) && defined(CAS38_DISABLED)
      return symb_quote(v.front());
#else
      if (quote)
	return symb_quote(v.front());
      return v.front();
#endif
    }
    r=symb_prod(v);
#if defined(GIAC_HAS_STO_38) && defined(CAS38_DISABLED)
    r=symb_quote(r);
#endif
    if (quote)
      return symb_quote(r);
    return r;
  }
  gen ifactor(const gen & n,GIAC_CONTEXT){
    vecteur l;
    l=ifactors(n,contextptr);
    if (!l.empty() && is_undef(l.front())) return l.front();
    return ifactors2ifactor(l,calc_mode(contextptr)==1);
  }
  gen _ifactor(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    gen n=args;
    if (n.type==_VECT && n._VECTptr->size()==1 && is_integer(n._VECTptr->front()))
      return ifactor(n,contextptr);
    if (n.type==_VECT)
      return apply(n,_ifactor,contextptr);
    if (!is_integral(n))
      return gensizeerr(contextptr);
    if (is_strictly_positive(-n,0))
      return -_ifactor(-n,contextptr);
    if (n.type==_INT_ && n.val<=3)
      return n;
    return ifactor(n,contextptr);
  }
  static const char _ifactor_s []="ifactor";
  static define_unary_function_eval (__ifactor,&_ifactor,_ifactor_s);
  define_unary_function_ptr5( at_ifactor ,alias_at_ifactor,&__ifactor,0,true);

  static const char _factoriser_entier_s []="factoriser_entier";
  static define_unary_function_eval (__factoriser_entier,&_ifactor,_factoriser_entier_s);
  define_unary_function_ptr5( at_factoriser_entier ,alias_at_factoriser_entier,&__factoriser_entier,0,true);

  static vecteur divis(const vecteur & l3,GIAC_CONTEXT){
    vecteur l1(1);
    gen d,e;
    int s=int(l3.size());
    gen taille=1;
    for (int k=0;k<s;k+=2){
      taille=taille*(l3[k+1]+1);
    }
    if (taille.type!=_INT_ || taille.val>LIST_SIZE_LIMIT)
      return vecteur(1,gendimerr(contextptr));
    l1.reserve(taille.val);
    l1[0]=1;//l3.push_back(..);
    for (int k=0;k<s;k=k+2) {
      vecteur l2;
      l2.reserve(taille.val);
      int s1;
      s1=int(l1.size());
      vecteur l4(s1);
      d=l3[k];
      e=l3[k+1];
      int ei;
      if (e.type==_INT_){
	ei=e.val;
      }
      else
	return vecteur(1,gensizeerr(gettext("Integer too large")));
      for (int j=1;j<=ei;j++){
	gen dj=pow(d,j);
	for (int l=0;l<s1;l++){ 
	  l4[l]=l1[l]*dj;
	}
	// l2=mergevecteur(l2,l4);
	iterateur it=l4.begin(),itend=l4.end();
	for (;it!=itend;++it)
	  l2.push_back(*it);
      }
      // l1=mergevecteur(l1,l2);
      iterateur it=l2.begin(),itend=l2.end();
      for (;it!=itend;++it)
	l1.push_back(*it);
    }
    return(l1); 
  }
  gen idivis(const gen & n,GIAC_CONTEXT){
    vecteur l3(ifactors(n,contextptr));
    if (!l3.empty() && is_undef(l3.front())) return l3.front();
    return divis(l3,contextptr);
  }
  gen _idivis(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT)
      return apply(args,_idivis,contextptr);
    gen n=args;
    if (is_zero(n) || (!is_integral(n) && !is_integer(n)) || n.type==_CPLX) 
      return gentypeerr(contextptr);
    return _sort(idivis(abs(n,contextptr),contextptr),contextptr);
  }
  static const char _idivis_s []="idivis";
  static define_unary_function_eval (__idivis,&_idivis,_idivis_s);
  define_unary_function_ptr5( at_idivis ,alias_at_idivis,&__idivis,0,true);

  gen _divis(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT)
      return apply(args,_divis,contextptr);
    return divis(factors(args,vx_var,contextptr),contextptr);
  }
  static const char _divis_s []="divis";
  static define_unary_function_eval (__divis,&_divis,_divis_s);
  define_unary_function_ptr5( at_divis ,alias_at_divis,&__divis,0,true);

  /*
  gen ichinreme(const vecteur & a,const vecteur & b){
    vecteur r(2);
    gen p=a[1],q=b[1],u,v,d;
    egcd(p,q,u,v,d);
    if (d!=1)  return gensizeerr(contextptr);
    r[0]=(u*p*b[0]+v*q*a[0]%p*q);
    r[1]=p*q;
    return(r);
  }
  gen _ichinreme(const gen & args){
  if ( args){
    if ( (args.type!=_VECT) || (args._VECTptr->size()!=4) )
      return gensizeerr(contextptr);
    vecteur a(2).type==_STRNG && args.subtype==-1{
    if ( (args.type!=_VECT) || (args._VECTptr->size()!=4) )
      return gensizeerr(contextptr);
    vecteur a(2))) return  args){
    if ( (args.type!=_VECT) || (args._VECTptr->size()!=4) )
      return gensizeerr(contextptr);
    vecteur a(2);
    if ( (args.type!=_VECT) || (args._VECTptr->size()!=4) )
      return gensizeerr(contextptr);
    vecteur a(2),b(2);
    a[0]=args[0];
    a[1]=args[1];
    b[0]=args[2];
    b[1]=args[3];
    //gen a=args[0],p=args[1], b=args[2],q=args[3];
    return ichinreme(a,b);
  }
  static const char _ichinreme_s []="ichinreme";
  static define_unary_function_eval (__ichinreme,&_ichinreme,_ichinreme_s);
  define_unary_function_ptr5( at_ichinreme ,alias_at_ichinreme,&__ichinreme,0,true); 
  */

  gen euler(const gen & e,GIAC_CONTEXT){
    if (e==0)
      return e;
    vecteur v(ifactors(e,contextptr));
    if (!v.empty() && is_undef(v.front())) return v.front();
    const_iterateur it=v.begin(),itend=v.end();
    for (gen res(plus_one);;){
      if (it==itend)
	return res;
      gen p=*it;
      ++it;
      int n=it->val;
      res = res * (p-plus_one)*pow(p,n-1);
      ++it;
    }
  }
  static const char _euler_s []="euler";
  gen _euler(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT)
      return apply(args,_euler,contextptr);
    if ( is_integer(args) && is_positive(args,contextptr))
      return euler(args,contextptr);
    return gentypeerr(contextptr);
  }
  static define_unary_function_eval (__euler,&_euler,_euler_s);
  define_unary_function_ptr5( at_euler ,alias_at_euler,&__euler,0,true);

  gen _propfrac(const gen & arg,GIAC_CONTEXT){
    if ( arg.type==_STRNG && arg.subtype==-1) return  arg;
    gen args(arg);
    vecteur v;
    if (arg.type==_VECT && arg._VECTptr->size()==2){
      v=vecteur(1,arg._VECTptr->back());
      args=arg._VECTptr->front();
      lvar(args,v);
    }
    else
      v=lvar(arg);
    gen g=e2r(args,v,contextptr);
    gen a,b;
    fxnd(g,a,b);
    {
      gen d=r2e(b,v,contextptr);
      g=_quorem(makesequence(r2e(a,v,contextptr),d,v.front()),contextptr);
      if (is_undef(g)) return g;
      vecteur &v=*g._VECTptr;
      return v[0]+rdiv(v[1],d,contextptr);
    }
  }
  static const char _propfrac_s []="propfrac";
  static define_unary_function_eval (__propfrac,&_propfrac,_propfrac_s);
  define_unary_function_ptr5( at_propfrac ,alias_at_propfrac,&__propfrac,0,true);
  
  gen iabcuv(const gen & a,const gen & b,const gen & c,GIAC_CONTEXT){
    gen d=gcd(a,b);
    if (c%d!=0)  return gensizeerr(gettext("No solution in ring"));
    gen a1=a/d,b1=b/d,c1=c/d;
    gen u,v,w;
    egcd(a1,b1,u,v,w);
    vecteur r(2);
    r[0]=smod(u*c1,b);
    r[1]=iquo(c-r[0]*a,b);
    return r;
  }
  gen _iabcuv(const gen & args,GIAC_CONTEXT){
#ifdef CASCAS_ALEVEL_ONLY
    return gensizeerr(contextptr);
#endif
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if ( (args.type!=_VECT) || (args._VECTptr->size()!=3) )
      return gensizeerr(contextptr);
    gen a=args[0],b=args[1],c=args[2];
    return iabcuv(a,b,c,contextptr);
  }
  static const char _iabcuv_s []="_rs93";
  static define_unary_function_eval (__iabcuv,&_iabcuv,_iabcuv_s);
  define_unary_function_ptr5( at_iabcuv ,alias_at_iabcuv,&__iabcuv,0,true);

  gen abcuv(const gen & a,const gen & b,const gen & c,const gen & x,GIAC_CONTEXT){
    gen g=_egcd(makesequence(a,b,x),contextptr);
    if (is_undef(g)) return g;
    vecteur & v=*g._VECTptr;
    gen h=_quorem(makesequence(c,v[2],x),contextptr);
    if (is_undef(h)) return h;
    vecteur & w=*h._VECTptr;
    if (!is_zero(w[1]))
      return gensizeerr(gettext("No solution in ring"));
    gen U=v[0]*w[0],V=v[1]*w[0];
    if (_degree(makesequence(c,x),contextptr).val<_degree(makesequence(a,x),contextptr).val+_degree(makesequence(b,x),contextptr).val ){
      U=_rem(makesequence(U,b,x),contextptr);
      V=_rem(makesequence(V,a,x),contextptr);
    }
    return makevecteur(U,V);
  }
  gen _abcuv(const gen & args,GIAC_CONTEXT){
#ifdef CASCAS_ALEVEL_ONLY
    return gensizeerr(contextptr);
#endif
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if ( (args.type!=_VECT) || (args._VECTptr->size()<3) )
      return gensizeerr(contextptr);
    vecteur & v =*args._VECTptr;
    if (v.size()>3)
      return abcuv(v[0],v[1],v[2],v[3],contextptr);
    return abcuv(v[0],v[1],v[2],vx_var,contextptr);
  }
  static const char _abcuv_s []="_rs94";
  static define_unary_function_eval (__abcuv,&_abcuv,_abcuv_s);
  define_unary_function_ptr5( at_abcuv ,alias_at_abcuv,&__abcuv,0,true);

  gen simp2(const gen & a,const gen & b,GIAC_CONTEXT){
    vecteur r(2);
    gen d=gcd(a,b);
    r[0]=normal(a/d,contextptr);
    r[1]=normal(b/d,contextptr);
    return r;
  }
  gen _simp2(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if ( (args.type!=_VECT) || (args._VECTptr->size()!=2) )
      return gensizeerr(contextptr);
    gen a=args[0],b=args[1];
    if ( (a.type==_VECT) || (b.type==_VECT) )
      return gensizeerr(contextptr);
    return simp2(a,b,contextptr);
  }
  static const char _simp2_s []="simp2";
  static define_unary_function_eval (__simp2,&_simp2,_simp2_s);
  define_unary_function_ptr5( at_simp2 ,alias_at_simp2,&__simp2,0,true);
 
  gen fxnd(const gen & a){
    vecteur v(lvar(a));
    gen g=e2r(a,v,context0); // ok
    gen n,d;
    fxnd(g,n,d);
    return makevecteur(r2e(n,v,context0),r2e(d,v,context0)); // ok
  }
  gen _fxnd(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT)
      return apply(args,fxnd);
    return fxnd(args);
  }
  static const char _fxnd_s []="fxnd";
  static define_unary_function_eval (__fxnd,&_fxnd,_fxnd_s);
  define_unary_function_ptr5( at_fxnd ,alias_at_fxnd,&__fxnd,0,true); 

  int generator(int p,const vecteur & v){
    vector<int> w;
    for (int i=0;i<v.size();i+=2){
      if (v[i].type!=_INT_)
	return 0;
      w.push_back((p-1)/v[i].val);
    }
    for (int a=2;a<p;++a){
      int r=0;
      for (int i=0;i<w.size();i++){
	r=powmod(a,w[i],p);
	if (r==1)
	  break;
      }
      if (r!=1)
	return a;
    }
    return 0; // p is not prime!
  }

  // p assumed to be prime, find a generator of (Z/pZ^*,*)
  int generator(int p){
    vecteur v=ifactors(p-1,context0);
    return generator(p,v);
  }

  gen _znprimroot(const gen & p,GIAC_CONTEXT){
    if (p==2) return 1;
    if (p==4) return 3;
    gen o=p-1,q=p; // order
    if (modulo(p,2)==0){
      q=p/2;
      if (modulo(q,2)==0)
	return undef;
    }
    int cyclic=0;
    if (is_probab_prime_p(q))
      cyclic=1;
    else {
      // is q a power of a prime?
      double d=evalf_double(q,1,contextptr)._DOUBLE_val;
      int maxpow=int(std::ceil(std::log(d)/std::log(3)));
      for (int i=2;i<=maxpow;++i){
	if ( (i>2 && i%2==0) ||
	     (i>3 && i%3==0) ||
	     (i>5 && i%5==0) ||
	     (i>7 && i%7==0) )
	  continue;
	gen u;
	if (i==2)
	  u=isqrt(q);
	else if (i==4)
	  u=isqrt(isqrt(q));
	else {
	  double x=std::pow(d,1./i);
	  u=longlong(x);
	}
	if (pow(u,i,contextptr)==q){
	  cyclic=2;
	  o=q*(1-inv(u,contextptr));
	  break;
	}
      }
    }
    if (cyclic){
      if (cyclic==1 && p.type==_INT_) 
	return makemod(generator(p.val),p);
      gen g=prime_factors(o,true,contextptr);
      if (g.type!=_VECT) return undef;
      vecteur & v=*g._VECTptr;
      vecteur w;
      for (int i=0;i<v.size();++i){
	w.push_back(o/v[i]);
      }
      for (int a=2;a<65536;++a){
	if (gcd(a,p)!=1)
	  continue;
	int i;
	for (i=0;i<w.size();++i){
	  if (powmod(a,w[i],p)==1)
	    break;
	}
	if (i==w.size())
	  return makemod(a,p);
      }
      return undef;
    }
#ifdef HAVE_LIBPARI
    if (!is_integer(p))
      return gentypeerr(contextptr);
    return _pari(makesequence(string2gen("znprimroot",false),p),contextptr);
#endif
    if (p.type!=_INT_ || !is_probab_prime_p(p))
      return gentypeerr("PARI not compiled in => currently, znprimroot(k) expects a prime<2^31");
    return makemod(generator(p.val),p);
  }
  static const char _znprimroot_s []="znprimroot";
  static define_unary_function_eval (__znprimroot,&_znprimroot,_znprimroot_s);
  define_unary_function_ptr5( at_znprimroot ,alias_at_znprimroot,&__znprimroot,0,true); 

  int znorder(int k,int p,int phi,const vecteur & v){
    int o=1;
    for (int i=0;i<v.size();i+=2){
      int pi=v[i].val;
      int mi=v[i+1].val;
      int pimi=pow((unsigned) pi,(unsigned) mi).val;
      int a=powmod(k,phi/pimi,p);
      while (a!=1){
	o *= pi;
	a=powmod(a,pi,p);
      }
    }
    return o;
  }

  int znorder(int k,int p){
    k %= p;
    if (gcd(k,p)!=1)
      return 0;
    if (k==1)
      return 1;
    int phi=euler(p,context0).val;
    vecteur v=ifactors(phi,context0);
    return znorder(k,p,phi,v);
  }

  gen znorder(const gen & k,const gen & p,const gen & phi,const vecteur & v){
    gen o=1;
    for (int i=0;i<v.size();i+=2){
      gen pi=v[i];
      if (is_greater(1,pi,context0))
	continue;
      int mi=v[i+1].val;
      gen pimi=pow(pi,mi);
      gen a=powmod(k,phi/pimi,p);
      while (a!=1){
	o = o*pi;
	a=powmod(a,pi,p);
      }
    }
    return o;
  }

  gen znorder(gen & k,const gen & p){
    k=k % p;
    if (gcd(k,p)!=1)
      return 0;
    if (k==1)
      return 1;
    gen phi=_euler(p,context0);
    gen v=_ifactors(phi,context0);
    if (v.type!=_VECT)
      return undef;
    return znorder(k,p,phi,*v._VECTptr);
  }

  gen _znorder(const gen & args,GIAC_CONTEXT){
    if (args.type==_MOD)
      return _znorder(makevecteur(*args._MODptr,*(args._MODptr+1)),contextptr);
    if (args.type!=_VECT || args._VECTptr->size()!=2)
      return gensizeerr(contextptr);
    gen k=args._VECTptr->front(),p=args._VECTptr->back();
#ifdef HAVE_LIBPARI
    if (gcd(p,k)!=1)
      return 0;
    return _pari(makesequence(string2gen("znorder",false),makemod(k,p)),contextptr);
#endif
    if (is_greater(1,p,contextptr))
      return undef;
    if (k.type==_INT_ && p.val==_INT_ )
      return znorder(k.val,p.val);
    return znorder(k,p);
  }
  static const char _znorder_s []="znorder";
  static define_unary_function_eval (__znorder,&_znorder,_znorder_s);
  define_unary_function_ptr5( at_znorder ,alias_at_znorder,&__znorder,0,true); 

  // b1 *= m mod p
  //  m += (m>>31) &p;
  //  int msurp=((1LL<<31)*m)/p+1;
  inline int precond_mulmod31(int b1,int m,int p,int msurp){
    // b1 += (b1>>31) &p;
    int t=longlong(b1)*m-((longlong(b1)*msurp)>>31)*p;
    // t += (t>>31)&p; // t positive (or at least t-p is valid)
    return t;
  }

  // Harvey algorithm for Bernoulli numbers
  // https://arxiv.org/pdf/0807.1347.pdf
  // k must be even and p prime
  // https://web.maths.unsw.edu.au/~davidharvey/code/bernmm/index.html
  // bernmm lib
  int bernoulli_mod(int k,int p){
#ifdef HAVE_LIBBERNMM
    return bernmm::bern_modp(p,k);
#endif
    if (k>p-3){
      int m=k % (p-1); // now m<p-1 is even therefore <=p-3
      int bm=bernoulli_mod(m,p);
      // bk/k=bm/m mod p
      return ( ( (longlong(bm)*k) %p)*invmod(m,p) )%p;
    }
    vecteur v=ifactors(p-1,context0);
    int g=generator(p,v),r=powmod(g,k-1,p),u;
    int N=p>11?znorder(2,p,p-1,v):0;
    if (debug_infolevel)
      CERR << CLOCK()*1e-6 << " end generator/znorder \n";
    if (N>4 && k%N){
      // faster summation is possible
      int n=(N%2)?N:N/2;
      int m=(p-1)/2/n;
      // twokm1=2^(k-1), gi=g^i, gkm1i=(g^(k-1))^i
      longlong S=0,twokm1=powmod(2,(k-1)%N,p),gi=1,gkm1i=1;
      int msurp=((1LL<<31)*twokm1)/p+1;
      for (int i=0;i<m;++i){
	longlong s=0,pow2=1;
	int gi2j=gi; // g^i*2^j
	for (int j=0;j<n;++j){
#if 1
	  gi2j = (gi2j<<1) -p;
	  s -= (1+ ((gi2j>>31)<<1))*pow2;// (2*(1+(gi2j>>31))-1)*pow2;
	  gi2j -= (gi2j>>31)*p;
	  pow2=precond_mulmod31(pow2,twokm1,p,msurp);// (pow2*twokm1)%p;
#else	  
	  gi2j <<= 1;
	  if (gi2j>=p){
	    gi2j -= p;
	    // f=-1
	    s -= pow2;
	    if (s<0)
	      s += p;
	  }
	  else {
	    // f=1
	    s += pow2;
	    if (s>=p)
	      s -= p;
	  }
	  pow2=(pow2*twokm1)%p;
#endif
	}
	// update g^i for next i iteration
	gi=(gi*g)%p;
	// s*(g^(k-1))^i
	s=((s%p)*gkm1i)%p;
	S += s;
	if (S>=p)
	  S -=p;
	// update (g^(k-1))^i for next i iteration
	gkm1i=(gkm1i*r)%p;
      }
      // final answer k/(2^(-(k-1))-2)*S
      S=(S*k)%p;
      S=(S*invmod(invmod(twokm1,p)-2,p))%p;
      return S;
    }
    if (g%2)
      u=(g-1)/2;
    else
      u=(longlong(g-1)*invmod(2,p))%p;
    int S=0,X=1,Y=r;
    for (int i=1;i<=p/2;i++){
      int q=(longlong(g)*X)/p;
      S=(S+(longlong(u)-q)*Y) % p;
      X=(longlong(g)*X) % p;
      Y=(longlong(r)*Y) % p;
    }
    int res=(2*longlong(k)*S)%p;
    res=(longlong(res)*invmod(1-powmod(g,k,p),p))%p;
    return res;
  }

#ifndef USE_GMP_REPLACEMENTS
  void ichinrem_inplace(int r,int m,gen & res,const gen & pim,int & proba,mpz_t & tmpz){
    if (pim.type==_ZINT && res.type==_ZINT){
      longlong amodm=mpz_fdiv_ui(*res._ZINTptr,m);
      if (amodm!=r){
	gen u,v,d; longlong U;
	egcd(pim,m,u,v,d);
	if (u.type==_ZINT)
	  U=mpz_fdiv_ui(*u._ZINTptr,m);
	else
	  U=u.val;
	if (d==-1){ U=-U; v=-v; d=1; }
	mpz_mul_si(tmpz,*pim._ZINTptr,(U*(r-amodm))%m);
	mpz_add(*res._ZINTptr,*res._ZINTptr,tmpz);
	proba=0;
      }
      else ++proba;
    }
  }
#endif

  const double m_ln2=0.69314718055994531;
  // Inspired by David Harvey code (bernmm)
  gen bernoulli_rat(int k){
    long bound1 = (long) std::ceil((k + 0.5) * std::log(double(k)) /m_ln2);
    if (bound1<37)
      bound1=37;
    // Computes the denominator of B_k using Clausen/von Staudt.
    // loop through factors of k
    gen D=1;
    for (int f=1; f*f<=k; f++){
      // if f divides k....
      if (k % f == 0){
	// ... then both f + 1 and k/f + 1 are candidates for primes
	// dividing the denominator of B_k
	if (is_probab_prime_p(f+1))
	  D = (f+1)*D; 
	if (f*f != k){
	  int tmp=k/f+1;
	  if (is_probab_prime_p(tmp))
	    D = tmp*D;
	}
      }
    }
    double bits= (k+0.5)*std::log(double(k))/m_ln2 - 4.094*k + 2.470 +
      std::log(evalf_double(D,1,context0)._DOUBLE_val)/m_ln2 ;
    gen res(0.0),pip=1;
    mpz_t tmpz; mpz_init(tmpz);
    for (int p = 5; ; p = nextprime(p+1).val){
      if (k % (p-1) == 0)
	continue;
      if (debug_infolevel)
	COUT << CLOCK()*1e-6 << " start bernoulli_mod " << p << '\n';
      int cur=bernoulli_mod(k,p);
      if (debug_infolevel)
	COUT << CLOCK()*1e-6 << " end bernoulli_mod " << p << '\n';
      if (res.type==_DOUBLE_)
	res=cur;
      else {
#ifndef USE_GMP_REPLACEMENTS
	if (res.type==_ZINT && pip.type==_ZINT){
	  int proba=0; // not used
	  ichinrem_inplace(cur,p,res,pip,proba,tmpz);
	} else
#endif
	  res=ichinrem(gen(cur),res,gen(p),pip);
      }
      pip = p*pip;
      bits -= std::log(double(p))/m_ln2;
      if (bits<-1)
	break;
    }
    mpz_clear(tmpz);
    res=smod(res*D,pip);
    int s=fastsign(res,context0);
    if (k%4==2){
      if (s==-1)
	res += pip;
    }
    else {
      if (s==1)
	res -= pip;
    }
    //COUT << _evalf(makesequence(res/pip,30),context0) << '\n';
    return res/D;
  }
  
  gen _bernoulli_mod(const gen & args,GIAC_CONTEXT){
    if (args.type!=_VECT || args._VECTptr->size()!=2)
      return gensizeerr(contextptr);
    gen k=args._VECTptr->front(),p=args._VECTptr->back();
    if (k.type!=_INT_ || k.val<2 || k.val%2 || p.type!=_INT_  || !is_probab_prime_p(p) )
      return gentypeerr(contextptr);
    return bernoulli_mod(k.val,p.val);
  }
  static const char _bernoulli_mod_s []="bernoulli_mod";
  static define_unary_function_eval (__bernoulli_mod,&_bernoulli_mod,_bernoulli_mod_s);
  define_unary_function_ptr5( at_bernoulli_mod ,alias_at_bernoulli_mod,&__bernoulli_mod,0,true); 

  inline void new_xab(int & x, int & a, int& b,const int N,const int n,const int alpha,const int beta) {
    switch (x % 3) {
    case 0: 
      x = longlong(x)*x % N;  
      a = longlong(a)*2 % n;  
      b = longlong(b)*2 % n;  
      break;
    case 1: 
      x = longlong(x)*alpha % N;  
      a = (a+1) % n;                  
      break;
    case 2: 
      x = longlong(x)*beta % N;                  
      b = (b+1) % n;  
      break;
    }
  }

  // N prime, solve alpha^e=beta mod N using Pollard-Rho
  int baby64(int alpha,int beta,int N,int n){
    int x = 1, a = 0, b = 0; // x=alpha^a*beta^b
    int X = x, A = a, B = b; // X=alpha^A*beta^B
    for (int i = 1; i < n; ++i) {
      new_xab(x,a,b,N,n,alpha,beta);
      new_xab(X,A,B,N,n,alpha,beta);
      new_xab(X,A,B,N,n,alpha,beta);
      if (x==X){ 
	// alpha^a*beta^b=alpha^A*beta^B hence
	// alpha^(A-a)=beta^(B-b) mod n, (B-b)*e=(A-a) mod n
	int b1=B-b,a1=a-A;
	int g=gcd(b1,n);
	if (a1%g) 
	  return -1;
	// b1*e=a1+k*n1 <-> b2*e=a2+k*n2
	int b2=b1/g,a2=a1/g,n2=n/g;
	longlong b3=invmod(b2,n2); // e=b3*a2 mod n2 
	int e0=(b3*a2) % n2;
	for (longlong k=0;k<g;++k){
	  int e=e0+k*n2;
	  if (e<0) e+=n; 
	  longlong chk=powmod(alpha,e,N);
	  if ((chk-beta)%N==0)
	    return e;
	  if ((chk+beta)%N==0)
	    return e+n/2;
	}
      }
    }
    return -1; // means not found
  }

#ifdef INT128
  inline void new_xab(longlong & x, longlong & a, longlong& b,const longlong N,const longlong n,const longlong alpha,const longlong beta) {
    switch (x % 3) {
    case 0: 
      x = int128_t(x)*x % N;  
      a = int128_t(a)*2 % n;  
      b = int128_t(b)*2 % n;  
      break;
    case 1: 
      x = int128_t(x)*alpha % N;  
      a = (a+1) % n;                  
      break;
    case 2: 
      x = int128_t(x)*beta % N;                  
      b = (b+1) % n;  
      break;
    }
  }

  // N prime, solve alpha^e=beta mod N using Pollard-Rho
  longlong baby128(longlong alpha,longlong beta,longlong N,longlong n){
    longlong x = 1, a = 0, b = 0; // x=alpha^a*beta^b
    longlong X = x, A = a, B = b; // X=alpha^A*beta^B
    for (longlong i = 1; i < n; ++i) {
      new_xab(x,a,b,N,n,alpha,beta);
      new_xab(X,A,B,N,n,alpha,beta);
      new_xab(X,A,B,N,n,alpha,beta);
      if (x==X){ 
	// alpha^a*beta^b=alpha^A*beta^B hence
	// alpha^(A-a)=beta^(B-b) mod n, (B-b)*e=(A-a) mod n
	longlong b1=B-b,a1=a-A;
	longlong g=gcd(b1,n);
	if (a1%g) 
	  return -1;
	// b1*e=a1+k*n1 <-> b2*e=a2+k*n2
	longlong b2=b1/g,a2=a1/g,n2=n/g;
	int128_t b3=invmod(b2,n2); // e=b3*a2 mod n2 
	if (b3<0) b3+=n2;
	longlong e0=(b3*a2) % n2;
	for (longlong k=0;k<g;++k){
	  longlong e=e0+k*n2;
	  int128_t chk=powmod(alpha,e,N);
	  if ((chk-beta)%N==0)
	    return e;
	  if ((chk+beta)%N==0)
	    return e+n/2;
	}
      }
    }
    return -1; // means not found
  }
#endif

  void new_xab(gen & x, gen & a, gen& b,const gen & N,const gen & n,const gen & alpha,const gen & beta) {
    mpz_t & xz=*x._ZINTptr;
    mpz_t & az=*a._ZINTptr;
    mpz_t & bz=*b._ZINTptr;
    const mpz_t & Nz=*N._ZINTptr;
    const mpz_t & nz=*n._ZINTptr;
    int t=modulo(xz,3);
    switch (t) {
    case 0: 
      mpz_mul(xz,xz,xz); mpz_tdiv_r(xz,xz,Nz); // x = longlong(x*x) % N;  
      mpz_mul_ui(az,az,2); mpz_tdiv_r(az,az,nz);  // a = longlong(a*2) % n;  
      mpz_mul_ui(bz,bz,2); mpz_tdiv_r(bz,bz,nz); // b = longlong(b*2) % n;  
      break;
    case 1: 
      mpz_mul(xz,*alpha._ZINTptr,xz); mpz_tdiv_r(xz,xz,Nz); // x = longlong(x*alpha) % N;  
      mpz_add_ui(az,az,1);  mpz_tdiv_r(az,az,nz); // a = (a+1) % n;                  
      break;
    case 2: 
      mpz_mul(xz,*beta._ZINTptr,xz); mpz_tdiv_r(xz,xz,Nz); // x = longlong(x*beta) % N;                  
      mpz_add_ui(bz,bz,1);  mpz_tdiv_r(bz,bz,nz); // b = (b+1) % n;  
      break;
    }
  }

  // N prime, solve alpha^e=beta mod N using Pollard-Rho
  gen baby(const gen & alpha,const gen & beta,const gen & N,const gen & n_){
    if (alpha.type==_INT_ || beta.type==_INT_ || N.type==_INT_){
      gen al=alpha,be=beta,NN=N;
      al.uncoerce(); be.uncoerce(); NN.uncoerce();
      return baby(al,be,NN,n_);
    }
    gen n=n_; n.uncoerce();
    gen x = 1, a = 0, b = 0; // x=alpha^a*beta^b
    gen X = x, A = a, B = b; // X=alpha^A*beta^B
    x.uncoerce(); a.uncoerce(); b.uncoerce(); X.uncoerce(); A.uncoerce(); B.uncoerce();
    for (longlong i=0; i<(1LL<<46); ++i) {
      new_xab(x,a,b,N,n,alpha,beta);
      new_xab(X,A,B,N,n,alpha,beta);
      new_xab(X,A,B,N,n,alpha,beta);
      if (x==X){ 
	// alpha^a*beta^b=alpha^A*beta^B hence
	// alpha^(A-a)=beta^(B-b) mod n, (B-b)*e=(A-a) mod n
	gen b1=B-b,a1=a-A;
	gen g=gcd(b1,n);
	if (a1%g!=0) 
	  return -1;
	// b1*e=a1+k*n1 <-> b2*e=a2+k*n2
	gen b2=b1/g,a2=a1/g,n2=n/g;
	gen b3=invmod(b2,n2); // e=b3*a2 mod n2 
	if (is_positive(-b3,context0))
	  b3 += n2;
	gen e0=(b3*a2) % n2;
	for (gen k=0;is_greater(g,k,context0);k+=1){
	  gen e=e0+k*n2;
	  gen chk=powmod(alpha,e,N);
	  if ((chk-beta)%N==0)
	    return e;
	  if ((chk+beta)%N==0) 
	    return e+n/2; // works only if alpha^(n/2)=-1
	}
      }
    }
    return -1; // means not found
  }

  // solve g^x=h mod N, where g^(p^e)=1 mod N and g^(p^(e-1))!=1 mod N
  // start from the fact that h^-1*g^0 is of order p^(e)  mod N, x0=0
  // then for k>=0 find dk such that
  // h^-1*g^(xk+dk*p^(k)) is of order p^(e-(k+1)) mod N 
  // where h^-1*g^xk of order p^(e-k) mod N
  // (g^(-xk)*h)^-1*g^(dk*p^(k)) must be of order p^(e-(k+1)) mod N
  // let hk:=(g^(-xk)*h)^(p^(e-1-k)) and gamma=g^(p^(e-1)
  // Take power p^(e-1-k) -> hk^-1*gamma^dk of order 1 mod N
  // gamma^dk=hk mod N
  gen padic_logb(const gen & g,const gen & h,const gen & p,int e,const gen & N){
    if (h==1) return 0;
    if (h==g) return 1;
    gen xk=0;
    gen pe1=pow(p,e-1),pe=pe1*p,invg=invmod(g,N);
    if (is_positive(-invg,context0))
      invg+=N;
    gen gamma=powmod(g,pe1,N);
    gen pk=1;
    for (int k=0;k<e;++k){
      gen g1=powmod(invg,xk,N)*h;
      gen hk=powmod(g1,pow(p,e-1-k),N);
      gen dk=logb(hk,gamma,N,context0,p);
      xk = xk+dk*pk;
      pk=p*pk;
    }
    return xk;
  }

  gen logb(const gen & a,const gen &b,const gen & N,GIAC_CONTEXT,gen order){
    if (a==0)
      return undef;
    if (N==2)
      return 0;
    if (a==b)
      return 1;
    if (a==1)
      return 0;
    gen n(order);
    if (order==0){
      n=N-1;
      if (_isprime(N,context0)==0)
	return gensizeerr(gettext("Expecting an odd prime as 3rd argument"));
      // solve b^x=a mod N (that's x=log_b(a))
      gen chk=powmod(b,(N-1)/2,N);
      if ((chk+1) % N !=0)
	*logptr(contextptr) << "2nd arg is not a generator, answer might be wrong\n";
      gen n=N-1;
      vecteur v(ifactors(n,contextptr));
      if (v.size()==2){
	gen res=padic_logb(b,a,v[0],v[1].val,N);
	return res;
      }
      // chinese remaindering step
      gen x,curprod(1);
      for (int i=0;i<v.size();i+=2){
	gen pi=v[i],ei=v[i+1];
	if (ei.type!=_INT_)
	  return gensizeerr(contextptr);
	gen piei=pow(pi,ei.val),coorder=n/piei;
	gen gi=powmod(b,coorder,N);
	gen hi=powmod(a,coorder,N);
	gen xi=padic_logb(gi,hi,pi,ei.val,N);
	if (i==0) 
	  x=xi;
	else
	  x=ichinrem(x,xi,curprod,piei);
	curprod=curprod*piei;
      }
      if (is_positive(x,contextptr))
	return x;
      return x+n;
    }
    if (is_integer(a) && is_integer(b) && N.type==_INT_){
      int A=a.type==_INT_?a.val:modulo(*a._ZINTptr,N.val),B=b.type==_INT_?b.val:modulo(*b._ZINTptr,N.val);
      int res=baby64(B,A,N.val,order.val);
      if (order!=0)
	return res % order.val;
      return res;
    }
    // ifactor n==N-1
    gen res=baby(a,b,N,order);
    if (order!=0)
      return res % order;
    return res;
  }

  // elliptic curve method, 
  // http://math.univ-lyon1.fr/~roblot/resources/factorisation.pdf
  // This is a very naive implementation
  // It does not use Montgomery representation and only phase 1
  // For professional implementations, cf.
  // https://members.loria.fr/PZimmermann/papers/ecm-submitted.pdf
  // https://pdfs.semanticscholar.org/e8eb/13b75292b15dd63c3e7e4b1c8dc334d278ba.pdf
  // ecm will be used if available
#define ECM_MAXITER 1000
  static gen L(double alpha,double beta,double N){
    double lnN=std::log(N);
    return std::exp(beta*std::pow(lnN,alpha)*std::pow(std::log(lnN),1-alpha));
  }


#ifndef USE_GMP_REPLACEMENTS
  // addition in elliptic curve, returns 1 on success or 0 and m=a divisor of n
  int ecm_add(const mpz_t &x1,const mpz_t &y1,const mpz_t & x2,const mpz_t &y2,const mpz_t & a,const mpz_t & n,mpz_t & m,mpz_t & x,mpz_t &y){
    if (mpz_cmp(x1,x2)){
      mpz_sub(x,x2,x1); // x=x2-x1
      int res=mpz_invert(m,x,n); // m=inv(x2-x1) mod n
      if (res==0){ // not invertible
	mpz_gcd(m,x,n);
	return 0; // m has non trivial gcd with n
      }
      mpz_sub(y,y2,y1);
      mpz_mul(m,m,y); // m=(y2-y1)*invmod(x2-x1,n);
    }
    else {
      mpz_mul_ui(y,y1,2);
      int res=mpz_invert(m,y,n); // m=inv(2*y) mod n
      if (res==0){ // not invertible
	mpz_gcd(m,y,n);
	return 0; // m has non trivial gcd with n
      }
      mpz_mul(x,x1,x1);
      mpz_mul_ui(x,x,3);
      mpz_add(x,x,a);
      mpz_mul(m,m,x); // m=(3*x1*x1+a)*invmod(2*y1,n);
    }
    mpz_fdiv_r(m,m,n); // m=mod(m,n);
    mpz_mul(x,m,m);
    mpz_sub(x,x,x1);
    mpz_sub(x,x,x2);
    mpz_fdiv_r(x,x,n); // x=mod(m*m-x1-x2,n);
    mpz_sub(y,x1,x);
    mpz_mul(y,m,y);
    mpz_sub(y,y,y1);
    mpz_fdiv_r(y,y,n); // y=mod(m*(x1-x)-y1,n);
    // smod x and y
    mpz_add(m,x,x);
    if (mpz_cmp(m,n)>0)
      mpz_sub(x,x,n);
    mpz_add(m,y,y);
    if (mpz_cmp(m,n)>0)
      mpz_sub(y,y,n);
    return 1;
  }
#endif

  gen ecm_add(const gen &x1,const gen &y1,const gen & x2,const gen &y2,const gen & a,const gen & n,gen & m,gen & x,gen &y){
    if (is_inf(x1)){
      x=x2; y=y2; return 1;
    }
    if (is_inf(x2)){
      x=x1; y=y1; return 1;
    }
    if (y1+y2==0){
      y=x=unsigned_inf; return 1;
    }
#ifndef USE_GMP_REPLACEMENTS
    if (x1.type==_ZINT && y1.type==_ZINT && x2.type==_ZINT && y2.type==_ZINT && a.type==_ZINT && n.type==_ZINT ){
      m=gen(1LL<<33);
      x=gen(1LL<<33);
      y=gen(1LL<<33);
      if (!ecm_add(*x1._ZINTptr,*y1._ZINTptr,*x2._ZINTptr,*y2._ZINTptr,*a._ZINTptr,*n._ZINTptr,*m._ZINTptr,*x._ZINTptr,*y._ZINTptr)){
	return gen(*m._ZINTptr);
      }
      return 1;
    }
#endif
    if (x1!=x2){
      m=gcd(x2-x1,n);
      if (m!=1)
	return m;
      m=(y2-y1)*invmod(x2-x1,n);
    }
    else {
      m=gcd(y1,n);
      if (m!=1)
	return m;
      m=(3*x1*x1+a)*invmod(2*y1,n);
    }
    m=smod(m,n);
    x=smod(m*m-x1-x2,n);
    y=smod(m*(x1-x)-y1,n);
    return 1;
  }
  // multiplication in elliptic curve,
  gen ecm_mult(const gen &x1,const gen &y1,ulonglong m,const gen & a,const gen & n,gen & x,gen &y){
    gen x2(x1),y2(y1),xtmp,ytmp,g,M;
    y=x=plus_inf;
    while (m){
      if (m%2){
	g=ecm_add(x,y,x2,y2,a,n,M,xtmp,ytmp);
	if (g!=1) 
	  return g;
	swapgen(x,xtmp); swapgen(y,ytmp);// x=xtmp; y=ytmp;
      }
      m/=2;
      g=ecm_add(x2,y2,x2,y2,a,n,M,xtmp,ytmp); // improve: ecmdup
      if (g!=1) 
	return g;
      swapgen(x2,xtmp);swapgen(y2,ytmp);// x2=xtmp; y2=ytmp;
    }
    return 1;
  }
  gen _ecm_factor(const gen &n_,GIAC_CONTEXT){
    gen B,n(n_);
    int maxiter(ECM_MAXITER);
    if (n.type==_VECT && n._VECTptr->size()>=2){
      const vecteur & v=*n._VECTptr;
      B=v[1];
      if (v.size()>=3 && v[2].type==_INT_)
	maxiter=giacmax(1,v[2].val);
      n=v.front();
    }
    if (!is_integer(n) || is_positive(-n,contextptr))
      return gensizeerr(contextptr);
    if (_isprime(n,contextptr)!=0)
      return n;
    double logp=.5*std::log(evalf_double(n,1,contextptr)._DOUBLE_val);
#ifdef HAVE_LIBECM
    double epsilon=.02; // to be adjusted
#else
    double epsilon=.45; // to be adjusted
#endif
    if (logp>80) // research factors of size not exceeding 35 digits
      logp=80;
    if (B==0)
      B=L(.5,0.707+epsilon,std::exp(logp));
    // B=1000;
    B=_ceil(B,contextptr);
#ifdef HAVE_LIBECM
    *logptr(contextptr) << "ECM-GMP factor n="<< n << " , B=" << B << ", #curves <=" << maxiter << '\n';
    n.uncoerce();
    double B1=evalf_double(B,1,contextptr)._DOUBLE_val;
    /* From ECM README, table of optimal values of B1
       digits D  optimal B1   default B2           expected curves
                                                       N(B1,B2,D)
                                              -power 1         default poly
          20       11e3         1.9e6             74               74 [x^1]
          25        5e4         1.3e7            221              214 [x^2]
          30       25e4         1.3e8            453              430 [D(3)]
          35        1e6         1.0e9            984              904 [D(6)]
          40        3e6         5.7e9           2541             2350 [D(6)]
          45       11e6        3.5e10           4949             4480 [D(12)]
          50       43e6        2.4e11           8266             7553 [D(12)]
          55       11e7        7.8e11          20158            17769 [D(30)]
          60       26e7        3.2e12          47173            42017 [D(30)]
          65       85e7        1.6e13          77666            69408 [D(30)]

     */
    int res;
    gen F(1LL<<33);
    for (int i=0;i<maxiter;++i){
      res=ecm_factor(*F._ZINTptr, *n._ZINTptr, B1, 0);
      if (res!=0) break;
    }
    if (res==0) return undef;
    return F;
#else
    *logptr(contextptr) << "ECM naive factor n="<< n << " , B=" << B << ", #curves <=" << maxiter << '\n';
    for (int i=0;i<maxiter;++i){
      gen a,x,y,b,d,g;
      a= rand_interval(makevecteur(0,n-1),true,contextptr);// a=1078104638; 
      x= smod(rand_interval(makevecteur(0,n-1),true,contextptr),n);// 317359960;
      y= smod(rand_interval(makevecteur(0,n-1),true,contextptr),n);// 983830906;
      b=smod(y*y-x*x*x-a*x,n);
      if (debug_infolevel)
	COUT << CLOCK()*1e-6 << " Factor "<< n << " ECM curve " << i << ", B="<<B << ", a=" << a << ", x=" << x << ", y=" << y << '\n';
      d=4*a*a*a-27*b*b;
      g=gcd(d,n);
      if (g==n)
	continue;
      if (g!=1)
	return g;
      gen p(2),pe,tmp,xm,ym;
      for (;is_greater(B,pe=p*p,contextptr);p=nextprime(p+1)){
	int e=2;
	while (is_greater(B,tmp=pe*p,contextptr)){
	  ++e;
	  pe=tmp;
	}
	tmp=evalf_double(pe,1,contextptr);
	if (pe==tmp){
	  g=ecm_mult(x,y,ulonglong(tmp._DOUBLE_val),a,n,xm,ym);
	  if (g!=1){
	    if (debug_infolevel)
	      COUT << CLOCK()*1e-6 << " ECM success p=" << p << ", p^e=" << pe << '\n';
	    return g;
	  }
	  swapgen(x,xm); swapgen(y,ym); // x=xm;y=ym;
	}
      }
      B=_floor(1.001*B,contextptr)+1; // to be adjusted
    } // end maxiter loop
    return undef;
#endif
  }
  static const char _ecm_factor_s []="ecm_factor";
  static define_unary_function_eval (__ecm_factor,&_ecm_factor,_ecm_factor_s);
  define_unary_function_ptr5( at_ecm_factor ,alias_at_ecm_factor,&__ecm_factor,0,true);

  // Find a list of prime factors 
  // such that g/factors^multiplicity<sqrt(g) if full==false
  // if not found return undef
  gen prime_factors(const gen &g0,bool full,GIAC_CONTEXT){
    gen g(g0);
    vecteur res;
    if (full){
      g=_ifactors(g0,contextptr);
      if (g.type!=_VECT)
	return undef;
      vecteur & v=*g._VECTptr;
      for (int i=0;i<v.size();i+=2){
	if (is_greater(v[i],1,contextptr))
	  res.push_back(v[i]);
      }
      return res;
    }
    gen gstop=isqrt(g);
    // trivial factor
    for (int i=0;i<sizeof(giac_primes)/sizeof(short int);++i){
      int p=giac_primes[i];
      if (modulo(g,p))
	continue;
      res.push_back(p);
      for (;;){
	g=g/p;
	if (modulo(g,p))
	  break;
      }
      if (is_greater(gstop,g,contextptr))
	return res;
      if (is_greater(g0,g*g,contextptr)){
	res.push_back(g);
	return res;
      }
    }
    // pollard(a,k,contextptr) -> factor or -1/0
    for (;;){
      if (is_greater(gstop,g,contextptr))
	return res;
      if (is_probab_prime_p(g)){
	// leave the user compute a certificate for this prime factor...
	res.push_back(g);
	return res;
      }
      gen b=pollard(g,1,contextptr);
      if (!is_greater(b,2,contextptr)){
	// _ecm_factor(n,contextptr) -> factor or undef
	b=_ecm_factor(g,contextptr);
	if (is_undef(b))
	  return undef; // could not partial factor
      }
      gen c=_ifactors(b,contextptr);
      if (c.type!=_VECT)
	return undef;
      vecteur & v=*c._VECTptr;
      for (int i=0;i<v.size();i+=2){
	if (is_greater(v[i],2,contextptr))
	  res.push_back(v[i]);
      }
      g=g/b;
    }
    return undef; // could not partial factor (never reached)
  }

  gen prime_cert(const gen & g,GIAC_CONTEXT){
    if (!is_probab_prime_p(g))
      return 0;
    gen o=g-1; // order
    gen lf=prime_factors(o,false,contextptr); // partial list of prime factors
    if (lf.type!=_VECT)
      return undef;
    vecteur & v=*lf._VECTptr;
    // for each element p of v, we must find a such that 
    // * a^o==1 mod g
    // * gcd(a^(o/p)-1,g)==1
    vecteur res;
    for (int i=0;i<v.size();++i){
      gen p=v[i];
      for (int a=2;;++a){
	if (a==RAND_MAX) return undef;
	if (powmod(a,o,g)!=1)
	  continue;
	if (gcd(powmod(a,o/p,g)-1,g)==1){
	  res.push_back(makevecteur(p,a,p.type==_INT_?1:0));
	  break;
	}
      }
    }
    return res;
  }

#ifndef NO_NAMESPACE_GIAC
} // namespace giac
#endif // ndef NO_NAMESPACE_GIAC
