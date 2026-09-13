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
#include <stdio.h>
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
Colormap mycolmap;

/* code */

void g_cleardisplay() {

  int i,from,to,x1,x2,y1,y2;
 
  if (CLRWIN) {  
     XClearWindow(mydisplay,mywin); 
  } else {
    XSetForeground(mydisplay,mygc,BKC);
    XDrawSegments(mydisplay,mydb,mygc,myeraseseg,coptr->numlines); 
    XSetForeground(mydisplay,mygc,FRC);
  }
}


void g_bufferline(x1,x2,y1,y2,i) {  
   
    myseg[i].x1=x1; myseg[i].x2=x2; myseg[i].y1=y1;
    myseg[i].y2=y2;
}
  
void g_putlines() {
   int i;
   if (!CLRWIN) { 
     for (i=0; i<coptr->numlines; i++) {
       myeraseseg[i]=myseg[i];
     }
   }
   XDrawSegments(mydisplay,mydb,mygc,myseg,coptr->numlines);
}

void g_fixcoords() {
  
  XGetWindowAttributes(mydisplay,mywin,&mywattrs);
  MAXX=mywattrs.width;
  SIZY=MAXY=mywattrs.height;
  CENX=(int)(MAXX/2);
  CENY=(int)(MAXY/2);
}

void g_checkevents() {
  /* Check events from the X Server. Actually I'll just update the
   * window size by hand. Also change the scaling factor proportional to
   * how much the window was resized. */
   double mywinsl1,mywinsl2,change; 
  
   mywinsl1=MAXX*MAXY;
   XGetWindowAttributes(mydisplay,mywin,&mywattrs);
   if (MAXX!=mywattrs.width || MAXY!=mywattrs.height) {
     MAXX=mywattrs.width;
     MAXY=SIZY=mywattrs.height;
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
   }
}

void g_startup() {

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
  mywin=XCreateSimpleWindow(mydisplay,parent,0,0,650,650,2,0,BKC);
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
  printf(" Mapping window.\n");
  XMapRaised(mydisplay,mywin); 
  XSync(mydisplay,0); 
  /* Set event mask then clear off the window */
  XSelectInput(mydisplay, mywin, StructureNotifyMask);
  XSetForeground(mydisplay,mygc,FRC);
  XSetBackground(mydisplay,mygc,BKC);
  XClearWindow(mydisplay,mywin);
  /* Go ahead and allocate the segment struct for the buffered lines. */
  if ((myseg= (XSegment *)malloc((coptr->numlines)*sizeof(XSegment)))==NULL) {
    fprintf(stderr,"4VA: could not allocate segment stucture.\n");
    exit(-1); 
  }
  if ((myeraseseg= (XSegment *)malloc((coptr->numlines)*sizeof(XSegment)))==NULL) {
    fprintf(stderr,"4VA: could not allocate erase segment structure.\n");
    exit(-1);
  }
  
  /* Fix the window size global variables. If you want to resize the window in realtime,
   * then you'll need to watch for the event and call this function. */
  g_fixcoords(); 
}

void g_shutdown() {
  XCloseDisplay(mydisplay);
}

