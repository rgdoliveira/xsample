/* 

xsample - extended sample objects for Max/MSP and pd (pure data)

Copyright (c) 2001,2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"

#ifdef _MSC_VER
#pragma warning (disable:4244)
#endif


//#define DEBUG 


class xplay:
	public xsample
{
	FLEXT_HEADER(xplay,xsample)

public:
	xplay(I argc, t_atom *argv);
	~xplay();
	
#ifdef MAXMSP
	virtual V m_loadbang() { m_refresh(); }
	virtual V m_assist(L msg,L arg,C *s);
#endif

	virtual V m_help();
	virtual V m_print();
	
	virtual I m_set(I argc,t_atom *argv);

	virtual V m_start();
	virtual V m_stop();

protected:
	BL doplay;
	I outchns;

#ifdef TMPLOPT
	template <int _BCHNS_,int _OCHNS_>
#endif
	V signal(I n,F *const *in,F *const *out);  // this is the dsp method

private:
	virtual V m_dsp(I n,F *const *in,F *const *out);
	virtual V m_signal(I n,F *const *in,F *const *out) { (this->*sigfun)(n,in,out); }

	V (xplay::*sigfun)(I n,F *const *in,F *const *out);  // this is my dsp method

	static V cb_start(t_class *c) { thisObject(c)->m_start(); }
	static V cb_stop(t_class *c) { thisObject(c)->m_stop(); }
	static V cb_reset(t_class *c) { thisObject(c)->m_reset(); }
};

FLEXT_NEW_WITH_GIMME("xplay~",xplay)


V xplay::cb_setup(t_class *c)
{
	add_bang(c,cb_start);
	add_method0(c,cb_start,"start");
	add_method0(c,cb_stop,"stop");
}


xplay::xplay(I argc, t_atom *argv): 
	doplay(false)
{
#ifdef DEBUG
	if(argc < 1) {
		post(OBJNAME " - Warning: no buffer defined");
	} 
#endif
	
#ifdef MAXMSP
	outchns = argc >= 2?atom_getflintarg(1,argc,argv):1;
#else
	outchns = 1;
#endif

	Inlet_signal();  // pos signal
	Outlet_signal(outchns);
	SetupInOut();

	buf = new buffer(argc >= 1?atom_getsymbolarg(0,argc,argv):NULL);	
	m_reset();
}

xplay::~xplay()
{
	if(buf) delete buf;
}

I xplay::m_set(I argc,t_atom *argv) 
{
	I r = xsample::m_set(argc,argv);
	if(r != 0) m_units();
	return r;
}

V xplay::m_start() 
{ 
	m_refresh(); 
	doplay = true; 
}

V xplay::m_stop() 
{ 
	doplay = false; 
}


#ifdef TMPLOPT
template <int _BCHNS_,int _OCHNS_>
#endif
V xplay::signal(I n,F *const *invecs,F *const *outvecs)
{
#ifdef TMPLOPT
	const I BCHNS = _BCHNS_ == 0?buf->Channels():_BCHNS_;
	const I OCHNS = _OCHNS_ == 0?MIN(outchns,BCHNS):MIN(_OCHNS_,BCHNS);
#else
	const I BCHNS = buf->Channels();
	const I OCHNS = MIN(outchns,BCHNS);
#endif

	const F *pos = invecs[0];
	F *const *sig = outvecs;
	register I si = 0;

	if(doplay && buf->Frames() > 0) {
		if(interp == xsi_4p && buf->Frames() > 4) {
			I maxo = buf->Frames()-3;

			for(I i = 0; i < n; ++i,++si) {	
				F o = *(pos++)/s2u;
				I oint = (I)o;
				register F a,b,c,d;

				for(I ci = 0; ci < OCHNS; ++ci) {
					register const F *fp = buf->Data()+oint*BCHNS+ci;
					F frac = o-oint;

					if (oint < 1) {
						if(oint < 0) {
							frac = 0;
							a = b = c = d = buf->Data()[0*BCHNS+ci];
						}
						else {
							a = b = fp[0*BCHNS];
							c = fp[1*BCHNS];
							d = fp[2*BCHNS];
						}
					}
					else if(oint > maxo) {
						if(oint == maxo+1) {
							a = fp[-1*BCHNS];	
							b = fp[0*BCHNS];	
							c = d = fp[1*BCHNS];	
						}
						else {
							frac = 0; 
							a = b = c = d = buf->Data()[(buf->Frames()-1)*BCHNS+ci]; 
						}
					}
					else {
						a = fp[-1*BCHNS];
						b = fp[0*BCHNS];
						c = fp[1*BCHNS];
						d = fp[2*BCHNS];
					}

					F cmb = c-b;
					sig[ci][si] = b + frac*( 
						cmb - 0.5f*(frac-1.) * ((a-d+3.0f*cmb)*frac + (b-a-cmb))
					);
				}
			}
		}
		else {
			// keine Interpolation
			for(I i = 0; i < n; ++i,++si) {	
				I o = (I)(*(pos++)/s2u);
				if(o < 0) {
					for(I ci = 0; ci < OCHNS; ++ci)
						sig[ci][si] = buf->Data()[0*BCHNS+ci];
				}
				if(o > buf->Frames()) {
					for(I ci = 0; ci < OCHNS; ++ci)
						sig[ci][si] = buf->Data()[(buf->Frames()-1)*BCHNS+ci];
				}
				else {
					for(I ci = 0; ci < OCHNS; ++ci)
						sig[ci][si] = buf->Data()[o*BCHNS+ci];
				}
			}
		}	
	}
	else {
		while(n--) { 
			for(I ci = 0; ci < OCHNS; ++ci)	sig[ci][si] = 0;
			++si;
		}
	}
}

V xplay::m_dsp(I /*n*/,F *const * /*insigs*/,F *const * /*outsigs*/)
{
	// this is hopefully called at change of sample rate ?!

	m_refresh();  

#ifdef TMPLOPT
	switch(buf->Frames()*100+outchns) {
		case 101:
			sigfun = &xplay::signal<1,1>; break;
		case 102:
			sigfun = &xplay::signal<1,2>; break;
		case 201:
			sigfun = &xplay::signal<2,1>; break;
		case 202:
			sigfun = &xplay::signal<2,2>; break;
		case 401:
		case 402:
		case 403:
			sigfun = &xplay::signal<4,0>; break;
		case 404:
			sigfun = &xplay::signal<4,4>; break;
		default:
			sigfun = &xplay::signal<0,0>;
	}
#else
	sigfun = &xplay::signal;
#endif
}



V xplay::m_help()
{
	post("%s - part of xsample objects",thisName());
	post("(C) Thomas Grill, 2001-2002 - version " VERSION " compiled on " __DATE__ " " __TIME__);
#ifdef MAXMSP
	post("Arguments: %s [buffer] [channels=1]",thisName());
#else
	post("Arguments: %s [buffer]",thisName());
#endif
	post("Inlets: 1:Messages/Position signal");
	post("Outlets: 1:Audio signal");	
	post("Methods:");
	post("\thelp: shows this help");
	post("\tset name: set buffer");
	post("\tenable 0/1: turn dsp calculation off/on");	
	post("\tprint: print current settings");
	post("\tbang/start: begin playing");
	post("\tstop: stop playing");
	post("\treset: checks buffer");
	post("\trefresh: checks buffer and refreshes outlets");
	post("\tunits 0/1/2/3: set units to samples/buffer size/ms/s");
	post("\tinterp 0/1: turn interpolation off/on");
	post("");
}

V xplay::m_print()
{
	// print all current settings
	post("%s - current settings:",thisName());
	post("bufname = '%s',buflen = %.3f",buf->Name(),(F)(buf->Frames()*s2u)); 
	post("samples/unit = %.3f,interpolation = %s",(F)(1./s2u),interp?"yes":"no"); 
	post("");
}


#ifdef MAXMSP
V xplay::m_assist(L msg,L arg,C *s)
{
	switch(msg) {
	case 1: //ASSIST_INLET:
		switch(arg) {
		case 0:
			strcpy(s,"Messages and Signal of playing position"); break;
		}
		break;
	case 2: //ASSIST_OUTLET:
		switch(arg) {
		case 0:
			strcpy(s,"Audio signal played"); break;
		}
		break;
	}
}
#endif


#ifdef PD
extern "C" FLEXT_EXT V xplay_tilde_setup()
#elif defined(MAXMSP)
extern "C" V main()
#endif
{
	xplay_setup();
}

