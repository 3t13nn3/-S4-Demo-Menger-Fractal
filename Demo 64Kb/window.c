/*DEMO ETIENNE PENAULT 17805598*/
/*DURÉE TOTALE 4MIN*/
/*./DEMO pour executer*/

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dp.h>
#include <GL4D/gl4duw_SDL2.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <fftw3.h>
#include <SDL_ttf.h>
#include "mobile.h"

/*PROTOTYPES DES FONCTIONS STATIQUES*/
static void init(void);
static void draw(void);
static void initAudio(const char * filename);
static void quit(void);
static void resize(int w, int h);
static void frameBufferGeneration();
static void resizeFrameBuffer();
static void loadTexture(GLuint, const char *);

/*PARTIE MUSIQUE*/
#define ECHANTILLONS 1024
/*HAUTEUR DE L'INSTANT T DE LA MUSIQUE, PLUS L'INDEX EST BAS, PLUS LA FREQUENCE EST BASSE*/
static Sint16 _hauteurs[ECHANTILLONS];
/*!\brief pointeur vers la musique chargée par SDL_Mixer */
static Mix_Music * _mmusic = NULL;
/*!\brief données entrées/sorties pour la lib fftw */
static fftw_complex * _in4fftw = NULL, * _out4fftw = NULL;
/*!\brief donnée à précalculée utile à la lib fftw */
static fftw_plan _plan4fftw = NULL;
/*VARIABLE UTILISER POUR DEPLACER DES ELEMENT PAR RAPPORT AUX AIGUS*/
static float moveAigue = 1;
/*FIN--PARTIE MUSIQUE*/

/*PARTIE VIDEO*/

/*ID DU SCREEN*/
static GLuint _screen = 0;
/*TAILLE DE LA FENETRE*/
static int _wW = 800, _wH = 600,_wW2 = 800, _wH2 = 600;
/*ID DU PROGRAMME GLSL*/
static GLuint _pId = 0, _pId2 = 0, _pId3 = 0;
/*framebuffer*/
static GLuint _frameBuffer = 0, _tamponRendu = 0;
/*ID DES FORMES ET DES TEXTURES*/
static GLuint _sphere = 0, _quad = 0, _cube = 0, _torus = 0, _tex = 0, _tId = 0, _texCredit =0;
/*NOMBRES DE MOBILES UTILISÉ POUR VORONOI AUX CREDIT*/
/*NE PAS EN METTRE ENORMÉMENT CAR GPU*/
static const int _nb_mobiles = 100;
/*JAUNE*/
static GLfloat maCouleur[] = {1, 0.8, 0, 0.2};

/*FIN--PARTIE VIDEO*/

int main(int argc, char ** argv) {
  srand(time(NULL));
  if(!gl4duwCreateWindow(argc, argv, "Demo Etienne", 0, 0, 
       _wW, _wH, GL4DW_RESIZABLE | GL4DW_SHOWN))
    return 1;
  init();
  atexit(quit);  
  gl4duwResizeFunc(resize);
  gl4duwDisplayFunc(draw);
  gl4duwMainLoop();
  return 0;
}

/*INITIALISES LES OPTIONS OPENGL A UTILISER*/
static void init(void) {
  glEnable(GL_DEPTH_TEST);
  /*ON COMMENCE PAR UTILSER LES SHADERS POUR L'EPONGE CAR C'EST LA PREMIÈRE PARTIE QUI EST LANCÉE*/
  _pId  = gl4duCreateProgram("<vs>shaders/sponge.vs", "<fs>shaders/sponge.fs", NULL);
  _pId2  = gl4duCreateProgram("<vs>shaders/voronoi.vs", "<fs>shaders/voronoi.fs", NULL);
  _pId3  = gl4duCreateProgram("<vs>shaders/credit.vs", "<fs>shaders/credit.fs", NULL);
  gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
  /*ON DONNE LA TAILLE POUR NOTRE FENETRE*/
  resize(_wW, _wH);
  /*ON GENERE NOS FORMES QUE L'ON VA UTILISER*/
  _cube = gl4dgGenCubef();
  _quad = gl4dgGenQuadf();
  _sphere = gl4dgGenSpheref(30, 30);
  _torus = gl4dgGenTorusf(300, 30, 0.1f);
  /*MUSIQUE*/
  _in4fftw   = fftw_malloc(ECHANTILLONS *  sizeof *_in4fftw);
  memset(_in4fftw, 0, ECHANTILLONS *  sizeof *_in4fftw);
  assert(_in4fftw);
  _out4fftw  = fftw_malloc(ECHANTILLONS * sizeof *_out4fftw);
  assert(_out4fftw);
  _plan4fftw = fftw_plan_dft_1d(ECHANTILLONS, _in4fftw, _out4fftw, FFTW_FORWARD, FFTW_ESTIMATE);
  assert(_plan4fftw);
  /* VIDEO*/
  glViewport(0, 0, _wW, _wH);
  _screen = gl4dpInitScreen();


  frameBufferGeneration();

  /*ON CHARGE L'IMAGE DANS NOTRE TEXTURE PRECEDEMMENT GENERÉE*/
  glGenTextures(1, &_tId);
  glBindTexture(GL_TEXTURE_1D, _tId);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, 1, 0, GL_RGBA, GL_FLOAT, NULL);
  glBindTexture(GL_TEXTURE_1D, 0);

  /*INITIALISATION DE NOTRE AUDIO*/
  initAudio("midori_-_snow_ride.it");
  
  /*GENERATION D'UN ID DE TEXTURE*/
  glGenTextures(1, &_texCredit);
  /*ON CHARGE L'IMAGE DANS NOTRE TEXTURE PRECEDEMMENT GENERÉE*/
  loadTexture(_texCredit, "credit.png");
}

/*GERE LES DIMENSIONS DE LA FENETRE*/
static void resize(int w, int h) {
  _wW  = w; _wH = h;
  glViewport(0, 0, _wW, _wH);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _wH / _wW, 0.5 * _wH / _wW, 1.0, 1000.0);
  gl4duBindMatrix("modelViewMatrix");
}

void frameBufferGeneration(){
  /*GENERATION DU FB*/
  glGenFramebuffers(1, &_frameBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);

  /*GENERATION D'UN ID DE TEXTURE*/
  glGenTextures(1, &_tex);
  glBindTexture(GL_TEXTURE_2D, _tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _wW, _wH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  // attache la texture au tampon d’image
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tex, 0);

  glGenRenderbuffers(1, &_tamponRendu);
  glBindRenderbuffer(GL_RENDERBUFFER, _tamponRendu); 
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _wW, _wH);  
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _tamponRendu);

  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("FB error.\n");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void resizeFrameBuffer(){
  if( _wW2 != _wW | _wH2 != _wH){
    printf("Recreer un framebuffer à la taille de la fenetre.\n");
    frameBufferGeneration();
  }
  _wW2 = _wW;
  _wH2 = _wH;
}

static void loadTexture(GLuint id, const char * filename) {
  SDL_Surface * t;
  glBindTexture(GL_TEXTURE_2D, id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  if( (t = IMG_Load(filename)) != NULL ) {
#ifdef __APPLE__
    int mode = t->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
#else
    int mode = t->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
#endif       
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, mode, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "can't open file %s : %s\n", filename, SDL_GetError());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
}

/*FONCTION PERMETTANT DE DESSINNER LA FRACTALE DE MENGER EN 3D*/
/*ELLE PREND EN ARGUMENT LE NOMBRE D'ITERATION (3 MAXIMUM, ET ENCORE, CELA ENTRAINE UNE CHUTE DE FPS)*/
/*LE TEMPS POUR LES ROTATIONS, UNE COULEUR AINSI QU'UN FLAG POUR SIGNALER A LA FONCTION D'APPLIQUER CETTE COULEUR*/
/*LE TYPE DE FORME AVEC LAQUELLE ON VA DESSINER L'EPONGE*/
/*UN BOOLEEN POUR SAVOIR SI ON VEUT DESSINER L'INTERSECTION DE LA FRACTALE*/
/*ET ENFIN, UN AUTRE BOLEEN AFIN DE SAVOIR SI ON VEUT METTRE EN ROTATION CHAQUE ELEMENT FORMANT L'EPONGE*/

/*LA FONCTION EST FAITES AU CAS PAR CAS ET NON DE MANIÈRE UNIVERSELLE CAR JE N'AI TOUT SIMPLEMENT PAS REUSSI A IMPLÉMENTER UNE TEL CHOSE*/
/*CEPENDANT, ETANT DU COTÉ CPU, NOUS NE POUVONS PAS ESPERER MIEUX QUE 3 ITERATIONS*/
void drawSponge(int it, int temps, GLfloat inverseColor[], int appliedChangeColor,GLuint shape, int cross, int rotate){
  glUseProgram(_pId);
  /*ON DEFINIT UNE VARIABLE OU NOUS ALLONS STOCKER NOTRE CONDITION*/
  int condition;
  /*ON GERE LE CAS OU L'ITERATION EST DE 0 (ON DESSINE JUSTE LA FORME)*/
  if (it == 0){
    gl4duPushMatrix(); 
    gl4duSendMatrices();
    if (appliedChangeColor == 0)
      glUniform4fv(glGetUniformLocation(_pId, "couleur"), 1, maCouleur);
    else
      glUniform4fv(glGetUniformLocation(_pId, "couleur"), 1, inverseColor);
    gl4duPopMatrix(); 
    gl4dgDraw(shape);
  }else{
    /*ON CALCUL LE NOMBRE TOTRANSLATE AFIN DE L'UTILISER POUR NOS FUTURE TRANSLATIONS*/
    double toTranslate = pow (3,it) - 1;
    /*ON SE POSITIONNE AUX BON ENDROIT AVANT DE DESSINER, PAR RAPPORT A L'ITERATION DE LA FONCTION*/
    gl4duTranslatef(0,0,(2*it));
    gl4duTranslatef(0,0,toTranslate - ((it-1)*12));
    gl4duTranslatef(-toTranslate,-toTranslate,-toTranslate + ((it-1)*12));
    /*ON CALCUL LE NOMBRE DE FORME A DESSINER*/
    int sum=1;
    for(int i = 1; i<it; i++){
      sum*=27;
    }
    /*ICI NOUS ALLONS PARCOURIR 3 BOUCLES POUR 3 DIMENSONS*/
    /*LES FORMES SONT NUMÉROTÉ DE LA SORTE*/
    /*7  8  9*/
    /*4  5  6*/
    /*1  2  3*/

    /*16 17 18*/
    /*13 14 15*/
    /*10 11 12*/

    /*25 26 27*/
    /*22 23 24*/
    /*19 20 21*/
    for(int j=0; j<sum; j++){
      for(int x =0; x < 3; x++){
        for(int y =0; y < 3; y++){
          for(int z =0; z < 3; z++){
            /*ON VERIFIE SI NOTRE BOOLEEN CROSS EST VRAI OU NON*/
            /*POUR SAVOIR SI NOUS DEVONT FAIRE LA NÉGATION DE LA CONDITION AFIN D'AVOIR SEULEMENT LA PARTIE NORMALEMENT VIDE DE DESSINÉ*/
            if(cross){
            condition = !(((!(x==1 && y==1 && z==0) && !(x==1 && y==1 && z==1) && !(x==1 && y==1 && z==2) 
                        && !(x==1 && y==2 && z==1) && !(x==1 && y==0 && z==1) &&!(x==0 && y==1 && z==1) && !(x==2 && y==1 && z==1))
                        && j%27 != 4 && j%27 != 10 && j%27 != 12 && j%27 != 13 && j%27 != 14 && j%27 != 16 && j%27 != 22
                        && !(j>= 4*27 && j< 5*27) && !(j>= 10*27 && j< 11*27) && !(j>= 12*27 && j< 13*27) && !(j>= 13*27 && j< 14*27) && !(j>= 14*27 && j< 15*27) && !(j>= 16*27 && j< 17*27) && !(j>= 22*27 && j< 23*27)));
            }else{
            condition = ((!(x==1 && y==1 && z==0) && !(x==1 && y==1 && z==1) && !(x==1 && y==1 && z==2) 
                        && !(x==1 && y==2 && z==1) && !(x==1 && y==0 && z==1) &&!(x==0 && y==1 && z==1) && !(x==2 && y==1 && z==1))
                        && j%27 != 4 && j%27 != 10 && j%27 != 12 && j%27 != 13 && j%27 != 14 && j%27 != 16 && j%27 != 22
                        && !(j>= 4*27 && j< 5*27) && !(j>= 10*27 && j< 11*27) && !(j>= 12*27 && j< 13*27) && !(j>= 13*27 && j< 14*27) && !(j>= 14*27 && j< 15*27) && !(j>= 16*27 && j< 17*27) && !(j>= 22*27 && j< 23*27));
            }
            /*ON APPLIQUE LA CONDITION PRECEDEMMENT SELECTIONNÉE*/
            if(condition){
              /*ON FAIS UNE ROTATION SI LE BOOLÉEN EST VRAI*/
              if(rotate)
                gl4duRotatef(temps/5,1,1,1);
              /*ON CHANGE LE SCALE A DES MOMENTS DE LA DEMO*/            
              if (temps >= 85500)
                gl4duScalef(0.5,0.5,0.5);
              if (temps >= 117525 && it != 1)
                gl4duScalef(0.25,0.25,0.25);
              gl4duSendMatrices();
              
              /*GESTION DES COULEURS SI AUCUNE COULEUR EST SELECTIONNÉ*/
              /*ON FAIT JOUER L'ALÉATOIRE POUR UN RENDU PLUS FANCY*/
              if (appliedChangeColor == 0){
                GLfloat couleur[] = {(float)rand()/RAND_MAX,0,0,1};
                if (it == 1){
                  if (temps <= 10791){
                    couleur[0] = 0.25;
                    couleur[1] = 0.7;
                    couleur[3] = 0.5;
                  }
                  else{
                    couleur[0] = (float)rand()/RAND_MAX/1.25;
                    couleur[1] = (float)rand()/RAND_MAX;
                  }
                  glUniform4fv(glGetUniformLocation(_pId, "couleur"), 1, couleur);
                }else if (it == 2){
                  if (temps <= 10791){
                    couleur[0] =0.75;
                  }
                  else if (temps >=64156 && temps </*90822*/96144){
                    couleur[0] =0.75;
                    couleur[1] = (float)rand()/RAND_MAX/2;
                  }
                  else if ((temps < 64156) && temps >= 10791){
                    couleur[1] = (float)rand()/RAND_MAX/8;
                    couleur[2] = (float)rand()/RAND_MAX/8;
                  }
                  else if (temps >= 85500){
                    couleur[1] = (float)rand()/RAND_MAX;
                    couleur[2] = (float)rand()/RAND_MAX;
                  }
                  glUniform4fv(glGetUniformLocation(_pId, "couleur"), 1, couleur);
                }else{
                  /*SINON ON CHOISI LA COULEUR PAR DEFAUT*/
                  glUniform4fv(glGetUniformLocation(_pId, "couleur"), 1, maCouleur);
                }
              }else{
                /*SINON ON CHOISIT LA COULEUR PASSÉ EN ARGUMENT*/
                glUniform4fv(glGetUniformLocation(_pId, "couleur"), 1, inverseColor);
              }
              /*ENFIN ON DESSINE NOTRE FORME A LA POSITION DONNER AVANT D'APPLIQUER LA TRANSLATION POUR LA FUTURE FORME*/
              gl4dgDraw(shape);
              if(rotate)
                gl4duRotatef(-temps/5,1,1,1);
              if (temps >= 85500)
                gl4duScalef(2,2,2);
              if (temps >= 117525 && it != 1)
                gl4duScalef(4,4,4);                
            }
            /*ENFIN ON SE REPOSITIONNE POUR LA PROCHAINE FORME A DESSINER*/
            gl4duTranslatef(0, 0, 2);
          }
          gl4duTranslatef(0, 0, -6);
          gl4duTranslatef(0, 2, 0);
        }
        gl4duTranslatef(0,-6, 0);
        gl4duTranslatef(2, 0, 0);
      }
      if (j%3 == 2)
        gl4duTranslatef(-18, 6, 0);
      
      if (j%9 == 8)
        gl4duTranslatef(0, -18, -6);
      if(j%27 == 26)
        gl4duTranslatef(18, 0, 18);
      if(j%81 == 80)
        gl4duTranslatef(-18*3, 18, 0);
      if(j%243 == 242)
        gl4duTranslatef(0, -18*3, -18);
    }
    /*ENFIN, ON SE REPLACE A NOTRE POSITION INITIALE*/
    if (it == 3)
      gl4duTranslatef(toTranslate, toTranslate, toTranslate + 10);
    if (it == 1){
      //2
      gl4duTranslatef(-toTranslate*2, toTranslate, -toTranslate);
    }
    if (it == 2){
      //8
      gl4duTranslatef(-toTranslate -2, toTranslate, -4);
    }

  }

}

/*C'EST ICI QU'ON VA ORCHESTRER LES DEPLACEMENTS, SELON LE TEMPS, DE LA MUSIQUE*/
/*DE LA PARTIE DE NOTRE DEMO SUR L'EPONGE DE MENGER*/
void demoEponge(int temps){
  /*VITESSE DE DEPLACEMENT DE NOS SPHERES AUTOUR L'EPONGE*/
  static GLfloat vitesse = 0;
  /*VITESSE UTILISÉE POUR NOS ROTATIONS*/
  static GLfloat vitesseRot = 0;
  /*VITESSE DU RAPPROCHEMENT DE L'EPONGE QUE NOUS POUVONS VOIR AU TOUT DEBUT ET A LA TOUTE FIN DE LA DEMO*/
  static GLfloat rapproche = 1;
  /*PERMET DE SAVOIR SI ON DOIT AUGMENTER OU DIMINUER NOTRE VARIABLE CHANGECOLOR*/
  static int colorState =1;
  /*VARIABLE COMPRISE ENTRE 0 ET 1 AFIN DE DETERMINER LA COULEUR DE FOND AINSI QUE DES SPHERES*/
  static GLfloat changeColor = 0;

  glEnable(GL_DEPTH_TEST);
  /*ON CLEAR LE FOND A PARTIR DE NOTRE VARIABLE CHANGECOLOR*/
  glClearColor(changeColor/6,0.05 + changeColor/4,changeColor/3, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (colorState)
    changeColor+=0.001;
  else if (!colorState){
    changeColor-=0.001;
  }  

  
  /*iCI NOUS ALLONS ADDITIONNER UN CHNATILLONS DE FREQUENCES TIRANT SUR LES AIGUS*/
  /*SI LA SOMME DE TOUTES CES FREQUENCES DEPASSE LE SEUIL FIXÉ (4000), ALORS MOVEAIGU*/
  /*NOTRE VARIABLE POUR SE DEPACER EN FONCTION DES AIGU CROIT, SINON ELLE DECROIT*/
  /*NOUS AVOUS TOUT DE MÊME BORNÉ LA VARIABLE POUR PAR QU'ELLE NE DEVIENNE TROP PETITE OU TROP GROSSE*/
  int sommeAigue;
  for(int i = 200; i< 900; i++)
    sommeAigue += _hauteurs[i];
  
  if (sommeAigue >= 6000 && moveAigue <= 3)
    moveAigue*=1.04; 
  else
    moveAigue/=1.01;

  if (moveAigue < 1)
    moveAigue = 1;


  if (temps >= 42817 && temps <= 128854)
    rapproche = 0;

  /*ON DEPLACE NOS OBJET SELON RAPPROCHE*/
  gl4duTranslatef(0, 0, -250+rapproche);

  /*COMME PARTOUT DANS CETTE FONCTION, ON VA CIBLÉ CERTAIN INTERVAL DE TEMPS POUR AFFICHER CERTAINES CHOSES*/
  /*PAR EXEMPLES DES FORMES DIFFERENTES, DES ROTATIONS, UN NOMBRE D'ITÉRATION DIFFERENT*/
  if (temps >= 42817 && temps < 128854){
    gl4duScalef(0.5,0.5,0.5);
    if (temps >= 42817)
      gl4duRotatef(-vitesseRot*60, 1.0, 1.0, 1.0);
    drawSponge(0,temps,NULL,0,_cube,0,0);
    if (temps >= 42817)
      gl4duRotatef(vitesseRot*30, 1.0, 1.0, 1.0);
    gl4duScalef(2,2,2);
    
    if (temps >= 42817)
      gl4duRotatef(vitesseRot*30, 0.0, 0.0, 1.0);
    gl4duTranslatef(0,0,-4);
    drawSponge(1,temps,NULL,0,_sphere,0,0);
    gl4duTranslatef(0,0,4);
    if (temps >= 42817)
      gl4duRotatef(-vitesseRot*30, 0.0, 0.0, 1.0);
  }

  
  if (temps >= 42817){
    gl4duRotatef(vitesseRot*5, 1.0, 0.0, 0.0);
    /*A PARTIR DE 42 SECONDES, NOUS ALLONS FAIRE BOUGER NOTRE EPONGE D'ITERATION 2 SELON LES AIGU DE LA MUSIQUE !*/
    gl4duScalef(moveAigue,moveAigue,moveAigue);
  }
  gl4duScalef(2,2,2);
  if(temps >=106972 && temps <= 117525)
    drawSponge(2,temps,NULL,0,_torus,0,1);
  else if (temps >=64156 && temps < 85500)
    drawSponge(2,temps,NULL,0,_cube,1,0);
  else if (temps >=85500 && temps </*90822*/96144)
    drawSponge(2,temps,NULL,0,_cube,1,1);
  else if (temps >=96144 && temps < 106972)
    drawSponge(2,temps,NULL,0,_cube,0,1);
  else
    drawSponge(2,temps,NULL,0,_cube,0,0);
  
  gl4duScalef(0.5,0.5,0.5); 
  if (temps >= 42817){
    gl4duRotatef(-vitesseRot*5, 1.0, 0.0, 0.0);
    gl4duScalef(-moveAigue,-moveAigue,-moveAigue);
  }else
    gl4duScalef(-1,-1,-1);

  /*C'EST ICI QUE NOUS ALLONS DECIDER A QUEL MOMENT NOUS ALLONS METTRE L'EFFET OU IL Y A 2 EPONGES ET UNE DES DEUX SE RAPPROCHE DE PLUS EN PLUS DE NOUS*/
  if (temps < 42817 || temps >= 128854){
    gl4duTranslatef(0, 0, -100+rapproche);
    
    if (temps >= 42817){
      gl4duRotatef(vitesseRot*5, 1.0, 0.0, 0.0);
      gl4duScalef(moveAigue,moveAigue,moveAigue);
    }
    gl4duScalef(2,2,2);
    drawSponge(2,temps,NULL,0,_cube,0,0);
    gl4duScalef(0.5,0.5,0.5); 
    if (temps >= 42817){
      gl4duRotatef(-vitesseRot*5, 1.0, 0.0, 0.0);
      gl4duScalef(-moveAigue,-moveAigue,-moveAigue);
    }else
      gl4duScalef(-1,-1,-1);
  }

  /*ICI LA VARIABLE INVERSECOLOR A POUR BUT DE SE DIFFERENCIER DE LA COULEUR DU CLEAR*/
  /*ELLE SERA ATTRIBUÉE A NOS SPHERES QUI GRAVITENT AU TOUR DE L'EPONGE*/
  GLfloat inverseColor[] = {changeColor,0.8,changeColor/2, 1.0f,1};
  float nbPoint = 5;
  gl4duRotatef(vitesse/2, 1.0, 1.0, 1.0);
  /*CETTE BOUCLE VA PERMETRE DE PLACER UN NOMBRE DE POINT, EN CERCLE*/
  /*AU TOUR DE NOTRE EPONGE*/
  /*NOUS ALLONS DESSINER TROIS SPHERES A CHAQUE TOUR DE BOUCLE, UNE SUR CHAQUE AXE (X,Y,Z)*/
  for (int i = 0; i <(nbPoint); i++){

    if (temps >= 21393){
      gl4duRotatef(vitesse*100, 1.0, 0.0, 0.0);
    }else{
      gl4duRotatef(vitesse*10, 1.0, 0.0, 0.0);
    }
    gl4duTranslatef(0,(cos(i*(nbPoint)) * 40),sin(i*nbPoint) * 40);
    gl4duSendMatrices();
    drawSponge(0,temps,inverseColor,1,_sphere,0,0);
    gl4duTranslatef(0,-(cos(i*nbPoint) * 40),-(sin(i*nbPoint) * 40) );
    if (temps >= 21393){
      gl4duRotatef(-vitesse*100, 1.0, 0.0, 0.0);
    }else{
      gl4duRotatef(-vitesse*10, 1.0, 0.0, 0.0);
    }

    

    if (temps >= 21393){
      gl4duRotatef(vitesse*100, 0.0, 0.0, 1.0);
    }else{
      gl4duRotatef(vitesse*10, 0.0, 0.0, 1.0);
    }
    gl4duTranslatef((cos(i*(nbPoint)) * 40), sin(i*nbPoint) * 40,0);
    drawSponge(0,temps,inverseColor,1,_sphere,0,0);
    gl4duTranslatef(-(cos(i*nbPoint) * 40), -(sin(i*nbPoint) * 40),0);
    if (temps >= 21393){
      gl4duRotatef(-vitesse*100, 0.0, 0.0, 1.0);
    }else{
      gl4duRotatef(-vitesse*10, 0.0, 0.0, 1.0);
    }



    if (temps >= 21393){
      gl4duRotatef(vitesse*100, 0.0, 1.0, 0.0);
    }else{
      gl4duRotatef(vitesse*10, 0.0, 1.0, 0.0);
    }
    gl4duTranslatef((cos(i*(nbPoint)) * 40),0, sin(i*nbPoint) * 40);
    drawSponge(0,temps,inverseColor,1,_sphere,0,0);
    gl4duTranslatef(-(cos(i*nbPoint) * 40), 0,-(sin(i*nbPoint) * 40));

    if (temps >= 21393){
      gl4duRotatef(-vitesse*100, 0.0, 1.0, 0.0);
    }else{
      gl4duRotatef(-vitesse*10, 0.0, 1.0, 0.0);
    }
  }

  /*ENFIN A LA FIN DE NOTRE FONCTION, NOUS ALLONS GERER LES INCREMENTATIONS ET LES DECREMENTATIONS DE CERTAINES VARIABLES*/
  vitesse+=0.05;
  if (temps >= 42817)
    vitesseRot+=0.05;

  if (changeColor >= 1){ 
    colorState = 0;
  }else if (changeColor <= 0){ 
    colorState = 1;
  }

  
  if (temps > 21393 && temps < 117525)
    rapproche+=2.5;
  else if (temps > 117525)
    rapproche +=6.9;
  else
    rapproche += 0.5;
  if (rapproche > 300){
    rapproche =1;
  }
}

/*CETTE DEMO A POUR BUT DE PRESENTER LES CREDITS D'UNE MANIÈRE ORIGINALE*/
/*CETTE PARTIE REPOSE SUR LE COTÉ GPU PRINCIPALEMENT*/
/*ON CHARGE UNE IMAGE, ON L'AFFICHE PAR LE BIAI DE LA TEXTURE*/
/*ET ENFIN ON APPLIQUE DES MODIFICATION A NOTRE TEXTURE DU COTÉ GLSL*/
/*COMME PAR EXEMPLE UN VORONOI PLUTOT ORIGINAL*/
void demoVoronoi(int temps){
  glUseProgram(_pId2);
  static int init = 0;
  
  if (temps >=187000){
    if (init == 0){
      init = 1;
      /*ON INITIALISE NOS MOBILES UNE FOIS QUE NOUS ALLONS LANCER VORONOI*/
      mobileInit(_nb_mobiles, _wW, _wH);
    }
  }
  glActiveTexture(GL_TEXTURE0);
  /*ON ENVOIE NOTRE TEXTURE AVEC L'IMAGE AU SHADERS*/
  glUniform1i(glGetUniformLocation(_pId2, "tex"), 0);
  /*ON PASSE LE TEMPS AU PROGRAMME GLSL AFIN D'INTERAGIR AVEC LUI COTÉ CPU*/
  /*COMME PAR EXEMPLE L'EFFET DE TOURBILLON OU ENCORE L'EFFET DE SIMILIE VITESSE*/
  if ((temps >= 149000)){
    glUniform1i(glGetUniformLocation(_pId2, "temps"), temps-149000);
  }else{
    glUniform1i(glGetUniformLocation(_pId2, "temps"), 0);
  }
  glBindTexture(GL_TEXTURE_2D, _tex);

  /*GESTIONS DES MOBILES POUR LE VORONOI*/
  GLfloat * f = malloc(_nb_mobiles * 8 * sizeof *f), step = 1.0 / (_nb_mobiles * 2);
  assert(f);
  glDisable(GL_DEPTH_TEST);

  glClear(GL_COLOR_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _tId);
  mobileMove();
  mobile2texture(f);
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, 2 * _nb_mobiles, 0, GL_RGBA, GL_FLOAT, f);
  free(f);
  glUniform1i(glGetUniformLocation(_pId2, "mobiles"), 1);
  glUniform1f(glGetUniformLocation(_pId2, "step"), step);

  gl4dgDraw(_quad);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_1D, 0);  
}

void demoCredit(){
  glUseProgram(_pId3);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(glGetUniformLocation(_pId, "tex"), _texCredit);
  glBindTexture(GL_TEXTURE_2D, _texCredit);
  gl4dgDraw(_quad);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_1D, 0); 

}

/*FONCTION D'AFFICHAGE PRINCIPAL*/
static void draw(void) {

  resizeFrameBuffer();
  /*ON RECUPERE LE TEMPS AU MOMENT T*/
  int temps = SDL_GetTicks();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
  /*ICI, PAR RAPPORT AU TEMPS, NOUS ALLONS DESSINER SOIT LA DEMO DE L'EPONGE, OU CELLE DES CREDITS*/
  /*INITIALEMENT, CE SONT LES SHADERS DE L'EPONGE QUI SONT CHARGÉ, CE QUI ENGENDRE DONC UN CHANGEMENT DE SHADER*/
  /*AU COURS DU PROGRAMME, ON REAFECTE L'ID DU PROCESSUS DU PROGRAMME GLSL*/  

  if ((temps < 149000)){
    demoEponge(temps);
  }
  else if ((temps >= 149000) && (temps < 207000)){
      
      glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);

      demoEponge(temps);
      
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      demoVoronoi(temps);

  }
  else
    demoCredit();
  if (temps >= 215000)
    exit(0);
}

/*!\brief appelée au moment de sortir du programme (atexit), libère les éléments utilisés */
static void quit(void) {
  if(_mmusic) {
    if(Mix_PlayingMusic())
      Mix_HaltMusic();
    Mix_FreeMusic(_mmusic);
    _mmusic = NULL;
  }
  Mix_CloseAudio();
  Mix_Quit();
  if(_screen) {
    gl4dpSetScreen(_screen);
    gl4dpDeleteScreen();
    _screen = 0;
  }
  if(_plan4fftw) {
    fftw_destroy_plan(_plan4fftw);
    _plan4fftw = NULL;
  }
  if(_in4fftw) {
    fftw_free(_in4fftw); 
    _in4fftw = NULL;
  }
  if(_out4fftw) {
    fftw_free(_out4fftw); 
    _out4fftw = NULL;
  }
  gl4duClean(GL4DU_ALL);
}

/*!\brief appelée quand l'audio est joué et met dans \a stream les
 * données audio de longueur \a len */
static void mixCallback(void *udata, Uint8 *stream, int len) {
  if(_plan4fftw) {
    int i, j, l = MIN(len >> 1, ECHANTILLONS);
    Sint16 *d = (Sint16 *)stream;
    for(i = 0; i < l; i++)
      _in4fftw[i][0] = d[i] / ((1 << 15) - 1.0);
    fftw_execute(_plan4fftw);
    for(i = 0; i < l >> 2; i++) {
      _hauteurs[4 * i] = (int)(sqrt(_out4fftw[i][0] * _out4fftw[i][0] + _out4fftw[i][1] * _out4fftw[i][1]) * exp(2.0 * i / (double)(l / 4.0)));
      for(j = 1; j < 4; j++)
  _hauteurs[4 * i + j] = MIN(_hauteurs[4 * i], 255);
    }
  }
}

/*!\brief charge le fichier audio avec les bonnes options */
static void initAudio(const char * filename) {
#if defined(__APPLE__)
  int mult = 1;
#else
  int mult = 2;
#endif
  int mixFlags = MIX_INIT_MP3, res;
  res = Mix_Init(mixFlags);
  if( (res & mixFlags) != mixFlags ) {
    fprintf(stderr, "Mix_Init: Erreur lors de l'initialisation de la bibliothèque SDL_Mixer\n");
    fprintf(stderr, "Mix_Init: %s\n", Mix_GetError());
    //exit(3); commenté car ne réagit correctement sur toutes les architectures
  }
  if(Mix_OpenAudio(44100, AUDIO_S16LSB, 1, mult * ECHANTILLONS) < 0)
    exit(4);  
  if(!(_mmusic = Mix_LoadMUS(filename))) {
    fprintf(stderr, "Erreur lors du Mix_LoadMUS: %s\n", Mix_GetError());
    exit(5);
  }
  Mix_SetPostMix(mixCallback, NULL);
  if(!Mix_PlayingMusic()){
    Mix_PlayMusic(_mmusic, 2);
  }
}
