/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* copyright 1998-2000 by Warren Lyford Delano of DeLano Scientific. 
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* -------------------------------------------------------------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/
#include"os_std.h"

#include"Base.h"
#include"MemoryDebug.h"
#include"Err.h"
#include"Vector.h"
#include"OOMac.h"
#include"Setting.h"
#include"Ortho.h"
#include"Util.h"
#include"Ray.h"

#ifndef RAY_SMALL
#define RAY_SMALL 0.00001
#endif

/* BASES 
   0 contains untransformed vertices (vector size = 3)
	1 contains transformed vertices (vector size = 3)
	2 contains transformed vertices for shadowing 
*/

typedef float float3[3];
typedef float float4[4];

void RayRelease(CRay *I);
void RayTexture(CRay *I,int mode,float *v);
void RayTransparentf(CRay *I,float v);

void RaySetup(CRay *I);
void RayColor3fv(CRay *I,float *v);
void RaySphere3fv(CRay *I,float *v,float r);
void RayCylinder3fv(CRay *I,float *v1,float *v2,float r,float *c1,float *c2);

void RayTriangle3fv(CRay *I,
						  float *v1,float *v2,float *v3,
						  float *n1,float *n2,float *n3,
						  float *c1,float *c2,float *c3);

void RayApplyMatrix33( unsigned int n, float3 *q, const float m[16],
							float3 *p );

void RayExpandPrimitives(CRay *I);
void RayTransformFirst(CRay *I);
void RayTransformBasis(CRay *I,CBasis *B);

int PrimitiveSphereHit(CRay *I,float *v,float *n,float *minDist,int except);

void RayReflectSphere(CRay *I,RayInfo *r);

void RayTransformNormals33( unsigned int n, float3 *q, const float m[16],float3 *p );

/*========================================================================*/
void RayReflectSphere(CRay *I,RayInfo *r)
{
  
  r->impact[0]=r->base[0]; 
  r->impact[1]=r->base[1]; 
  r->impact[2]=r->base[2]-r->dist;
  
  r->surfnormal[0]=r->impact[0]-r->sphere[0];
  r->surfnormal[1]=r->impact[1]-r->sphere[1];
  r->surfnormal[2]=r->impact[2]-r->sphere[2];
  
  normalize3f(r->surfnormal);
  
  if(r->prim->texture)
    switch(r->prim->texture) {
    case 1:
      scatter3f(r->surfnormal,r->prim->texture_param[0]);
      break;
    case 2:
      wiggle3f(r->surfnormal,r->impact,r->prim->texture_param);
      break;
    }

  r->dotgle = -r->surfnormal[2]; 
  
  r->reflect[0]= - ( 2 * r->dotgle * r->surfnormal[0] );
  r->reflect[1]= - ( 2 * r->dotgle * r->surfnormal[1] );
  r->reflect[2]= -1.0 - ( 2 * r->dotgle * r->surfnormal[2] );
  
}
/*========================================================================*/
void RayExpandPrimitives(CRay *I)
{
  int a;
  float *v0,*v1,*n0,*n1;
  CBasis *basis;
  int nVert, nNorm;
 
  nVert=0;
  nNorm=0;
  for(a=0;a<I->NPrimitive;a++) {
	 switch(I->Primitive[a].type) {
	 case cPrimSphere:
		nVert++;
		break;
	 case cPrimCylinder:
		nVert++;
		nNorm++;
		break;
	 case cPrimTriangle:
		nVert+=3;
		nNorm+=4;
		break;
	 }
  }

  basis = I->Basis;
  
  VLACheck(basis->Vertex,float,3*nVert);
  VLACheck(basis->Radius,float,nVert);
  VLACheck(basis->Radius2,float,nVert);
  VLACheck(basis->Vert2Normal,int,nVert);
  VLACheck(basis->Normal,float,3*nNorm);
  VLACheck(I->Vert2Prim,int,nVert);
  basis->MaxRadius = 0.0;
  basis->MinVoxel = 0.0;
  basis->NVertex=nVert;
  basis->NNormal=nNorm;

  nVert=0;
  nNorm=0;
  v0=basis->Vertex;
  n0=basis->Normal;
  for(a=0;a<I->NPrimitive;a++) {
	 switch(I->Primitive[a].type) {
	 case cPrimTriangle:
		I->Primitive[a].vert=nVert;
		I->Vert2Prim[nVert]=a;
		I->Vert2Prim[nVert+1]=a;
		I->Vert2Prim[nVert+2]=a;
		basis->Radius[nVert]=I->Primitive[a].r1;
		basis->Radius2[nVert]=I->Primitive[a].r1*I->Primitive[a].r1; /*necessary??*/
		/*		if(basis->Radius[nVert]>basis->MinVoxel)
				basis->MinVoxel=basis->Radius[nVert];*/
		if(basis->MinVoxel<0.001F)
		  basis->MinVoxel=0.001F;
		basis->Vert2Normal[nVert]=nNorm;
		basis->Vert2Normal[nVert+1]=nNorm;
		basis->Vert2Normal[nVert+2]=nNorm;
		n1=I->Primitive[a].n0;
		(*n0++)=(*n1++);
		(*n0++)=(*n1++);
		(*n0++)=(*n1++);
		n1=I->Primitive[a].n1;
		(*n0++)=(*n1++);
		(*n0++)=(*n1++);
		(*n0++)=(*n1++);
		n1=I->Primitive[a].n2;
		(*n0++)=(*n1++);
		(*n0++)=(*n1++);
		(*n0++)=(*n1++);
		n1=I->Primitive[a].n3;
		(*n0++)=(*n1++);
		(*n0++)=(*n1++);
		(*n0++)=(*n1++);
		nNorm+=4;
		v1=I->Primitive[a].v1;
		(*v0++)=(*v1++);
		(*v0++)=(*v1++);
		(*v0++)=(*v1++);
		v1=I->Primitive[a].v2;
		(*v0++)=(*v1++);
		(*v0++)=(*v1++);
		(*v0++)=(*v1++);
		v1=I->Primitive[a].v3;
		(*v0++)=(*v1++);
		(*v0++)=(*v1++);
		(*v0++)=(*v1++);
		nVert+=3;
		break;
	 case cPrimSphere:
		I->Primitive[a].vert=nVert;
		I->Vert2Prim[nVert]=a;
		v1=I->Primitive[a].v1;
		basis->Radius[nVert]=I->Primitive[a].r1;
		basis->Radius2[nVert]=I->Primitive[a].r1*I->Primitive[a].r1; /*precompute*/
		if(basis->Radius[nVert]>basis->MaxRadius)
		  basis->MaxRadius=basis->Radius[nVert];
		(*v0++)=(*v1++);
		(*v0++)=(*v1++);
		(*v0++)=(*v1++);
		nVert++;
		break;
	 case cPrimCylinder:
		I->Primitive[a].vert=nVert;
		I->Vert2Prim[nVert]=a;
		basis->Radius[nVert]=I->Primitive[a].r1;
		basis->Radius2[nVert]=I->Primitive[a].r1*I->Primitive[a].r1; /*precompute*/
		if(basis->Radius[nVert]>basis->MinVoxel)
		  basis->MinVoxel=basis->Radius[nVert];
		subtract3f(I->Primitive[a].v2,I->Primitive[a].v1,n0);
		I->Primitive[a].l1=length3f(n0);
		normalize3f(n0);
		n0+=3;
		basis->Vert2Normal[nVert]=nNorm;
		nNorm++;
		v1=I->Primitive[a].v1;
		(*v0++)=(*v1++);
		(*v0++)=(*v1++);
		(*v0++)=(*v1++);
		nVert++;
		break;
	 }
  }
  /*  printf("minvoxel  %8.3f\n",basis->MinVoxel);
		printf("NPrimit  %d nvert %d\n",I->NPrimitive,nVert);*/
}
/*========================================================================*/
void RayTransformFirst(CRay *I)
{
  CBasis *basis0,*basis1;
  CPrimitive *prm;
  int a;
  float *v0;
  int backface_cull;

  backface_cull = (int)SettingGet(cSetting_backface_cull);

  basis0 = I->Basis;
  basis1 = I->Basis+1;
  
  VLACheck(basis1->Vertex,float,3*basis0->NVertex);
  VLACheck(basis1->Normal,float,3*basis0->NNormal);
  VLACheck(basis1->Precomp,float,3*basis0->NNormal);
  VLACheck(basis1->Vert2Normal,int,basis0->NVertex);
  VLACheck(basis1->Radius,float,basis0->NVertex);
  VLACheck(basis1->Radius2,float,basis0->NVertex);
  
  RayApplyMatrix33(basis0->NVertex,(float3*)basis1->Vertex,
					  I->ModelView,(float3*)basis0->Vertex);

  for(a=0;a<basis0->NVertex;a++)
	 {
		basis1->Radius[a]=basis0->Radius[a];
		basis1->Radius2[a]=basis0->Radius2[a];
		basis1->Vert2Normal[a]=basis0->Vert2Normal[a];
	 }

  basis1->MaxRadius=basis0->MaxRadius;
  basis1->MinVoxel=basis0->MinVoxel;
  basis1->NVertex=basis0->NVertex;

  RayTransformNormals33(basis0->NNormal,(float3*)basis1->Normal,
					  I->ModelView,(float3*)basis0->Normal);
  
  basis1->NNormal=basis0->NNormal;

  for(a=0;a<I->NPrimitive;a++) {
	 prm=I->Primitive+a;
	 if(prm->type==cPrimTriangle) {
		BasisTrianglePrecompute(basis1->Vertex+prm->vert*3,
										basis1->Vertex+prm->vert*3+3,
										basis1->Vertex+prm->vert*3+6,
										basis1->Precomp+basis1->Vert2Normal[prm->vert]*3);
      v0 = basis1->Normal + (basis1->Vert2Normal[prm->vert]*3 + 3);
      prm->cull = backface_cull&&((v0[2]<0.0)&&(v0[5]<0.0)&&(v0[8]<0.0));
	 }
  }

}
/*========================================================================*/
void RayTransformBasis(CRay *I,CBasis *basis1)
{
  CBasis *basis0;
  int a;
  float *v0,*v1;
  CPrimitive *prm;

  basis0 = I->Basis+1;

  VLACheck(basis1->Vertex,float,3*basis0->NVertex);
  VLACheck(basis1->Normal,float,3*basis0->NNormal);
  VLACheck(basis1->Precomp,float,3*basis0->NNormal);
  VLACheck(basis1->Vert2Normal,int,basis0->NVertex);
  VLACheck(basis1->Radius,float,basis0->NVertex);
  VLACheck(basis1->Radius2,float,basis0->NVertex);
  v0=basis0->Vertex;
  v1=basis1->Vertex;
  for(a=0;a<basis0->NVertex;a++)
	 {
		matrix_transform33f3f(basis1->Matrix,v0,v1);
		v0+=3;
		v1+=3;
		basis1->Radius[a]=basis0->Radius[a];
		basis1->Radius2[a]=basis0->Radius2[a];
		basis1->Vert2Normal[a]=basis0->Vert2Normal[a];
	 }
  v0=basis0->Normal;
  v1=basis1->Normal;
  for(a=0;a<basis0->NNormal;a++)
	 {
		matrix_transform33f3f(basis1->Matrix,v0,v1);
		v0+=3;
		v1+=3;
	 }
  basis1->MaxRadius=basis0->MaxRadius;
  basis1->MinVoxel=basis0->MinVoxel;
  basis1->NVertex=basis0->NVertex;
  basis1->NNormal=basis0->NNormal;


  for(a=0;a<I->NPrimitive;a++) {
	 prm=I->Primitive+a;
	 if(prm->type==cPrimTriangle) {
		BasisTrianglePrecompute(basis1->Vertex+prm->vert*3,
										basis1->Vertex+prm->vert*3+3,
										basis1->Vertex+prm->vert*3+6,
										basis1->Precomp+basis1->Vert2Normal[prm->vert]*3);

	 }
  }

  
}


/*========================================================================*/
void RayRenderPOV(CRay *I,int width,int height,char *charVLA,float front,float back,float fov)
{
  float *light;
  int antialias;
  int fogFlag=false;
  int fogRangeFlag=false;
  float fog;
  float *bkrd;
  float fog_start=0.0;
  float gamma;
  CBasis *base;
  CPrimitive *prim;
  float *vert;

  int a;

  PRINTFB(FB_Ray,FB_Details)
    " RayRenderPOV: w %d h %d f %8.3f b %8.3f\n",width,height,front,back
    ENDFB;
  if(Feedback(FB_Ray,FB_Details)) {
    dump3f(I->Volume," RayRenderPOV: vol");
    dump3f(I->Volume+3," RayRenderPOV: vol");
  }
  if(!charVLA) {
    charVLA=VLAlloc(char,10000);
  }
  gamma = SettingGet(cSetting_gamma);
  if(gamma>R_SMALL4)
    gamma=1.0/gamma;
  else
    gamma=1.0;

  fog = SettingGet(cSetting_ray_trace_fog);
  if(fog!=0.0) {
    fogFlag=true;
    fog_start = SettingGet(cSetting_ray_trace_fog_start);
    if(fog_start>R_SMALL4) {
      fogRangeFlag=true;
      if(fabs(fog_start-1.0)<R_SMALL4) /* prevent div/0 */
        fogFlag=false;
    }
  }
  /* SETUP */
  
  antialias = (int)SettingGet(cSetting_antialias);
  bkrd=SettingGetfv(cSetting_bg_rgb);

  RayExpandPrimitives(I);
  RayTransformFirst(I);
  
  PRINTFB(FB_Ray,FB_Details)
    " RayRenderPovRay: processed %i graphics primitives.\n",I->NPrimitive 
    ENDFB;
  base = I->Basis+1;
  printf("#include \"colors.inc\"\n");
  printf("  camera {location <0.0 , 0.0 , %10.6f>\nlook_at  <0.0 , 0.0 , -1.0> angle %10.6f}\n",
         front,fov*0.75);
  printf("    light_source{<-400,400,1000> color White}\n");

  for(a=0;a<I->NPrimitive;a++) {
    prim = I->Primitive+a;
    vert = base->Vertex+3*(prim->vert);
    switch(prim->type) {
	 case cPrimSphere:
      printf("sphere{<%10.6f,%10.6f,%10.6f>, %10.6f\n",
             -vert[0],vert[1],vert[2],prim->r1);
      printf("pigment{color rgb<1,0.8,0>}}\n");
		break;
	 case cPrimCylinder:
		break;
	 case cPrimTriangle:
		break;
    }
  }
  /*  BasisMakeMap(I->Basis+1,I->Vert2Prim,I->Primitive,I->Volume);*/
  
  light=SettingGetfv(cSetting_light);
  dump3f(light,"light");
  
}
/*========================================================================*/
void RayRender(CRay *I,int width,int height,unsigned int *image,float front,float back,
               double timing)
{
  int x,y;
  int a,b;
  unsigned int *p;
  float excess=0.0;
  float dotgle;
  float bright,direct_cmp,reflect_cmp,*v,fc[4];
  float ambient,direct,lreflect,ft,ffact,ffact1m;
  unsigned int c[4],aa,za;
  unsigned int *image_copy = NULL;
  int i;
  unsigned int back_mask,fore_mask=0;
  unsigned int background,buffer_size,z[16],zm[16],tot;
  int antialias;
  RayInfo r1,r2;
  int fogFlag=false;
  int fogRangeFlag=false;
  int opaque_back=0;
  
  float fog;
  float *bkrd;
  float fog_start=0.0;
  float gamma,inp,sig=1.0;
  double now;
  float persist,persist_inv;
  float new_front;
  int pass;
  unsigned int last_pixel=0,*pixel;
  int exclude;
  float lit;
  int backface_cull;
  
  backface_cull = (int)SettingGet(cSetting_backface_cull);
  opaque_back = (int)SettingGet(cSetting_ray_opaque_background);
  gamma = SettingGet(cSetting_gamma);
  if(gamma>R_SMALL4)
    gamma=1.0/gamma;
  else
    gamma=1.0;

  fog = SettingGet(cSetting_ray_trace_fog);
  if(fog!=0.0) {
    fogFlag=true;
    fog_start = SettingGet(cSetting_ray_trace_fog_start);
    if(fog_start>R_SMALL4) {
      fogRangeFlag=true;
      if(fabs(fog_start-1.0)<R_SMALL4) /* prevent div/0 */
        fogFlag=false;

    }
  }
  /* SETUP */
  
  antialias = (int)SettingGet(cSetting_antialias);
  if(antialias>0) {
	 width=width*2;
	 height=height*2;
	 image_copy = image;
	 buffer_size = 4*width*height;
	 image = (void*)Alloc(char,buffer_size);
	 ErrChkPtr(image);
  }

  bkrd=SettingGetfv(cSetting_bg_rgb);
  if(opaque_back) {
    if(I->BigEndian)
      back_mask = 0x000000FF;
    else
      back_mask = 0xFF000000;
    fore_mask = back_mask;
  } else {
    if(I->BigEndian) {
      back_mask = 0x00000000;
    } else {
      back_mask = 0x00000000;
    }
  }
  if(I->BigEndian) {
    background = back_mask|
      ((0xFF& ((unsigned int)(bkrd[0]*255))) <<24)|
      ((0xFF& ((unsigned int)(bkrd[1]*255))) <<16)|
      ((0xFF& ((unsigned int)(bkrd[2]*255))) <<8 );
  } else {
    background = back_mask|
      ((0xFF& ((unsigned int)(bkrd[2]*255))) <<16)|
      ((0xFF& ((unsigned int)(bkrd[1]*255))) <<8)|
      ((0xFF& ((unsigned int)(bkrd[0]*255))) );
  }

  PRINTFB(FB_Ray,FB_Blather) 
    " RayNew: Background = %x %d %d %d\n",background,(int)(bkrd[0]*255),
    (int)(bkrd[1]*255),(int)(bkrd[2]*255)
    ENDFB;

  if(!I->NPrimitive) {
    p=(unsigned int*)image; 
    for(x=0;x<width;x++)
      for(y=0;y<height;y++)
        *(p++)=background;
  } else {
    
    ambient=SettingGet(cSetting_ambient);
    lreflect=SettingGet(cSetting_reflect);
    direct=SettingGet(cSetting_direct);
    
    RayExpandPrimitives(I);
    RayTransformFirst(I);
    
    now = UtilGetSeconds()-timing;

	 PRINTFB(FB_Ray,FB_Details)
      " Ray: processed %i graphics primitives in %4.2f sec.\n",I->NPrimitive,now
      ENDFB;
    BasisMakeMap(I->Basis+1,I->Vert2Prim,I->Primitive,I->Volume);
    
    I->NBasis=3; /* light source */
    BasisInit(I->Basis+2);
    
    v=SettingGetfv(cSetting_light);
    
    I->Basis[2].LightNormal[0]=v[0];
    I->Basis[2].LightNormal[1]=v[1];
    I->Basis[2].LightNormal[2]=v[2];
    normalize3f(I->Basis[2].LightNormal);
    
    BasisSetupMatrix(I->Basis+2);
    RayTransformBasis(I,I->Basis+2);
    BasisMakeMap(I->Basis+2,I->Vert2Prim,I->Primitive,NULL);

    now = UtilGetSeconds()-timing;

#ifdef _MemoryDebug_ON
    PRINTFB(FB_Ray,FB_Details)
      " Ray: voxels: [%4.2f:%dx%dx%d], [%4.2f:%dx%dx%d], %d MB, %4.2f sec.\n",
      I->Basis[1].Map->Div,   I->Basis[1].Map->Dim[0],
      I->Basis[1].Map->Dim[1],I->Basis[1].Map->Dim[2],
      I->Basis[2].Map->Div,   I->Basis[2].Map->Dim[0],
      I->Basis[1].Map->Dim[2],I->Basis[2].Map->Dim[2],
      (int)(MemoryDebugUsage()/(1024.0*1024)),
      now
      ENDFB;
#else
    PRINTFB(FB_Ray,FB_Details)
      " Ray: voxels: [%4.2f:%dx%dx%d], [%4.2f:%dx%dx%d], %4.2f sec.\n",
      I->Basis[1].Map->Div,   I->Basis[1].Map->Dim[0],
      I->Basis[1].Map->Dim[1],I->Basis[1].Map->Dim[2],
      I->Basis[2].Map->Div,   I->Basis[2].Map->Dim[0],
      I->Basis[1].Map->Dim[2],I->Basis[2].Map->Dim[2],
      now
      ENDFB;
#endif

    /* IMAGING */
    
    /* erase buffer */
    
    p=(unsigned int*)image; 
    for(a=0;a<height;a++)
      for(b=0;b<width;b++)
        *p++=back_mask;
    
    /* ray-trace */
	 r1.base[2]=0.0;
    for(x=0;x<width;x++)
      {
        if(!(x&0xF)) OrthoBusyFast(x,width); /* don't slow down rendering too much */
        r1.base[0]=(((float)x)/width)*I->Range[0]+I->Volume[0];
        for(y=0;y<height;y++)
          {
            pixel = image+((width)*y)+x;
            exclude = -1;
            r1.base[1]=(((float)y)/height)*I->Range[1]+I->Volume[2];
            persist = 1.0;
            pass = 0;
            new_front=front;
            while((persist>0.0001)&&(pass<25)) {
              i=BasisHit(I->Basis+1,&r1,exclude,I->Vert2Prim,I->Primitive,false,new_front,back);
              if(i>=0) {
                new_front=r1.dist;
                if(r1.prim->type==cPrimTriangle) {
                  BasisReflectTriangle(I->Basis+1,&r1,i,fc);
                } else {
                  RayReflectSphere(I,&r1);/*,zRay,sphere,dist,surf,impact,reflect,&dotgle);*/
                  if(r1.prim->type==cPrimCylinder) {
                    ft = r1.tri1;
                    fc[0]=(r1.prim->c1[0]*(1-ft))+(r1.prim->c2[0]*ft);
                    fc[1]=(r1.prim->c1[1]*(1-ft))+(r1.prim->c2[1]*ft);
                    fc[2]=(r1.prim->c1[2]*(1-ft))+(r1.prim->c2[2]*ft);
                  } else {
                    fc[0]=r1.prim->c1[0];
                    fc[1]=r1.prim->c1[1];
                    fc[2]=r1.prim->c1[2];
                  }
                }
                dotgle=-r1.dotgle;
                              
                if(dotgle<0.0) dotgle=0.0;
                direct_cmp=(dotgle+(pow(dotgle,SettingGet(cSetting_power))))/2.0;
                
                matrix_transform33f3f(I->Basis[2].Matrix,r1.impact,r2.base);
                
                if(BasisHit(I->Basis+2,&r2,i,I->Vert2Prim,I->Primitive,true,0.0,0.0)>=0) {
                  lit=pow(r2.prim->trans,0.5);
                } else {
                  lit=1.0;
                }
                if(lit>0.0) {
                  dotgle=-dot_product3f(r1.surfnormal,I->Basis[2].LightNormal);
                  
                  if(dotgle<0.0) dotgle=0.0;
                  reflect_cmp=lit*(dotgle+(pow(dotgle,SettingGet(cSetting_power))))/2.0;
                  excess=pow(dotgle,SettingGet(cSetting_spec_power))*
                    SettingGet(cSetting_spec_reflect)*lit;
                } else {
                  excess=0.0;
                  reflect_cmp=0.0;
                }
                
                bright=ambient+(1.0-ambient)*(direct*direct_cmp+
                                              (1.0-direct)*direct_cmp*lreflect*reflect_cmp);
                
                if(bright>1.0) bright=1.0;
                if(bright<0.0) bright=0.0;
                
                fc[0] = (bright*fc[0]+excess);
                fc[1] = (bright*fc[1]+excess);
                fc[2] = (bright*fc[2]+excess);
                
                if (fogFlag) {
                  ffact = fog*(front-r1.dist)/(front-back);
                  if(fogRangeFlag) {
                    ffact = (ffact-fog_start)/(1.0-fog_start);
                  }
                  if(ffact<0.0)
                    ffact=0.0;
                  if(ffact>1.0)
                    ffact=0.0;
                  ffact1m = 1.0-ffact;
                  if(opaque_back) {
                    fc[0]=ffact*bkrd[0]+fc[0]*ffact1m;
                    fc[1]=ffact*bkrd[1]+fc[1]*ffact1m;
                    fc[2]=ffact*bkrd[2]+fc[2]*ffact1m;
                  } else {
                    fc[3]=1.0-ffact;
                  }
                }
                
                inp=(fc[0]+fc[1]+fc[2])/3.0;
                if(inp<R_SMALL4) 
                  sig=1.0;
                else {
                  sig = pow(inp,gamma)/inp;
                }
                
                c[0]=(uint)(sig*fc[0]*255.0F);
                c[1]=(uint)(sig*fc[1]*255.0F);
                c[2]=(uint)(sig*fc[2]*255.0F);
                
                if(c[0]>255) c[0]=255;
                if(c[1]>255) c[1]=255;
                if(c[2]>255) c[2]=255;
                /*			{ int a1,b1,c1;
                           if(MapInsideXY(I->Basis[1].Map,base,&a1,&b1,&c1))				
                           {
                           c[0]=255.0*((float)a1)/I->Basis[1].Map->iMax[0];
                           c[1]=255.0*((float)b1)/I->Basis[1].Map->iMax[1];
                           c[2]=0xFF&(c[0]*c[1]);
                           }}
                */
                
                if(opaque_back) { 
                  if(I->BigEndian) {
                    *(image+((width)*y)+x)=
                      fore_mask|(c[0]<<24)|(c[1]<<16)|(c[2]<<8);
                  } else {
                    *(image+((width)*y)+x)=
                      fore_mask|(c[2]<<16)|(c[1]<<8)|c[0];
                  }
                } else { /* use alpha channel for fog with transparent backgrounds */
                  c[3] = (uint)(fc[3]*255.0F);
                  if(c[3]>255) c[3]=255;
                  if(I->BigEndian) {
                    *(pixel) = (c[0]<<24)|(c[1]<<16)|(c[2]<<8)|c[3];
                  } else {
                    *(pixel) = (c[3]<<24)|(c[2]<<16)|(c[1]<<8)|c[0];
                  }
                }
              } else {
                *(image+((width)*y)+x)=
                  background;
              }

              if(pass) { /* average all four channels */
                persist_inv = 1.0-persist;
                fc[0] = (0xFF&((*pixel)>>24))*persist + (0xFF&(last_pixel>>24))*persist_inv;
                fc[1] = (0xFF&((*pixel)>>16))*persist + (0xFF&(last_pixel>>16))*persist_inv;
                fc[2] = (0xFF&((*pixel)>>8))*persist + (0xFF&(last_pixel>>8))*persist_inv;
                fc[3] = (0xFF&((*pixel)))*persist + (0xFF&(last_pixel))*persist_inv;
                
                c[0]=(uint)(fc[0]);
                c[1]=(uint)(fc[1]);
                c[2]=(uint)(fc[2]);
                c[3]=(uint)(fc[3]);
                
                if(c[0]>255) c[0]=255;
                if(c[1]>255) c[1]=255;
                if(c[2]>255) c[2]=255;
                if(c[3]>255) c[3]=255;
                
                *(pixel) = (c[0]<<24)|(c[1]<<16)|(c[2]<<8)|c[3];
              }
              if(i>=0) { 
                if(!backface_cull) 
                  persist = persist * r1.prim->trans;
                else {
                  if((persist<0.9999)&&(r1.prim->trans)) { /* don't combine transparent surfaces */
                    *(pixel)=last_pixel;
                  } else {
                    persist = persist * r1.prim->trans;
                  }
                }
              }
              if(i<0) {
                break; /* hit nothing */
              } else {
                last_pixel = (*pixel);
                exclude = i;
                pass++;
              }
            }
          }
      }
  }

  if(antialias) {
	 OrthoBusyFast(9,10);
	 width=width/2;
	 height=height/2;
	 for(x=1;x<(width-1);x++)
		for(y=1;y<(height-1);y++)
		  {
			 aa=0;

			 p = image+((width*2)*(y*2-1))+(x*2);
			 
          /*  12  4   5  13 
           *  6   0   1   7
           *  8   2   3   9
           *  14 10  11  15
           *
           * 0-3 are weighted double
           */

          z[12] = (*(p-1));
			 z[4] = (*(p));
			 z[5] = (*(p+1));
          z[13] = (*(p+2));
			 p+=(width*2);
			 z[6] = (*(p-1));
			 z[0] = (*(p));
			 z[1] = (*(p+1));
			 z[7] = (*(p+2));
			 p+=(width*2);
			 z[8] = (*(p-1));
			 z[2] = (*(p));
			 z[3] = (*(p+1));
			 z[9] = (*(p+2));
			 p+=(width*2);
          z[14] = (*(p-1));
			 z[10] = (*(p));
			 z[11] = (*(p+1));
          z[15] = (*(p+2));

			 if(I->BigEndian) { 
				for(a=0;a<16;a++) {
              zm[a]=z[a]&0xFF; /* copy alpha channel */
				  z[a]=z[a]>>8;
            }
			 } else {
				for(a=0;a<16;a++) {
              zm[a]=z[a]>>24; /* copy alpha channel */
            }
          }
          
          
			 tot=0;
			 for(a=0;a<16;a++) /* average alpha channel */
				tot+=(zm[a]&0xFF);
			 for(a=0;a<4;a++)
				tot+=(zm[a]&0xFF)<<2;
			 za = (0xFF&(tot>>5));

			 tot=0;
			 for(a=0;a<16;a++)
				tot+=(z[a]&0xFF);
			 for(a=0;a<4;a++)
				tot+=(z[a]&0xFF)<<2;
			 aa=aa|(0xFF&(tot>>5));
			 
			 tot=0;
			 for(a=0;a<16;a++)
				tot+=(z[a]&0xFF00);
			 for(a=0;a<4;a++)
				tot+=(z[a]&0xFF00)<<2;
			 aa=aa|(0xFF00&(tot>>5));

			 tot=0;
			 for(a=0;a<16;a++)
				tot+=(z[a]&0xFF0000);
			 for(a=0;a<4;a++)
				tot+=(z[a]&0xFF0000)<<2;
			 aa=aa|(0xFF0000&(tot>>5));			 
			 
			 if(I->BigEndian) {
				aa=(aa<<8)|za;
			 } else {
				aa=aa|(za<<24);
			 }
			 
			 *(image_copy+((width)*y)+x) = aa;			 
		  }

    /* top and bottom edges */

	 for(x=0;x<width;x++)
		for(y=0;y<height;y=y+(height-1))
		  {
			 aa=0;

			 p = image+((width*2)*(y*2))+(x*2);
			 
			 z[0] = (*(p));
			 z[1] = (*(p+1));
			 p+=(width*2);
			 z[2] = (*(p));
			 z[3] = (*(p+1));

			 if(I->BigEndian) { 
				for(a=0;a<4;a++) {
              zm[a]=z[a]&0xFF;
				  z[a]=z[a]>>8;
            }
			 } else {
				for(a=0;a<4;a++) {
              zm[a]=z[a]>>24; /* copy alpha channel */
            }
          }

			 tot=0;
			 for(a=0;a<4;a++)
				tot+=(zm[a]&0xFF);
			 za=(0xFF&(tot>>2));

			 tot=0;
			 for(a=0;a<4;a++)
				tot+=(z[a]&0xFF);
			 aa=aa|(0xFF&(tot>>2));
			 
			 tot=0;
			 for(a=0;a<4;a++)
				tot+=(z[a]&0xFF00);
			 aa=aa|(0xFF00&(tot>>2));

			 tot=0;
			 for(a=0;a<4;a++)
				tot+=(z[a]&0xFF0000);
			 aa=aa|(0xFF0000&(tot>>2));			 
			 
			 if(I->BigEndian) {
				aa=(aa<<8)|za;
			 } else {
				aa=aa|(za<<24);
			 }
			 *(image_copy+((width)*y)+x) = aa;			 
		  }

    /* left and right edges */

	 for(x=0;x<width;x=x+(width-1))
		for(y=0;y<height;y++)
		  {
			 aa=0;

			 p = image+((width*2)*(y*2))+(x*2);
			 
			 z[0] = (*(p));
			 z[1] = (*(p+1));
			 p+=(width*2);
			 z[2] = (*(p));
			 z[3] = (*(p+1));

			 if(I->BigEndian) { 
				for(a=0;a<4;a++) {
              zm[a]=z[a]&0xFF;
				  z[a]=z[a]>>8;
            }
			 } else {
				for(a=0;a<4;a++) {
              zm[a]=z[a]>>24; /* copy alpha channel */
            }
          }

			 tot=0;
			 for(a=0;a<4;a++)
				tot+=(zm[a]&0xFF);
			 za=(0xFF&(tot>>2));

			 tot=0;
			 for(a=0;a<4;a++)
				tot+=(z[a]&0xFF);
			 aa=aa|(0xFF&(tot>>2));
			 
			 tot=0;
			 for(a=0;a<4;a++)
				tot+=(z[a]&0xFF00);
			 aa=aa|(0xFF00&(tot>>2));

			 tot=0;
			 for(a=0;a<4;a++)
				tot+=(z[a]&0xFF0000);
			 aa=aa|(0xFF0000&(tot>>2));			 
			 
			 if(I->BigEndian) {
				aa=(aa<<8)|za;
			 } else {
				aa=aa|(za<<24);
			 }
			 *(image_copy+((width)*y)+x) = aa;			 
		  }

	 FreeP(image);
	 image=image_copy;
  }
  

}

/*========================================================================*/
void RayTexture(CRay *I,int mode,float *v)
{
  I->Texture=mode;
  if(v) 
    copy3f(v,I->TextureParam);
}
/*========================================================================*/
void RayTransparentf(CRay *I,float v)
{
  I->Trans=v;
}
/*========================================================================*/
void RayColor3fv(CRay *I,float *v)
{
  I->CurColor[0]=(*v++);
  I->CurColor[1]=(*v++);
  I->CurColor[2]=(*v++);
}
/*========================================================================*/
void RaySphere3fv(CRay *I,float *v,float r)
{
  CPrimitive *p;

  float *vv;

  VLACheck(I->Primitive,CPrimitive,I->NPrimitive);
  p = I->Primitive+I->NPrimitive;

  p->type = cPrimSphere;
  p->r1=r;
  p->trans=0.0;
  p->texture=I->Texture;
  copy3f(I->TextureParam,p->texture_param);
  vv=p->v1;
  (*vv++)=(*v++);
  (*vv++)=(*v++);
  (*vv++)=(*v++);
  vv=p->c1;
  v=I->CurColor;
  (*vv++)=(*v++);
  (*vv++)=(*v++);
  (*vv++)=(*v++);
  I->NPrimitive++;
}
/*========================================================================*/
void RayCylinder3fv(CRay *I,float *v1,float *v2,float r,float *c1,float *c2)
{
  CPrimitive *p;

  float *vv;

  VLACheck(I->Primitive,CPrimitive,I->NPrimitive);
  p = I->Primitive+I->NPrimitive;

  p->type = cPrimCylinder;
  p->r1=r;
  p->trans=0.0;
  p->texture=I->Texture;
  copy3f(I->TextureParam,p->texture_param);

  vv=p->v1;
  (*vv++)=(*v1++);
  (*vv++)=(*v1++);
  (*vv++)=(*v1++);
  vv=p->v2;
  (*vv++)=(*v2++);
  (*vv++)=(*v2++);
  (*vv++)=(*v2++);

  vv=p->c1;
  (*vv++)=(*c1++);
  (*vv++)=(*c1++);
  (*vv++)=(*c1++);
  vv=p->c2;
  (*vv++)=(*c2++);
  (*vv++)=(*c2++);
  (*vv++)=(*c2++);

  I->NPrimitive++;
}
/*========================================================================*/
void RayTriangle3fv(CRay *I,
						  float *v1,float *v2,float *v3,
						  float *n1,float *n2,float *n3,
						  float *c1,float *c2,float *c3)
{
  CPrimitive *p;

  float *vv;
  float n0[3],nx[3],s1[3],s2[3],s3[3];
  float l1,l2,l3;

  /*  dump3f(v1," v1");
  dump3f(v2," v2");
  dump3f(v3," v3");
  dump3f(n1," n1");
  dump3f(n2," n2");
  dump3f(n3," n3");
  dump3f(c1," c1");
  dump3f(c2," c2");
  dump3f(c3," c3");*/
  VLACheck(I->Primitive,CPrimitive,I->NPrimitive);
  p = I->Primitive+I->NPrimitive;

  p->type = cPrimTriangle;
  p->trans=I->Trans;
  p->texture=I->Texture;
  copy3f(I->TextureParam,p->texture_param);

  /* determine exact triangle normal */
  add3f(n1,n2,nx);
  add3f(n3,nx,nx);
  subtract3f(v1,v2,s1);
  subtract3f(v3,v2,s2);
  subtract3f(v1,v3,s3);
  cross_product3f(s1,s2,n0);
  if((fabs(n0[0])<RAY_SMALL)&&
	  (fabs(n0[1])<RAY_SMALL)&&
	  (fabs(n0[2])<RAY_SMALL))
	 {copy3f(nx,n0);} /* fall-back */
  else if(dot_product3f(n0,nx)<0)
	 invert3f(n0);
  normalize3f(n0);

  vv=p->n0;
  (*vv++)=n0[0];
  (*vv++)=n0[1];
  (*vv++)=n0[2];

  /* determine maximum distance from vertex to point */
  l1=length3f(s1);
  l2=length3f(s2);
  l3=length3f(s3);
  if(l2>l1) { if(l3>l2)	l1=l3; else	l1=l2;  }
  /* store cutoff distance */

  p->r1=l1*0.6;

  /*  if(l1>20) {
		printf("%8.3f\n",l1);
		printf("%8.3f %8.3f %8.3f\n",s1[0],s1[1],s1[2]);
		printf("%8.3f %8.3f %8.3f\n",s2[0],s2[1],s2[2]);
		printf("%8.3f %8.3f %8.3f\n",s3[0],s3[1],s3[2]);
		}*/

  vv=p->v1;
  (*vv++)=(*v1++);
  (*vv++)=(*v1++);
  (*vv++)=(*v1++);
  vv=p->v2;
  (*vv++)=(*v2++);
  (*vv++)=(*v2++);
  (*vv++)=(*v2++);
  vv=p->v3;
  (*vv++)=(*v3++);
  (*vv++)=(*v3++);
  (*vv++)=(*v3++);

  vv=p->c1;
  (*vv++)=(*c1++);
  (*vv++)=(*c1++);
  (*vv++)=(*c1++);
  vv=p->c2;
  (*vv++)=(*c2++);
  (*vv++)=(*c2++);
  (*vv++)=(*c2++);
  vv=p->c3;
  (*vv++)=(*c3++);
  (*vv++)=(*c3++);
  (*vv++)=(*c3++);

  vv=p->n1;
  (*vv++)=(*n1++);
  (*vv++)=(*n1++);
  (*vv++)=(*n1++);
  vv=p->n2;
  (*vv++)=(*n2++);
  (*vv++)=(*n2++);
  (*vv++)=(*n2++);
  vv=p->n3;
  (*vv++)=(*n3++);
  (*vv++)=(*n3++);
  (*vv++)=(*n3++);

  I->NPrimitive++;

}
/*========================================================================*/
CRay *RayNew(void)
{
  unsigned int test;
  unsigned char *testPtr;

  OOAlloc(CRay);
  
  test = 0xFF000000;
  testPtr = (unsigned char*)&test;
  I->BigEndian = *testPtr&&1;
  I->Trans=0.0;
  I->Texture=0;
  zero3f(I->TextureParam);

  PRINTFB(FB_Ray,FB_Blather) 
    " RayNew: BigEndian = %d\n",I->BigEndian
    ENDFB;

  I->Basis=VLAlloc(CBasis,10);
  BasisInit(I->Basis);
  BasisInit(I->Basis+1);
  I->Vert2Prim=VLAlloc(int,1);
  I->NBasis=2;
  I->Primitive=NULL;
  I->NPrimitive=0;
  I->fColor3fv=RayColor3fv;
  I->fSphere3fv=RaySphere3fv;
  I->fCylinder3fv=RayCylinder3fv;
  I->fTriangle3fv=RayTriangle3fv;
  I->fTexture=RayTexture;
  I->fTransparentf=RayTransparentf;
  
  return(I);
}
/*========================================================================*/
void RayPrepare(CRay *I,float v0,float v1,float v2,float v3,float v4,float v5,float *mat)
	  /*prepare for vertex calls */
{
  int a;
  if(!I->Primitive) 
	 I->Primitive=VLAlloc(CPrimitive,100);  
  if(!I->Vert2Prim) 
	 I->Vert2Prim=VLAlloc(int,100);
  I->Volume[0]=v0;
  I->Volume[1]=v1;
  I->Volume[2]=v2;
  I->Volume[3]=v3;
  I->Volume[4]=v4;
  I->Volume[5]=v5;
  I->Range[0]=I->Volume[1]-I->Volume[0];
  I->Range[1]=I->Volume[3]-I->Volume[2];
  I->Range[2]=I->Volume[5]-I->Volume[4];

  for(a=0;a<16;a++)
    I->ModelView[a]=mat[a];
}

/*========================================================================*/
void RayRelease(CRay *I)
{
  int a;

  for(a=0;a<I->NBasis;a++) {
	 BasisFinish(&I->Basis[a]);
  }
  I->NBasis=0;
  VLAFreeP(I->Primitive);
  VLAFreeP(I->Vert2Prim);
}
/*========================================================================*/
void RayFree(CRay *I)
{
  RayRelease(I);
  VLAFreeP(I->Basis);
  VLAFreeP(I->Vert2Prim);
  OOFreeP(I);
}
/*========================================================================*/

void RayApplyMatrix33( unsigned int n, float3 *q, const float m[16],
                          float3 *p )
{
   {
      unsigned int i;
      float m0 = m[0],  m4 = m[4],  m8 = m[8],  m12 = m[12];
      float m1 = m[1],  m5 = m[5],  m9 = m[9],  m13 = m[13];
      for (i=0;i<n;i++) {
         float p0 = p[i][0], p1 = p[i][1], p2 = p[i][2];
         q[i][0] = m0 * p0 + m4  * p1 + m8 * p2 + m12;
         q[i][1] = m1 * p0 + m5  * p1 + m9 * p2 + m13;
      }
   }
   {
      unsigned int i;
      float m2 = m[2],  m6 = m[6],  m10 = m[10],  m14 = m[14];
      float m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];
      if (m3==0.0F && m7==0.0F && m11==0.0F && m15==1.0F) {
         /* common case */
         for (i=0;i<n;i++) {
            float p0 = p[i][0], p1 = p[i][1], p2 = p[i][2];
            q[i][2] = m2 * p0 + m6 * p1 + m10 * p2 + m14;
				/*				q[i][3] = 1.0F;*/
         }
      }
      else {
         /* general case */
         for (i=0;i<n;i++) {
            float p0 = p[i][0], p1 = p[i][1], p2 = p[i][2];
            q[i][2] = m2 * p0 + m6 * p1 + m10 * p2 + m14;
				/*				q[i][3] = m3 * p0 + m7 * p1 + m11 * p2 + m15; */
         }
      }
   }
}

void RayTransformNormals33( unsigned int n, float3 *q, const float m[16],float3 *p )
{
   {
      unsigned int i;
      float m0 = m[0],  m4 = m[4],  m8 = m[8];
      float m1 = m[1],  m5 = m[5],  m9 = m[9];
      for (i=0;i<n;i++) {
         float p0 = p[i][0], p1 = p[i][1], p2 = p[i][2];
         q[i][0] = m0 * p0 + m4  * p1 + m8 * p2;
         q[i][1] = m1 * p0 + m5  * p1 + m9 * p2;
      }
   }
   {
      unsigned int i;
      float m2 = m[2],  m6 = m[6],  m10 = m[10];
      float m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];
      if (m3==0.0F && m7==0.0F && m11==0.0F && m15==1.0F) {
         /* common case */
         for (i=0;i<n;i++) {
            float p0 = p[i][0], p1 = p[i][1], p2 = p[i][2];
            q[i][2] = m2 * p0 + m6 * p1 + m10 * p2;
         }
      }
      else {
         /* general case */
         for (i=0;i<n;i++) {
            float p0 = p[i][0], p1 = p[i][1], p2 = p[i][2];
            q[i][2] = m2 * p0 + m6 * p1 + m10 * p2;
         }
      }
   }
	{      
	  unsigned int i;
	  for (i=0;i<n;i++) { /* renormalize - can we do this to the matrix instead? */
		 normalize3f(q[i]);
	  }
	}
}



