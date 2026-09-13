/*   
 * 4VAHEAD.H
 * This is the header file for 4ViewAuto, basically the same but with stuff
 * cut out like the interface.
 *
 * (c)1991 Matt Welsh
 *
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

#define VER_STRING "1.21"

/* typedefs */

typedef struct {
  float rxy, sxy, cxy; 
  float rxz, sxz, cxz; 
  float ryz, syz, cyz;
  float rxw, sxw, cxw;
  float ryw, syw, cyw;
  float rzw, szw, czw;

  float sclx, scly, sclz, sclw;  
  float trnx, trny, trnz, trnw;  
} transfParams;


typedef struct {     
  float x, y, z, w;
} point_t;

typedef struct {  
  int from, to;
} line_t;

typedef struct {       
  transfParams params;        
  point_t *pts;        
  point_t *transpts;   
  line_t  *lns;        
  int numpoints;            
  int numlines;
  char name[32];            
} object;

/* extern storage dec's */
extern transfParams emptyparams;
extern object *coptr;           /*pointer to current object*/

/* Global parameters to the system. */
extern int perspon;
extern float w_dist;
extern float z_dist;
extern int MAXX, MAXY;          
extern int CENX, CENY;
extern int SIZY;
extern long unsigned FRC, BKC;
extern char FRCname[512], BKCname[512];
extern int LTHK, CLRWIN, RESCALE, TITLEBAR;
extern char displayname[512];

/* 4VaPROJ Prototypes ... projection */
extern void fixangles(transfParams *params);
extern float deg2rad(float ang);
extern void makeemptyparams(void);
extern void project(object *obj);

/* 4VaRTN Prototypes ... object routines */
extern int loaddfile(const char *filename);


/* 4VaD_ Prototypes ... display */
extern void g_cleardisplay(void);
extern void g_bufferline(int x1, int x2, int y1, int y2, int i);
extern void g_putlines(void);
extern void g_fixcoords(void);
extern void g_checkevents(void);
extern void g_startup(void);
extern void g_shutdown(void);
