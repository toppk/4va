/*
 * 4VAPROJ.C
 * This is 4ViewAuto Projection Unit...same as 4View but no axes!
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

#include "4vahead.h"
#include <math.h>

void fixangles(transfParams *params) {
  params->cxy = cos(params->rxy); params->sxy = sin(params->rxy);
  params->cxz = cos(params->rxz); params->sxz = sin(params->rxz);
  params->cyz = cos(params->ryz); params->syz = sin(params->ryz);
  params->cxw = cos(params->rxw); params->sxw = sin(params->rxw);
  params->cyw = cos(params->ryw); params->syw = sin(params->ryw);
  params->czw = cos(params->rzw); params->szw = sin(params->rzw);
}

float deg2rad(float ang) {
  return (ang * (M_PI/180));
}

void makeemptyparams(void) {
  emptyparams.rxy=0; emptyparams.rxz=0; emptyparams.ryz=0;
  emptyparams.rxw=0; emptyparams.ryw=0; emptyparams.rzw=0;
  fixangles(&emptyparams);
  emptyparams.trnx=emptyparams.trny=emptyparams.trnz=emptyparams.trnw=0;
  emptyparams.sclx=emptyparams.scly=emptyparams.sclz=emptyparams.sclw=1;
}

static void matrix(float *a, float *b, float sinr, float cosr)
{
  float tma;
  tma= *a;
  *a = (tma * cosr) - (*b * sinr);
  *b = (tma * sinr) + (*b * cosr);
}

static point_t transform(point_t thept, transfParams *params) {
  thept.x *= params->sclx;
  thept.y *= params->scly;
  thept.z *= params->sclz;
  thept.w *= params->sclw;

  thept.x += params->trnx;
  thept.y += params->trny;
  thept.z += params->trnz;
  thept.w += params->trnw;   

  matrix(&thept.x, &thept.y, params->sxy, params->cxy);
  matrix(&thept.x, &thept.z, params->sxz, params->cxz);
  matrix(&thept.y, &thept.z, params->syz, params->cyz);
  matrix(&thept.x, &thept.w, params->sxw, params->cxw);
  matrix(&thept.y, &thept.w, params->syw, params->cyw);
  matrix(&thept.z, &thept.w, params->szw, params->czw);

  return thept;
}

static void perspective(point_t *thept) {
  float mw, mz;
  mw= w_dist - thept->w; mz= z_dist - thept->z;


  /* Smash object against screen in case it gets too close. */
  if (mw < 3.4e-35) {
    mw = 3.4e-35;
  }

  if (mz < 3.4e-35) {
    mz = 3.4e-35;
  }

  /* Perspective over w */
  thept->x /= mw * (1/w_dist);
  thept->y /= mw * (1/w_dist);
  thept->z /= mw * (1/w_dist);

  /* Perspective over z */
  thept->x /= mz * (1/z_dist);
  thept->y /= mz * (1/z_dist);

}

void project(object *obj) {
/* This function no longer displays the object. It trnasforms all points and buffers the lines.
 * It doesn't draw anything. */

  int i, from, to;
  float x1, y1, x2, y2;
  
  for (i=0; i < obj->numpoints; i++) {   
    obj->transpts[i] = transform(obj->pts[i], &obj->params);
  }

  if (perspon) {
    for (i=0; i < obj->numpoints; i++) { 
      perspective(&obj->transpts[i]);
    }
  }

  for (i=0; i < obj->numlines; i++) {
    from=obj->lns[i].from;
    to=obj->lns[i].to;
    x1=obj->transpts[from].x+CENX;
    y1=SIZY-(obj->transpts[from].y+CENY);
    x2=obj->transpts[to].x+CENX;
    y2=SIZY-(obj->transpts[to].y+CENY);
    g_bufferline(x1,x2,y1,y2);
  }

}

