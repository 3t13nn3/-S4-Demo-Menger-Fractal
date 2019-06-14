#version 330

/*VARIABLES ENVOYÉES COTÉ CPU*/
uniform sampler1D mobiles;
uniform sampler2D tex;
uniform int temps;
uniform float step;
uniform vec4 couleur;

in  vec2 vsoTexCoord;
out vec4 fragColor;

/*DESSINE UN VORONOI AVEC LES EFFET DEJA COMPRIS DANS LA FONCTION*/
/*ON CALCUL LE VORONOI A PARTIR DE NOTRE TEXTURE IMAGES ET DES MOBILES*/
vec4 voronoif(void) {
  float i, d, pp = step / 2.0, tmp;
  tmp = float(temps);
  /*ICI ON CHOISI LA COULEUR DES ELEMENTS DU VORONOI PAR LE BIAI DES MOBILES*/
  d = length(vsoTexCoord - texture(mobiles,pp +step).xy);
  for(i = 2.0 * step + step /2.0;i<1.0; i+=2.0*step){
    float nd = length(vsoTexCoord - texture(mobiles,i+step).xy);
    if(nd < d){
      pp = i;
      d = nd;
    }
  }
  
  /*EFFET TOURBILLON*/
  vec2 vecteur = vsoTexCoord - vec2(0.5);
  float distance = length(vecteur);
  float angle = atan(vecteur.y, vecteur.x);

  /*EFFET D'ASPIRATION*/
  if (tmp >= 171019-149000)
  /*ON FAIT tmp-(171019-149000) POUR SE METTRE EN POSITION INITIALE (TMP = 0)*/
    distance += abs(cos((tmp-(171019-149000))/20000)) * abs(sin((tmp-(171019-149000))/20000));

  /*TOURBILLON*/
  if (tmp >= 190000-149000){
    /*COMMENTER OU ENLEVER -tc POUR AVOIR UN VORONOI NORMAL AVEC LES COULEURS DE LA TEXTURE*/
    angle = 0.0;
  }
  if (tmp == 0.0)
    angle +=  0.0005;
  else
    angle +=  0.0005 * tmp / (1.0 + distance);
  vec2 tc = vec2(0.5) + vec2(-distance * cos(angle), distance * sin(angle));

  /*ON RETOURNE -tc C'EST A DIRE L'EFFET TOURBILLON, ET SELON LE TEMPS, ON RAJOUTE LE VORONOI (texture(mobiles,pp+step).xy)*/
  if (tmp >= 190000-149000){
    /*COMMENTER OU ENLEVER -tc POUR AVOIR UN VORONOI NORMAL AVEC LES COULEURS DE LA TEXTURE*/
    return fragColor = texture(tex, texture(mobiles,pp+step).xy - tc);
  }
  else
    return fragColor = texture(tex, - tc);
}

void main(void) {
  	fragColor = voronoif();  
}
