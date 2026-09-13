/* 4VAMAIN.C 
 * By Matt Welsh
 * (c)1991 Matt Welsh
 *
 * 4va is a fourth dimensional object tumbling program for X-Windows. 
 * It does a lot of stuff. See the file 4VA.DOC for details. 
 * PLEASE MAIL WELSH@ODIN.NCSSM.EDU IF YOU FIND ANY BUGS OR HAVE SUGGESTIONS
 * FOR SPEED-UPS, ETC!!!
 * **************************************************************************
 * You are free to distribute this and all code and documentation for
 * the package '4VA' in any form, provided you:
 * 1. Don't sell it.
 * 2. Keep the copyright notice intact on all modules, compiled versions,
 *    and documentation.
 * 3. Give the original author(s) credit if you use this code in your
 *    own work or base another program on it. 
 * ***************************************************************************
 * Matt Welsh-- mdw1@crux2.cit.cornell.edu
 * v0.1 mdw 6 apr 91
 * v1.21 mdw 6 Aug 92
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include "4vahead.h"

/* These are things that were kept in 4VCMD before, but needed */
object *coptr;
transfParams emptyparams;
int MAXX, MAXY, CENX, CENY, SIZY;
char filename[512];
long unsigned FRC, BKC;
char FRCname[512], BKCname[512];
int perspon, LTHK, CLRWIN, ROTCLRD, RESCALE, TITLEBAR, FPS, NODAEMON;
float w_dist, z_dist, rxy, rxz, ryz, rxw, ryw, rzw, SCL;
char displayname[512];

void phelp() {
   printf("  usage: 4va [datafile] [options]\n");
   printf("  list of options:\n");
   printf("  -xy -xz -yz -xw -yw -zw (angle after each, like -xw4.5) \n");  
   printf("    Will rotate object through specified plane each cycle.\n");
   printf("  -np              no perspective                         \n");
   printf("  -ns              don't rescale if window is resized     \n");
   printf("  -nt              no title bar                           \n");
   printf("  -cw              clear window, don't draw over lines    \n");
   printf("  -nd              no daemon, run in the foreground       \n");
   printf("  -zd(distance)    specify z and w distance for           \n");
   printf("  -wd(distance)    perspective. Default is 430.0.         \n");
   printf("  -lc (colorname)  set line color: like -lc LightGreen    \n");
   printf("  -bc (colorname)  set background color                   \n");
   printf("  -lw(width)       set line width, default=0 (fastest)    \n");
   printf("  -d (display)     set display name: like -d lsd:0        \n");
   printf("  -s(scale)        scaling factor. Default is 200.0.      \n");
   printf("  -fps(rate)       frame rate: -1 unlocked, 0 display     \n");
   printf("                   refresh (default), n frames/second    \n");
   printf("  -h or -?         get this help                          \n");
   printf("\n");
}

void optbarf(char *o)
/* Barfs if bad command is given. */
{
   printf("\n");
   printf("4va: bad option %s\n",o);
   phelp();
   exit(1);
}

void setupdefaults() {
/* Initialize the coptr, and set up global defaults for things. */

  makeemptyparams();
  coptr->params = emptyparams;
  coptr->numpoints=1; coptr->numlines=0;
  LTHK=0;
  SCL=200.0;
  strcpy(FRCname,"Red");
  strcpy(BKCname,"Black");
  perspon=1; w_dist=430; z_dist=430;
  rxy=0.0; rxz=0.6; ryz=0.0;
  rxw=0.6; ryw=0.45; rzw=0.0;
  ROTCLRD=0; 
  CLRWIN=0;
  RESCALE=1;
  TITLEBAR=1;
  FPS=0;
  NODAEMON=0;
  strcpy(displayname,"unix:0");
  if (getenv("DISPLAY")) strcpy(displayname,getenv("DISPLAY"));
}

void fixrot() {
/* Make all angles in radians, not degrees. */
  rxy = deg2rad(rxy); 
  rxz = deg2rad(rxz);
  ryz = deg2rad(ryz);
  rxw = deg2rad(rxw);
  ryw = deg2rad(ryw);
  rzw = deg2rad(rzw);
} 

void clearrot() {
/* If one angle is specified on the command line, then clear them all out. */
  if (!ROTCLRD) {
    rxy=0; rxz=0; ryz=0; rxw=0; ryw=0; rzw=0;
    ROTCLRD=1;
  } 
}

void handleclo(int argc, char **argv)
{
/* Look for command line options. */
  int i,j;
  int ook; /* Option OK- flag to tell end of loop that option was accepted, and not to barf. */
  char opt[32], opt2[32];

  if (argc < 2) {
    phelp();
    exit(1);
  }

  for (i=1; i<argc; i++) {
   
   strcpy(opt,argv[i]);
   if (opt[0] != '-') {
     strcpy(filename,opt);
   } else {
     
     ook=0;
     strcpy(opt2,opt);
    
     if (!strncmp(opt2,"-xy",3)) { 
       clearrot();
       sscanf(opt2,"-xy%f",&rxy);
       ook=1;
     }
     if (!strncmp(opt2,"-xz",3)) { 
       clearrot();
       sscanf(opt2,"-xz%f",&rxz);
       ook=1;
     }
     if (!strncmp(opt2,"-yz",3)) { 
       clearrot();
       sscanf(opt2,"-yz%f",&ryz);
       ook=1;
     }
     if (!strncmp(opt2,"-xw",3)) { 
       clearrot();
       sscanf(opt2,"-xw%f",&rxw); 
       ook=1;
     }
     if (!strncmp(opt2,"-yw",3)) { 
       clearrot();
       sscanf(opt2,"-yw%f",&ryw);
       ook=1;
     }
     if (!strncmp(opt2,"-zw",3)) { 
      clearrot();
      sscanf(opt2,"-zw%f",&rzw);
      ook=1;
     }

     if (!strncmp(opt2,"-lc",3)) { 
       strcpy(FRCname,argv[i+1]);
       i++;
       ook=1;
     }
     if (!strncmp(opt2,"-bc",3)) { 
       strcpy(BKCname,argv[i+1]);
       i++;
       ook=1;
     }
     if (!strncmp(opt2,"-lw",3)) { 
       sscanf(opt2,"-lw%d",&LTHK);
       ook=1;
     }
     if (!strncmp(opt2,"-zd",3)) { 
       sscanf(opt2,"-zd%f",&z_dist);
       ook=1;
     }
     if (!strncmp(opt2,"-wd",3)) { 
       sscanf(opt2,"-wd%f",&w_dist);
       ook=1;
     }
     if (!strncmp(opt2,"-np",3)) { 
       perspon=0;
       ook=1;
     }
     if (!strncmp(opt2,"-ns",3)) {
       RESCALE=0;
       ook=1;
     }
     if (!strncmp(opt2,"-nt",3)) {
       TITLEBAR=0;
       ook=1;
     }
     if (!strncmp(opt2,"-nd",3)) {
       NODAEMON=1;
       ook=1;
     }
     if (!strncmp(opt2,"-cw",3)) {
       CLRWIN=1;
       ook=1;
     }
     if (!strncmp(opt2,"-fps",4)) {
       if (sscanf(opt2,"-fps%d",&FPS) != 1 || FPS < -1) optbarf(opt2);
       ook=1;
     }
     if (!strncmp(opt2,"-d",2)) {
       strcpy(displayname,argv[i+1]);
       i++;
       ook=1;
     }
     if (!strncmp(opt2,"-s",2))  { 
       sscanf(opt2,"-s%f",&SCL); 
       ook=1;
     }
     if (!strncmp(opt2,"-h",2))  {ook=1; phelp(); exit(1); break; }
     if (!strncmp(opt2,"-?",2))  {ook=1; phelp(); exit(1); break; }
    
     if (!ook) { 
       optbarf(opt2);
     }
   }
  } 
}
     

static long long now_ns(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

int main(int argc, char **argv)
{
  int i,j,n,done=0;
  char p,c;
  int mypid;
  long long period_ns=0, next_ns, cur_ns;
  struct timespec ts;
  double hz;
   
  printf("\n4va v%s, by Matt Welsh\n",VER_STRING);

  if ((coptr=(object *)malloc(sizeof(object)))==NULL) {
    perror("malloc");
    exit(-1);
  }
 
  /* Set up global stuff and init the object */
  setupdefaults(); 
  /* Get command line options */
  handleclo(argc,argv);
  /* Make all angles in radians */
  fixrot();
  /* Set the scaling factor for the object */
  coptr->params.sclx=coptr->params.scly=coptr->params.sclz=coptr->params.sclw=SCL;

  /* Get the data file. */ 
  loaddfile(filename);
  /* Start up the display */
  g_startup();

  if (FPS == 0) {
    hz = g_refreshrate();
    period_ns = (long long)(1e9 / hz);
    printf(" Frame rate %.2f fps (display refresh).\n", hz);
  } else if (FPS > 0) {
    period_ns = 1000000000LL / FPS;
    printf(" Frame rate %d fps.\n", FPS);
  } else {
    printf(" Frame rate unlocked.\n");
  }

  /* Fork myself off... */
  if (NODAEMON) {
    mypid=0;
  } else {
    printf(" Forking...");
    mypid=fork();
  }
  switch(mypid) {
    case -1: 
      fprintf(stderr,"4va: error creating child process.\n");
      exit(-1);
      break;
    case 0:
      /* child does his little ol' thing... */
        next_ns = now_ns();
        while (done==0) {
           /* Transform object and buffer the lines */
           project(coptr);
           /* Check any "events" that your graphics system may use. For instance,
            * if the window size has changed. */
           g_checkevents();
           /* Clear window */
           g_cleardisplay();
           /* Put the lines to the window */
           g_putlines();
           if (period_ns) {
             next_ns += period_ns;
             cur_ns = now_ns();
             /* If a frame ran long, resync rather than bursting to catch up. */
             if (next_ns < cur_ns) {
               next_ns = cur_ns;
             } else {
               ts.tv_sec = next_ns / 1000000000LL;
               ts.tv_nsec = next_ns % 1000000000LL;
               clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
             }
           }
           /* Rotate the object */
           coptr->params.rxy += rxy;
           coptr->params.rxz += rxz;
           coptr->params.ryz += ryz;
           coptr->params.rxw += rxw;
           coptr->params.ryw += ryw;
           coptr->params.rzw += rzw;
           /* Set the sin and cos components of the angles right */
           fixangles(&coptr->params);
       }
       g_shutdown();
       break;
    default:
       /* And the parent can die. 
        * The address space used by the parent's copy of the object will
	* be freed-- if this is every a problem, use vfork() so that
	* two copies aren't present in the arena at once. 
	*/
       printf("My pid is %d.\n",mypid);
       break;
  }
  printf("Thanks for enjoying 4va!\n");
  return 0;
}
