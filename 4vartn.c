/*
 * 4VARTN.C
 * Okay. This module used to have alot of stuff to add and delete points and
 * lines, etc. but the only function that survived was loaddfile. :)
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

#include <stdio.h>
#include <stdlib.h>
#include "4vahead.h"

int loaddfile(filename) char *filename; {

  int i;
  FILE *in;

  if ((in=fopen(filename,"rt")) == NULL) {
      fprintf(stderr,"4VA: Cannot open object file %s.\n",filename);
      exit(-1);
  }

  else {
    printf(" Loading object file %s.\n",filename);
    fscanf(in,"p={%d}\n",&coptr->numpoints);
    printf(" %d points, ",coptr->numpoints);
    if (!(coptr->pts=(point_t *)malloc((coptr->numpoints)*(sizeof(point_t))))) {
      perror("malloc");
      exit(-1);
    }
    if (!(coptr->transpts=(point_t *)malloc((coptr->numpoints)*(sizeof(point_t))))) {
      perror("malloc");
      exit(-1);
    }
    for (i=0; i<coptr->numpoints; i++) {
      fscanf(in,"%f %f %f %f\n",&coptr->pts[i].x, &coptr->pts[i].y,
				&coptr->pts[i].z, &coptr->pts[i].w);
    }
    fscanf(in,"l={%d}\n",&coptr->numlines);
    printf("%d lines.\n",coptr->numlines);
    if (!(coptr->lns=(line_t *)malloc((coptr->numlines)*(sizeof(line_t))))) {
      perror("malloc");
      exit(-1);
    }
    for (i=0; i<coptr->numlines; i++) {
      fscanf(in,"%d %d\n",&coptr->lns[i].from, &coptr->lns[i].to);
    }
    fscanf(in,"n=%s",coptr->name);     

    fclose(in);
    return (1);
  }
}
