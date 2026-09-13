/*
 * 4VDMAKE.C by Matt Welsh
 * Your generic program to make a 4View .4VD file of a 3-d function. Can
 * of course be expanded to 4d functions easily. Spits coordinates out
 * to stdout, then figures out all of the line connections for the grid and
 * spits them out to. Please redirect (!). 
 * **************************************************************************
 * You are free to distribute this and all code and documentation for
 * the package '4VA' in any form, provided you:
 * 1. Don't sell it.
 * 2. Keep the copyright notice intact on all modules, compiled versions,
 *    and documentation.
 * 3. Give the original author(s) credit if you use this code in your
 *    own work or base another program on it. 
 * ***************************************************************************
 * v2.1.2 mdw 29 Sep 91
 */


#include <stdio.h>
#include <math.h>

main (argc, argv)
int argc; char **argv;
{

   double x,y,z;
   double tempx, tempy;
   int maxx, maxy;
   int i,j;

   if (argc!=3) {
     fprintf(stderr,"4VDMAKE: need parameters.\n");
	 fprintf(stderr,"  usage: 4VDMAKE <x grid size> <y grid size>\n");
	 exit(-1);
   }
   maxx=atoi(argv[1]);
   maxy=atoi(argv[2]);

   printf("p={%d}\n",maxx*maxy);

   for (y=0; y<maxy; y++) {
     for (x=0; x<maxx; x++) {
   
      /* squeeze everything into a -1 to 1 grid. Apply your calculations to tempy and
       * tempx ONLY!!!! */
      tempx=(x/(maxx/2))-1;
      tempy=(y/(maxy/2))-1;


      /* okay... put your function here. Not #defined 'coz you may need 
	   * intermediate calculations. */
	   z=(sin(tempx*15)+sin(tempy*15))/5;
     
      printf("%f %f %f 0\n",tempx,z,tempy);
     }
   }

   /* Now spit out the grid connections. You don't need to touch
    * this code unless you want to. :) */
   printf("l={%d}\n",(maxx-1)*(maxy)+(maxy-1)*(maxx));
   /* do the horizontal lines across x */
   for (j=0; j<maxy; j++) {
     for (i=0; i<maxx-1; i++) {
	   printf("%d %d\n",i+(j*maxx),i+(j*maxx)+1);
	 }
   }
   /* do vertical lines across x */
   for (j=0; j<maxy-1; j++) {
     for (i=0; i<maxx; i++) {
	   printf("%d %d\n",i+(j*maxx), i+(j*maxx)+maxx);
	 }
   }

   /* Name field. Modify. */
   printf("n=4VDMMAKEV2.1.2CREATED\n");

   /* Now you project! */
}
