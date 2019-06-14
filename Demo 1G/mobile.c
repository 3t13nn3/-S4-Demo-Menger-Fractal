/*REPRIS DU SAMPLE VORONOI GPU*/
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GL4D/gl4du.h>
#include <SDL_image.h>

typedef struct mobile_t mobile_t;
struct mobile_t {
  double x, y, vx, vy;
  float color[3];
  Uint32 r;
};

static mobile_t * _mobile = NULL;
static int _nb_mobiles = 0;
static int _hasInit = 0;
static int _w = 1, _h = 1;

static void quit(void) {
  _nb_mobiles = 0;
  if(_mobile) {
    free(_mobile);
    _mobile = NULL;
  }
}

void mobile2texture(float * f) {
  int i;
  for(i = 0; i < _nb_mobiles; i++) {
    f[8 * i + 0] = _mobile[i].color[0];
    f[8 * i + 1] = _mobile[i].color[1];
    f[8 * i + 2] = _mobile[i].color[2];
    f[8 * i + 3] = 1;
    f[8 * i + 4] = _mobile[i].x / _w;
    f[8 * i + 5] = _mobile[i].y / _h;
    f[8 * i + 6] = _mobile[i].r / ((GLfloat)MIN(_w, _h));
    f[8 * i + 7] = 0;
  }
}

void mobileInit(int n, int w, int h) {
  int i;
  _w = w; _h = h;
  srand(time(NULL));
  _nb_mobiles = n;
  if(_mobile) {
    free(_mobile);
    _mobile = NULL;
  }
  if(!_hasInit) {
    atexit(quit);
    _hasInit = 1;
  }
  _mobile = malloc(_nb_mobiles * sizeof *_mobile);
  assert(_mobile);
  for(i = 0; i < _nb_mobiles; i++) {
    _mobile[i].x = gl4dmURand() * _w ;
    _mobile[i].y = gl4dmURand() * _h ;
    _mobile[i].vx = 100 + gl4dmSURand() * 500;
    _mobile[i].vy = 100 + gl4dmURand() * 200;
    _mobile[i].color[0] = gl4dmURand();
    _mobile[i].color[1] = gl4dmURand();
    _mobile[i].color[2] = gl4dmURand();
    _mobile[i].r = 10 + gl4dmURand() * 10;
  }
}

/*On enleve les frottements*/
void mobileMove(void) {
  const double G = -300;
  static Uint32 t0 = 0;
  Uint32 t = SDL_GetTicks();
  double dt = (t - t0) / 1000.0, d;
  t0 = t;
  int i, collision_bas;
  for(i = 0; i < _nb_mobiles; i++) {
    collision_bas = 0;
    _mobile[i].x += _mobile[i].vx * dt;
    _mobile[i].y += _mobile[i].vy * dt;
    if( (d = _mobile[i].x - _mobile[i].r) <= 0 ) {
      if(_mobile[i].vx < 0)
	_mobile[i].vx = -_mobile[i].vx;
      _mobile[i].vx -= d;
    }
    if( (d = _mobile[i].x + _mobile[i].r - (_w - 1)) >= 0 ) {
      if(_mobile[i].vx > 0)
	_mobile[i].vx = -_mobile[i].vx;
      _mobile[i].vx -= d;
    }
    if( (d = _mobile[i].y - (_mobile[i].r + 1)) <= 0 ) {
      if(_mobile[i].vy < 0)
	_mobile[i].vy = -_mobile[i].vy;
      _mobile[i].vy -= d;
      collision_bas = 1;
    }
    if(!collision_bas)
      _mobile[i].vy += G * dt;
  }
}
