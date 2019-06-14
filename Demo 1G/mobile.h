/*REPRIS DU SAMPLE VORONOI GPU*/
#ifndef MOBILE_H

#define MOBILE_H

#ifdef __cplusplus
extern "C" {
#endif
  
  extern void mobile2texture(float * f);
  extern void mobileInit(int n, int w, int h);
  extern void mobileMove(void);
  
#ifdef __cplusplus
}
#endif
#endif
