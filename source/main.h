/* 

xsample - extended sample objects for Max/MSP and pd (pure data)

Copyright (c) 2001,2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __XSAMPLE_H
#define __XSAMPLE_H

#define XSAMPLE_VERSION "0.2.3"


#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 202)
#error You need at least flext version 0.2.2
#endif


// most compilers are somehow broken.....
// in other words: can't handle all C++ features

#if defined(_MSC_VER)
// MS VC 6.0 can't handle <int,int> templates?! -> no optimization
//	#define TMPLOPT
#elif defined(__BORLANDC__) 
// handles all optimizations
	#define TMPLOPT 
#elif defined(__GNUC__)
// GNUC 2.95.2 dies at compile with <int,int> templates
	#define TMPLOPT
#elif defined(__MWERKS__)
// CodeWarrior can't take address of a template member function 
	#define TMPLOPT
	#define SIGSTATIC
#elif defined(__MRC__)
// Apple MPW - MrCpp
//	#define TMPLOPT  // template optimation for more speed
#else
// another compiler
//	#define TMPLOPT  // template optimation for more speed (about 10%)
	//#define SIGSTATIC  // another redirection to avoid addresses of class member functions
#endif


// lazy me
#define F float
#define D double
#define I int
#define L long
#define C char
#define V void
#define BL bool


class xsample:
	public flext_dsp
{
	FLEXT_HEADER(xsample,flext_dsp)
	
public:
	xsample();
	~xsample();
	
	enum xs_unit {
		xsu__ = -1,  // don't change
		xsu_sample = 0,xsu_buffer,xsu_ms,xsu_s
	};
	
	enum xs_intp {
		xsi__ = -1,  // don't change
		xsi_none = 0,xsi_4p,xsi_lin
	};
	
	enum xs_sclmd {
		xss__ = -1,  // don't change
		xss_unitsinbuf = 0,xss_unitsinloop,xss_buffer,xss_loop
	};
	
protected:
	buffer *buf;

	virtual V m_start() = 0;
	virtual V m_stop() = 0;
	virtual V m_reset();

  	virtual I m_set(I argc,t_atom *argv);
	virtual V m_print() = 0;
	virtual V m_refresh();
	virtual V m_loadbang();

	virtual V m_units(xs_unit u = xsu__);
//	virtual V m_iunits(xs_unit u = xsu__);
//	virtual V m_ounits(xs_unit u = xsu__);
	virtual V m_sclmode(xs_sclmd u = xss__);
//	virtual V m_isclmode(xs_sclmd u = xss__);
//	virtual V m_osclmode(xs_sclmd u = xss__);
	virtual V m_interp(xs_intp u = xsi__);

	virtual V m_all();
	virtual V m_min(F mn);
	virtual V m_max(F mx);

	virtual V m_dsp(I n,F *const *insigs,F *const *outsigs);
	virtual V s_dsp() = 0;

	xs_unit unitmode; //iunitmode,ounitmode;
	xs_sclmd sclmode; //isclmode,osclmode;
	xs_intp interp;

	I curmin,curmax,curlen;  // in samples
	I sclmin; // in samples
	F sclmul;
	F s2u;  // sample to unit conversion factor

	inline F scale(F smp) const { return (smp-sclmin)*sclmul; }

private:

	FLEXT_CALLBACK(m_start)
	FLEXT_CALLBACK(m_stop)

	FLEXT_CALLBACK_G(m_set)
	FLEXT_CALLBACK(m_print)
	FLEXT_CALLBACK(m_refresh)
	FLEXT_CALLBACK(m_reset)

	FLEXT_CALLBACK_1(m_units,xs_unit)
//	FLEXT_CALLBACK_1(m_iunits,xs_unit)
//	FLEXT_CALLBACK_1(m_ounits,xs_unit)
	FLEXT_CALLBACK_1(m_sclmode,xs_sclmd)
//	FLEXT_CALLBACK_1(m_isclmode,xs_sclmd)
//	FLEXT_CALLBACK_1(m_osclmode,xs_sclmd)
	FLEXT_CALLBACK_1(m_interp,xs_intp)
};


// defines which are used in the derived classes
#ifdef SIGSTATIC
	#ifdef TMPLOPT
		#define SIGFUN(CL,FUN,BCHNS,IOCHNS) &CL::st_##FUN<BCHNS,IOCHNS>
		#define TMPLDEF template <int _BCHNS_,int _IOCHNS_>
		#define TMPLCALL <_BCHNS_,_IOCHNS_>
	#else
		#define SIGFUN(CL,FUN,BCHNS,IOCHNS) &CL::st_##FUN
		#define TMPLDEF 
		#define TMPLCALL
	#endif
#else
	#ifdef TMPLOPT
		#define SIGFUN(CL,FUN,BCHNS,IOCHNS) &CL::FUN<BCHNS,IOCHNS>
		#define TMPLDEF template <int _BCHNS_,int _IOCHNS_>
		#define TMPLCALL <_BCHNS_,_IOCHNS_>
	#else
		#define SIGFUN(CL,FUN,BCHNS,IOCHNS) &CL::FUN
		#define TMPLDEF 
		#define TMPLCALL
	#endif
#endif

#ifndef SIGSTATIC
	// member function
	#define DEFSIGFUN(CL) \
	V (CL::*sigfun)(I n,F *const *in,F *const *out);  \
	virtual V m_signal(I n,F *const *in,F *const *out) { (this->*sigfun)(n,in,out); }
#else
	// static function -> another redirection...
	#define DEFSIGFUN(CL) \
	V (*sigfun)(CL *obj,I n,F *const *in,F *const *out);  \
	virtual V m_signal(I n,F *const *in,F *const *out)   \
		{ (*sigfun)(this,n,in,out); }  \
	TMPLDEF static V st_signal(CL *obj,I n,F *const *in,F *const *out)  \
		{ obj->signal TMPLCALL (n,in,out); }
#endif



#ifndef MIN
#define MIN(x,y) ((x) < (y)?(x):(y))
#endif

// in the signal functions
#ifdef TMPLOPT
	// optimization by using constants for channel numbers
	#define SIGCHNS(BCHNS,bchns,IOCHNS,iochns)  \
		const I BCHNS = _BCHNS_ < 0?(bchns):_BCHNS_;  \
		const I IOCHNS = _IOCHNS_ < 0?MIN(iochns,BCHNS):MIN(_IOCHNS_,BCHNS)
#else 
	// no template optimization
	#ifdef PD // only mono buffers
		#define SIGCHNS(BCHNS,bchns,IOCHNS,iochns)   \
			const I BCHNS = 1;  \
			const I IOCHNS = MIN(iochns,BCHNS)
	#else // MAXMSP
		#define SIGCHNS(BCHNS,bchns,IOCHNS,iochns)   \
			const I BCHNS = bchns;  \
			const I IOCHNS = MIN(iochns,BCHNS)
	#endif
#endif


#endif


