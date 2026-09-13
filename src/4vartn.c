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
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "4vahead.h"

char **objfiles;
int nobjfiles, curobj;

static void append(char *path) {
  char **grown;

  if (!path || !(grown = (char **)realloc(objfiles, (nobjfiles + 1) * sizeof(char *)))) {
    perror("malloc");
    exit(-1);
  }
  objfiles = grown;
  objfiles[nobjfiles++] = path;
}

static int cmpstr(const void *a, const void *b) {
  return strcmp(*(char * const *)a, *(char * const *)b);
}

void addpath(const char *path) {
  /* A directory stands for the .4vd files in it, in name order; anything else is taken as a file. */
  struct stat st;
  struct dirent *ent;
  DIR *dir;
  size_t len, plen = strlen(path);
  int first = nobjfiles;
  char *full;

  if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
    append(strdup(path));
    return;
  }
  if (!(dir = opendir(path))) {
    fprintf(stderr, "4VA: cannot read directory %s.\n", path);
    return;
  }
  while ((ent = readdir(dir))) {
    len = strlen(ent->d_name);
    if (len <= 4 || strcmp(ent->d_name + len - 4, ".4vd")) continue;
    if (!(full = (char *)malloc(plen + len + 2))) append(NULL);
    sprintf(full, "%s%s%s", path, (plen && path[plen-1] == '/') ? "" : "/", ent->d_name);
    if (stat(full, &st) == 0 && S_ISREG(st.st_mode)) append(full);
    else free(full);
  }
  closedir(dir);
  qsort(objfiles + first, nobjfiles - first, sizeof(char *), cmpstr);
  if (nobjfiles == first) fprintf(stderr, "4VA: no .4vd files in %s.\n", path);
}

int loaddfile(const char *filename) {
  /* Load into scratch arrays and only replace the current object once the whole file is valid. */
  int i, np, nl;
  FILE *in;
  point_t *pts = NULL, *transpts = NULL;
  line_t *lns = NULL;
  char name[32] = "";

  if ((in=fopen(filename,"r")) == NULL) {
    fprintf(stderr,"4VA: Cannot open object file %s.\n",filename);
    return -1;
  }
  printf(" Loading object file %s.\n",filename);

  if (fscanf(in," p={%d }",&np) != 1 || np <= 0) goto bad;
  if (!(pts=(point_t *)malloc(np*sizeof(point_t))) ||
      !(transpts=(point_t *)malloc(np*sizeof(point_t)))) goto nomem;
  for (i=0; i<np; i++) {
    if (fscanf(in,"%f %f %f %f",&pts[i].x,&pts[i].y,&pts[i].z,&pts[i].w) != 4) goto bad;
  }
  if (fscanf(in," l={%d }",&nl) != 1 || nl < 0) goto bad;
  if (!(lns=(line_t *)malloc((nl ? nl : 1)*sizeof(line_t)))) goto nomem;
  for (i=0; i<nl; i++) {
    if (fscanf(in,"%d %d",&lns[i].from,&lns[i].to) != 2 ||
        lns[i].from < 0 || lns[i].from >= np || lns[i].to < 0 || lns[i].to >= np) goto bad;
  }
  if (fscanf(in," n=%31s",name) != 1) name[0]='\0';
  fclose(in);

  free(coptr->pts); free(coptr->transpts); free(coptr->lns);
  coptr->pts=pts; coptr->transpts=transpts; coptr->lns=lns;
  coptr->numpoints=np; coptr->numlines=nl;
  strcpy(coptr->name,name);
  printf(" %d points, %d lines.\n",np,nl);
  return 0;

bad:
  fprintf(stderr,"4VA: %s is not a valid object file.\n",filename);
  goto fail;
nomem:
  perror("malloc");
fail:
  free(pts); free(transpts); free(lns);
  fclose(in);
  return -1;
}

int loadobject(int start, int step) {
  /* Load the first readable object starting at start and moving by step, wrapping; returns its index or -1. */
  int k, i;

  for (k=0; k<nobjfiles; k++) {
    i=((start + k*step) % nobjfiles + nobjfiles) % nobjfiles;
    if (loaddfile(objfiles[i]) == 0) {
      curobj=i;
      return i;
    }
  }
  return -1;
}
