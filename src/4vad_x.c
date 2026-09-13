/*
 * 4VAD_X.C
 * By Matt Welsh
 *
 * This is the 4VA module to bind with X.
 *
 * (c) 1991 Matt Welsh
 * **************************************************************************
 * You are free to distribute this and all code and documentation for
 * the package '4VA' in any form, provided you:
 * 1. Don't sell it.
 * 2. Keep the copyright notice intact on all modules, compiled versions,
 *    and documentation.
 * 3. Give the original author(s) credit if you use this code in your
 *    own work or base another program on it.
 * ***************************************************************************
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif
#ifdef HAVE_XRENDER
#include <X11/extensions/Xrender.h>
#endif
#ifdef HAVE_XPRESENT
#include <X11/extensions/Xpresent.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "4vahead.h"

Drawable mydb;
GC mygc;
Window mywin,parent;
XSizeHints myhints;
Display *mydisplay;
XWindowAttributes mywattrs;
XSegment  *myseg, *myeraseseg;
int nseg, neraseseg;
Pixmap mypix;
Colormap mycolmap;

/* Back buffers; -rpresent cycles through several because the server holds a presented pixmap until it goes idle. */
#define NBUF 3
typedef struct {
  Pixmap pix;
  XID pict;
  int busy;
} buf_t;
buf_t bufs[NBUF];

#ifdef HAVE_XPRESENT
int presentopcode, presentwait;
unsigned presentoptions;
uint32_t presentserial, completeserial;
uint64_t presentinterval, lastmsc;
#endif

static void handleevent(XEvent *ev);
static void sizebuffers(void);
static void settitle(void);

/* -control: Up/Down cycle through these; key presses are queued and applied between frames. */
static const char *colors[] = {"Red", "Orange", "Yellow", "LightGreen", "Cyan",
                               "DeepSkyBlue", "Violet", "HotPink", "White"};
#define NCOLORS ((int)(sizeof colors / sizeof colors[0]))
static int colorindex=-1, objsteps, colorsteps, quitrequested;

/* Sub-pixel copies of the clipped lines, used for anti-aliasing. */
typedef struct {
  float x1, y1, x2, y2;
} fseg_t;
fseg_t *myfseg;

#ifdef HAVE_XRENDER
Picture mypict, mypen;
XRenderPictFormat *myfmt, *mymaskfmt;
XTriangle *mytris;
#endif

/* Lines are clipped to the window plus this margin before going to X. */
#define CLIPMARGIN 1024

/* code */

#ifdef HAVE_XRENDER
static void makepen(void) {
  XColor c;
  XRenderColor rc;

  if (mypen) XRenderFreePicture(mydisplay,mypen);
  c.pixel=FRC;
  XQueryColor(mydisplay,mycolmap,&c);
  rc.red=c.red; rc.green=c.green; rc.blue=c.blue; rc.alpha=0xffff;
  mypen=XRenderCreateSolidFill(mydisplay,&rc);
}
#endif

static void setupaa(void) {
#ifdef HAVE_XRENDER
  int evbase, errbase;

  if (!XRenderQueryExtension(mydisplay,&evbase,&errbase) ||
      !(myfmt=XRenderFindVisualFormat(mydisplay,DefaultVisual(mydisplay,DefaultScreen(mydisplay))))) {
    fprintf(stderr,"4VA: XRender not available, -aa disabled.\n");
    AA=0;
    return;
  }
  mymaskfmt=XRenderFindStandardFormat(mydisplay,PictStandardA8);
  makepen();
#else
  fprintf(stderr,"4VA: built without XRender, -aa disabled.\n");
  AA=0;
#endif
}

#ifdef HAVE_XRENDER
static void setpoint(XPointFixed *p, double x, double y) {
  p->x=XDoubleToFixed(x);
  p->y=XDoubleToFixed(y);
}

static void drawaa(void) {
  /* Each line becomes a square-capped quad split into two triangles. */
  int i, n=0;
  double hw=(LWIDTH > 0 ? LWIDTH : 1.0)/2;
  double ax, ay, bx, by, dx, dy, len, ux, uy, nx, ny;

  for (i=0; i<nseg; i++) {
    /* +0.5 puts integer coordinates on pixel centers, matching core X lines. */
    ax=myfseg[i].x1+0.5; ay=myfseg[i].y1+0.5;
    bx=myfseg[i].x2+0.5; by=myfseg[i].y2+0.5;
    dx=bx-ax; dy=by-ay;
    len=sqrt(dx*dx+dy*dy);
    if (len < 1e-6) { ux=1; uy=0; } else { ux=dx/len; uy=dy/len; }
    ax-=ux*hw; ay-=uy*hw; bx+=ux*hw; by+=uy*hw;
    nx=-uy*hw; ny=ux*hw;
    setpoint(&mytris[n].p1,ax+nx,ay+ny);
    setpoint(&mytris[n].p2,bx+nx,by+ny);
    setpoint(&mytris[n].p3,bx-nx,by-ny);
    n++;
    setpoint(&mytris[n].p1,ax+nx,ay+ny);
    setpoint(&mytris[n].p2,bx-nx,by-ny);
    setpoint(&mytris[n].p3,ax-nx,ay-ny);
    n++;
  }
  if (n)
    XRenderCompositeTriangles(mydisplay,PictOpOver,mypen,mypict,mymaskfmt,0,0,mytris,n);
}
#endif

static void fillbackground(void) {
  XSetForeground(mydisplay,mygc,BKC);
  XFillRectangle(mydisplay,mypix,mygc,0,0,MAXX,MAXY);
  XSetForeground(mydisplay,mygc,FRC);
}

static void usebuffer(int i) {
  mypix=mydb=bufs[i].pix;
#ifdef HAVE_XRENDER
  mypict=bufs[i].pict;
#endif
}

static void makebuffer(void) {
  int i, n=(RENDER == RENDER_PRESENT) ? NBUF : 1;

  for (i=0; i<n; i++) {
#ifdef HAVE_XRENDER
    if (bufs[i].pict) XRenderFreePicture(mydisplay,bufs[i].pict);
    bufs[i].pict=0;
#endif
    if (bufs[i].pix) XFreePixmap(mydisplay,bufs[i].pix);
    bufs[i].pix=XCreatePixmap(mydisplay,mywin,MAXX,MAXY,
                              DefaultDepth(mydisplay,DefaultScreen(mydisplay)));
    bufs[i].busy=0;
#ifdef HAVE_XRENDER
    if (AA) bufs[i].pict=XRenderCreatePicture(mydisplay,bufs[i].pix,myfmt,0,NULL);
#endif
  }
  usebuffer(0);
  fillbackground();
}

#ifdef HAVE_XPRESENT
static void setuppresent(void) {
  int evbase, errbase, major, minor;

  if (!XPresentQueryExtension(mydisplay,&presentopcode,&evbase,&errbase) ||
      !XPresentQueryVersion(mydisplay,&major,&minor)) {
    fprintf(stderr,"4VA: Present not available, using -rbuffer.\n");
    RENDER=RENDER_BUFFER;
    return;
  }
  XPresentSelectInput(mydisplay,mywin,PresentCompleteNotifyMask|PresentIdleNotifyMask);
}

static void pickbuffer(void) {
  /* Draw into a pixmap the server has released, waiting for one if needed. */
  XEvent ev;
  int i;

  for (;;) {
    for (i=0; i<NBUF; i++)
      if (!bufs[i].busy) { usebuffer(i); return; }
    XNextEvent(mydisplay,&ev);
    handleevent(&ev);
  }
}

static void presentframe(void) {
  XEvent ev;
  int i;
  uint64_t target=(presentinterval > 1 && lastmsc) ? lastmsc+presentinterval : 0;

  XPresentPixmap(mydisplay,mywin,mypix,++presentserial,None,None,0,0,None,None,None,
                 presentoptions,target,0,0,NULL,0);
  for (i=0; i<NBUF; i++)
    if (bufs[i].pix == mypix) bufs[i].busy=1;
  while (presentwait && completeserial != presentserial) {
    XNextEvent(mydisplay,&ev);
    handleevent(&ev);
  }
}
#else
static void setuppresent(void) {
  fprintf(stderr,"4VA: built without Present, using -rbuffer.\n");
  RENDER=RENDER_BUFFER;
}
#endif

void g_cleardisplay(void) {
  if (RENDER != RENDER_DIRECT) {
#ifdef HAVE_XPRESENT
    if (RENDER == RENDER_PRESENT) pickbuffer();
#endif
    fillbackground();
  } else if (CLRWIN) {
     XClearWindow(mydisplay,mywin);
  } else {
    XSetForeground(mydisplay,mygc,BKC);
    XDrawSegments(mydisplay,mydb,mygc,myeraseseg,neraseseg);
    XSetForeground(mydisplay,mygc,FRC);
  }
}

static int clipline(double *x1, double *y1, double *x2, double *y2,
                    double xmin, double ymin, double xmax, double ymax) {
  /* Liang-Barsky; returns 0 if the line is entirely outside. */
  double dx = *x2 - *x1, dy = *y2 - *y1, t0 = 0, t1 = 1, t;
  double p[4], q[4];
  int k;

  if (!isfinite(*x1) || !isfinite(*y1) || !isfinite(*x2) || !isfinite(*y2))
    return 0;
  p[0] = -dx; q[0] = *x1 - xmin;
  p[1] =  dx; q[1] = xmax - *x1;
  p[2] = -dy; q[2] = *y1 - ymin;
  p[3] =  dy; q[3] = ymax - *y1;
  for (k = 0; k < 4; k++) {
    if (p[k] == 0) {
      if (q[k] < 0) return 0;
    } else {
      t = q[k] / p[k];
      if (p[k] < 0) {
        if (t > t1) return 0;
        if (t > t0) t0 = t;
      } else {
        if (t < t0) return 0;
        if (t < t1) t1 = t;
      }
    }
  }
  *x2 = *x1 + t1 * dx; *y2 = *y1 + t1 * dy;
  *x1 = *x1 + t0 * dx; *y1 = *y1 + t0 * dy;
  return 1;
}

void g_bufferline(float x1, float x2, float y1, float y2) {
  double ax = x1, ay = y1, bx = x2, by = y2;
  double m = CLIPMARGIN + LTHK;

  if (nseg >= coptr->numlines) return;
  if (!clipline(&ax, &ay, &bx, &by, -m, -m, MAXX + m, MAXY + m)) return;
  if (AA) {
    myfseg[nseg].x1 = ax; myfseg[nseg].y1 = ay;
    myfseg[nseg].x2 = bx; myfseg[nseg].y2 = by;
  }
  myseg[nseg].x1 = lrint(ax); myseg[nseg].y1 = lrint(ay);
  myseg[nseg].x2 = lrint(bx); myseg[nseg].y2 = lrint(by);
  nseg++;
}

void g_putlines(void) {
   int i;
#ifdef HAVE_XRENDER
   if (AA) drawaa();
   else
#endif
   XDrawSegments(mydisplay,mydb,mygc,myseg,nseg);
   if (RENDER == RENDER_BUFFER) {
     XCopyArea(mydisplay,mypix,mywin,mygc,0,0,MAXX,MAXY,0,0);
#ifdef HAVE_XPRESENT
   } else if (RENDER == RENDER_PRESENT) {
     presentframe();
#endif
   } else if (!CLRWIN) {
     for (i=0; i<nseg; i++) {
       myeraseseg[i]=myseg[i];
     }
     neraseseg=nseg;
   }
   nseg=0;
   XFlush(mydisplay);
}

void g_fixcoords(void) {

  XGetWindowAttributes(mydisplay,mywin,&mywattrs);
  MAXX=mywattrs.width;
  SIZY=MAXY=mywattrs.height;
  CENX=(int)(MAXX/2);
  CENY=(int)(MAXY/2);
  if (RENDER != RENDER_DIRECT) makebuffer();
}

static void resize(int width, int height) {
  /* Change the scaling factor proportional to how much the window was resized. */
   double mywinsl1,mywinsl2,change;

   mywinsl1=MAXX*MAXY;
   MAXX=width;
   MAXY=SIZY=height;
   CENX=(int)(MAXX/2);
   CENY=(int)(MAXY/2);
   if (RESCALE) {
     mywinsl2=MAXX*MAXY;
     change=(sqrt(mywinsl2)/sqrt(mywinsl1));
     coptr->params.sclx *= change;
     coptr->params.scly *= change;
     coptr->params.sclz *= change;
     coptr->params.sclw *= change;
     z_dist *= change;
     w_dist *= change;
   }
   if (RENDER != RENDER_DIRECT) makebuffer();
}

static void handleevent(XEvent *ev) {
  switch (ev->type) {
    case ConfigureNotify:
      if (ev->xconfigure.width != MAXX || ev->xconfigure.height != MAXY)
        resize(ev->xconfigure.width, ev->xconfigure.height);
      break;
    case KeyPress:
      switch (XLookupKeysym(&ev->xkey,0)) {
        case XK_Right: objsteps++; break;
        case XK_Left: objsteps--; break;
        case XK_Up: colorsteps++; break;
        case XK_Down: colorsteps--; break;
        case XK_q: case XK_Escape: quitrequested=1; break;
      }
      break;
    case Expose:
      /* -rpresent repaints on its next frame. */
      if (RENDER == RENDER_BUFFER)
        XCopyArea(mydisplay,mypix,mywin,mygc,ev->xexpose.x,ev->xexpose.y,
                  ev->xexpose.width,ev->xexpose.height,ev->xexpose.x,ev->xexpose.y);
      break;
#ifdef HAVE_XPRESENT
    case GenericEvent:
      if (ev->xcookie.extension == presentopcode && XGetEventData(mydisplay,&ev->xcookie)) {
        if (ev->xcookie.evtype == PresentCompleteNotify) {
          XPresentCompleteNotifyEvent *ce = ev->xcookie.data;
          completeserial=ce->serial_number;
          lastmsc=ce->msc;
        } else if (ev->xcookie.evtype == PresentIdleNotify) {
          XPresentIdleNotifyEvent *ie = ev->xcookie.data;
          int i;
          for (i=0; i<NBUF; i++)
            if (bufs[i].pix == ie->pixmap) bufs[i].busy=0;
        }
        XFreeEventData(mydisplay,&ev->xcookie);
      }
      break;
#endif
  }
}

void g_checkevents(void) {
  /* Drain events from the X Server: track resizes and repaint exposed areas. */
  XEvent ev;

  while (XPending(mydisplay)) {
    XNextEvent(mydisplay,&ev);
    handleevent(&ev);
  }
}

void g_startup(void) {

  XColor theRGBColor, theHardwareColor;
  XSizeHints myhints;
  int theStatus;

  /* This is the most specific and important part of the X code. Here
     we set up the display, all colours, and the fonts (which will go in
     their own gc for relocation later on). */

  /* Open pipe to display */
  printf(" Opening display.\n");
  if ((mydisplay=XOpenDisplay(displayname)) == NULL) {
    fprintf(stderr,"4VA: Could not open display.\n");
    exit(1);
  }
  parent=XDefaultRootWindow(mydisplay);

  /* Ressurrect up the colormap */
  mycolmap=DefaultColormap(mydisplay,0);
  theStatus=XLookupColor(mydisplay,mycolmap,BKCname,&theRGBColor,&theHardwareColor);
  if (theStatus) {
    theStatus=XAllocColor(mydisplay,mycolmap,&theHardwareColor);
    if (theStatus) BKC = theHardwareColor.pixel;
    else BKC=1;
  }
  theStatus=XLookupColor(mydisplay,mycolmap,FRCname,&theRGBColor,&theHardwareColor);
  if (theStatus) {
    theStatus=XAllocColor(mydisplay,mycolmap,&theHardwareColor);
    if (theStatus) FRC = theHardwareColor.pixel;
    else FRC=6;
  }

  /* Set up the drawable and GC */
  mygc=DefaultGC(mydisplay,0);
  XSetLineAttributes(mydisplay,mygc,LTHK,LineSolid,CapButt,JoinMiter);
  /* Copying from our own pixmap never needs GraphicsExpose/NoExpose events. */
  XSetGraphicsExposures(mydisplay,mygc,False);
  mywin=XCreateSimpleWindow(mydisplay,parent,0,0,650,650,2,0,BKC);
  /* The buffer covers the whole window, so don't let the server clear it first. */
  if (RENDER != RENDER_DIRECT) XSetWindowBackgroundPixmap(mydisplay,mywin,None);
  myhints.flags = USPosition|PSize;
  myhints.x=myhints.y=0;
  myhints.width=650; myhints.height=650;
  XSetNormalHints(mydisplay,mywin,&myhints);

  mydb=mywin; /*drawable for lines same as window*/
  /* Set the name of the window to the object name, then map the window. */
  settitle();
  XSelectInput(mydisplay, mywin, StructureNotifyMask|ExposureMask|(CONTROL ? KeyPressMask : 0));
  printf(" Mapping window.\n");
  XMapRaised(mydisplay,mywin);
  XSync(mydisplay,0);
  XSetForeground(mydisplay,mygc,FRC);
  XSetBackground(mydisplay,mygc,BKC);
  if (RENDER == RENDER_DIRECT) XClearWindow(mydisplay,mywin);
  if (RENDER == RENDER_PRESENT) setuppresent();
  if (AA) setupaa();
  /* Go ahead and allocate the segment structs for the buffered lines. */
  sizebuffers();
  for (colorindex=NCOLORS-1; colorindex>=0 && strcasecmp(FRCname,colors[colorindex]); colorindex--)
    ;

  /* Fix the window size global variables; later resizes arrive as ConfigureNotify. */
  g_fixcoords();
}

void g_shutdown(void) {
  XCloseDisplay(mydisplay);
}

static void sizebuffers(void) {
  /* Line buffers follow the current object's line count. */
  size_t n=coptr->numlines ? coptr->numlines : 1;

  if (!(myseg=(XSegment *)realloc(myseg,n*sizeof(XSegment))) ||
      !(myeraseseg=(XSegment *)realloc(myeraseseg,n*sizeof(XSegment))))
    goto nomem;
  if (AA) {
    if (!(myfseg=(fseg_t *)realloc(myfseg,n*sizeof(fseg_t)))) goto nomem;
#ifdef HAVE_XRENDER
    if (!(mytris=(XTriangle *)realloc(mytris,2*n*sizeof(XTriangle)))) goto nomem;
#endif
  }
  return;
nomem:
  fprintf(stderr,"4VA: could not allocate line buffers.\n");
  exit(-1);
}

static void settitle(void) {
  char name[600];
  const char *base;

  if (!TITLEBAR) return;
  if (CONTROL) {
    base=strrchr(objfiles[curobj],'/');
    base=base ? base+1 : objfiles[curobj];
    snprintf(name,sizeof name,"4va v%s - %s [%d/%d]",VER_STRING,base,curobj+1,nobjfiles);
  } else {
    snprintf(name,sizeof name,"4va v%s",VER_STRING);
  }
  XStoreName(mydisplay,mywin,name);
}

static void setcolor(const char *cname) {
  XColor exact, hw;

  if (!XLookupColor(mydisplay,mycolmap,cname,&exact,&hw) || !XAllocColor(mydisplay,mycolmap,&hw))
    return;
  FRC=hw.pixel;
  snprintf(FRCname,sizeof FRCname,"%s",cname);
  XSetForeground(mydisplay,mygc,FRC);
#ifdef HAVE_XRENDER
  if (AA) makepen();
#endif
}

void g_objectchanged(void) {
  sizebuffers();
  nseg=neraseseg=0;
  if (RENDER == RENDER_DIRECT) XClearWindow(mydisplay,mywin);
  settitle();
}

int g_controls(int *objstep) {
  /* Apply queued -control keys between frames; returns 1 if quit was requested. */
  if (colorsteps) {
    if (colorindex < 0) colorindex=colorsteps > 0 ? -1 : NCOLORS;
    colorindex=((colorindex+colorsteps) % NCOLORS + NCOLORS) % NCOLORS;
    setcolor(colors[colorindex]);
    colorsteps=0;
  }
  *objstep=objsteps;
  objsteps=0;
  return quitrequested;
}

int g_vsync(int fps, double hz) {
  /* Configure Present pacing; returns 1 if each frame waits for the display. */
#ifdef HAVE_XPRESENT
  long k;

  if (RENDER != RENDER_PRESENT) return 0;
  presentoptions=PresentOptionNone;
  presentinterval=0;
  presentwait=0;
  if (fps < 0) {
    presentoptions=PresentOptionAsync;
    return 0;
  }
  /* Rates that divide the refresh rate map to whole refresh intervals; others use the timer. */
  k = fps ? lrint(hz / fps) : 1;
  if (k >= 1 && (fps == 0 || fabs(hz / k - fps) <= 0.02 * fps)) {
    presentinterval=k;
    presentwait=1;
    return 1;
  }
#endif
  return 0;
}

double g_refreshrate(void) {
  /* Refresh rate of the monitor the window is on, falling back to 60Hz. */
  double hz = 0;
#ifdef HAVE_XRANDR
  XRRScreenResources *res;
  XRRCrtcInfo *crtc;
  XRRModeInfo *m;
  Window child;
  int i, j, wx, wy, evbase, errbase, inside;
  double rate;

  if (XRRQueryExtension(mydisplay, &evbase, &errbase) &&
      (res = XRRGetScreenResourcesCurrent(mydisplay, parent))) {
    XTranslateCoordinates(mydisplay, mywin, parent, MAXX/2, MAXY/2, &wx, &wy, &child);
    for (i = 0; i < res->ncrtc; i++) {
      if (!(crtc = XRRGetCrtcInfo(mydisplay, res, res->crtcs[i]))) continue;
      for (j = 0; j < res->nmode; j++) {
        m = &res->modes[j];
        if (m->id != crtc->mode || !m->hTotal || !m->vTotal) continue;
        rate = (double)m->dotClock / ((double)m->hTotal * m->vTotal);
        if (m->modeFlags & RR_DoubleScan) rate /= 2;
        if (m->modeFlags & RR_Interlace) rate *= 2;
        inside = wx >= crtc->x && wx < crtc->x + (int)crtc->width &&
                 wy >= crtc->y && wy < crtc->y + (int)crtc->height;
        if (hz == 0 || inside) hz = rate;
      }
      XRRFreeCrtcInfo(crtc);
    }
    XRRFreeScreenResources(res);
  }
#endif
  return hz > 0 ? hz : 60.0;
}

