/*
 * CUTCTORUS.C by Matt Welsh 
 * Creates a standard clifford torus and maps the lines by the same grid
 * technique as 4VDMAKE, but I make the lines wrap around to the other
 * side of the torus as well. Took out some of the lines, too much of a
 * bitch to adapt params.
 * **************************************************************************
 * You are free to distribute this and all code and documentation for
 * the package '4VA' in any form, provided you:
 * 1. Don't sell it.
 * 2. Keep the copyright notice intact on all modules, compiled versions,
 *    and documentation.
 * 3. Give the original author(s) credit if you use this code in your
 *    own work or base another program on it. 
 * ***************************************************************************
 * v2.1 mdw 30 Sep 91 
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main (int argc, char **argv)
{

   double x,y;
   double tempx, tempy;
   double cx, cy, cz, cw;
   int maxx, maxy;
   int i,j;

   if (argc!=3) {
     fprintf(stderr,"CUTCTORUS: need parameters.\n");
	 fprintf(stderr,"  usage: CUTCTORUS <x increments> <y increments>\n");
	 exit(-1);
   }
   maxx=atoi(argv[1]);
   maxy=atoi(argv[2]);

   printf("p={%d}\n",maxx*maxy);

   for (y=0; y<maxy; y++) {
     for (x=0; x<maxx; x++) {
   
      /* squeeze everything into a 0 to 2pi grid. Apply your calculations to tempy and
       * tempx ONLY!!!! We do this by dividing max vals by whatever size we want and shifting. */
      tempx=(x/(maxx/(2*M_PI)));
      tempy=(y/(maxy/(2*M_PI)));

      /* Here is the function. We gotta use new values, because the tempx and tempy are only
       * grid values. All four coordinates are mapped from them. */
       cx=cos(tempx); cy=sin(tempx);   
       cz=cos(tempy); cw=sin(tempy); 
     
      printf("%.5f %.5f %.5f %.5f\n",cx,cy,cz,cw);
     }
   }

   /* Now spit out the grid connections. You don't need to touch
    * this code unless you want to. :) */
   printf("l={%d}\n",(maxx)*(maxy));
   /* do the horizontal lines across x */
   for (i=0; i<(maxx-1); i++) {
     for (j=0; j<maxy; j++) {
	   printf("%d %d\n",i+(j*maxx),i+(j*maxx)+1);
	 }
   }
   /* Connect lines back to other side of torus.  */
   for (j=0; j<maxy; j++) {
     printf("%d %d\n",maxx+(j*maxx)-1,(j*maxx));
   }


   /* Name field. Modify. */
   printf("n=CutCliffordTorus-%dx%d\n",maxx,maxy);

   /* Now you project! */
   return 0;
}
