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
#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif
#ifdef HAVE_XRENDER
#include <X11/extensions/Xrender.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static void setupaa(void) {
#ifdef HAVE_XRENDER
  int evbase, errbase;
  XColor c;
  XRenderColor rc;

  if (!XRenderQueryExtension(mydisplay,&evbase,&errbase) ||
      !(myfmt=XRenderFindVisualFormat(mydisplay,DefaultVisual(mydisplay,DefaultScreen(mydisplay))))) {
    fprintf(stderr,"4VA: XRender not available, -aa disabled.\n");
    AA=0;
    return;
  }
  mymaskfmt=XRenderFindStandardFormat(mydisplay,PictStandardA8);
  c.pixel=FRC;
  XQueryColor(mydisplay,mycolmap,&c);
  rc.red=c.red; rc.green=c.green; rc.blue=c.blue; rc.alpha=0xffff;
  mypen=XRenderCreateSolidFill(mydisplay,&rc);
  if ((myfseg=(fseg_t *)malloc((coptr->numlines)*sizeof(fseg_t)))==NULL ||
      (mytris=(XTriangle *)malloc(2*(coptr->numlines)*sizeof(XTriangle)))==NULL) {
    fprintf(stderr,"4VA: could not allocate anti-aliasing structures.\n");
    exit(-1);
  }
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

static void makebuffer(void) {
#ifdef HAVE_XRENDER
  if (mypict) XRenderFreePicture(mydisplay,mypict);
  mypict=0;
#endif
  if (mypix) XFreePixmap(mydisplay,mypix);
  mypix=XCreatePixmap(mydisplay,mywin,MAXX,MAXY,
                      DefaultDepth(mydisplay,DefaultScreen(mydisplay)));
  mydb=mypix;
#ifdef HAVE_XRENDER
  if (AA) mypict=XRenderCreatePicture(mydisplay,mypix,myfmt,0,NULL);
#endif
  fillbackground();
}

void g_cleardisplay(void) {
  if (RENDER == RENDER_BUFFER) {
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
  if (RENDER == RENDER_BUFFER) makebuffer();
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
   if (RENDER == RENDER_BUFFER) makebuffer();
}

void g_checkevents(void) {
  /* Drain events from the X Server: track resizes and repaint exposed areas. */
  XEvent ev;

  while (XPending(mydisplay)) {
    XNextEvent(mydisplay,&ev);
    switch (ev.type) {
      case ConfigureNotify:
        if (ev.xconfigure.width != MAXX || ev.xconfigure.height != MAXY)
          resize(ev.xconfigure.width, ev.xconfigure.height);
        break;
      case Expose:
        if (RENDER == RENDER_BUFFER)
          XCopyArea(mydisplay,mypix,mywin,mygc,ev.xexpose.x,ev.xexpose.y,
                    ev.xexpose.width,ev.xexpose.height,ev.xexpose.x,ev.xexpose.y);
        break;
    }
  }
}

void g_startup(void) {

  char name[255];
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
  if (RENDER == RENDER_BUFFER) XSetWindowBackgroundPixmap(mydisplay,mywin,None);
  myhints.flags = USPosition|PSize;
  myhints.x=myhints.y=0;
  myhints.width=650; myhints.height=650;
  XSetNormalHints(mydisplay,mywin,&myhints);

  mydb=mywin; /*drawable for lines same as window*/
  /* Set the name of the window to the object name, then map the window. */
  if (TITLEBAR) {
    sprintf(name,"4va v%s",VER_STRING);
    XStoreName(mydisplay,mywin,name);
  }
  XSelectInput(mydisplay, mywin, StructureNotifyMask|ExposureMask);
  printf(" Mapping window.\n");
  XMapRaised(mydisplay,mywin);
  XSync(mydisplay,0);
  XSetForeground(mydisplay,mygc,FRC);
  XSetBackground(mydisplay,mygc,BKC);
  if (RENDER == RENDER_DIRECT) XClearWindow(mydisplay,mywin);
  /* Go ahead and allocate the segment struct for the buffered lines. */
  if ((myseg= (XSegment *)malloc((coptr->numlines)*sizeof(XSegment)))==NULL) {
    fprintf(stderr,"4VA: could not allocate segment stucture.\n");
    exit(-1);
  }
  if ((myeraseseg= (XSegment *)malloc((coptr->numlines)*sizeof(XSegment)))==NULL) {
    fprintf(stderr,"4VA: could not allocate erase segment structure.\n");
    exit(-1);
  }

  if (AA) setupaa();

  /* Fix the window size global variables; later resizes arrive as ConfigureNotify. */
  g_fixcoords();
}

void g_shutdown(void) {
  XCloseDisplay(mydisplay);
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

