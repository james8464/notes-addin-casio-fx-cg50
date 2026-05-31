#include <stdio.h>
#include <fxcg/keyboard.h>
#include "giacPCH.h"
#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
extern "C" {
#include <fxcg/rtc.h>
}
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alloca.h>
#include <ctype.h>

#include "history.h"
#include "dConsole.h"
#include "main.h"
  //#include "memmgr.h"
#include "catalogGUI.hpp"
#include "fileGUI.hpp"
#include "textGUI.hpp"
#include "graphicsProvider.hpp"
#include "constantsProvider.hpp"
#include "kdisplay.h"
#include "input_lexer.h"
#include "input_parser.h"


#define STATUS_AREA_PX 24

#define CAT_CATEGORY_ALL 0
#define CAT_CATEGORY_ALGEBRA 1
#define CAT_CATEGORY_LINALG 2
#define CAT_CATEGORY_CALCULUS 3
#define CAT_CATEGORY_ARIT 4
#define CAT_CATEGORY_COMPLEXNUM 5
#define CAT_CATEGORY_PLOT 6
#define CAT_CATEGORY_POLYNOMIAL 7
#define CAT_CATEGORY_PROBA 8
#define CAT_CATEGORY_PROGCMD 9
#define CAT_CATEGORY_REAL 10
#define CAT_CATEGORY_SOLVE 11
#define CAT_CATEGORY_STATS 12
#define CAT_CATEGORY_TRIG 13
#define CAT_CATEGORY_OPTIONS 14
#define CAT_CATEGORY_LIST 15
#define CAT_CATEGORY_MATRIX 16
#define CAT_CATEGORY_PROG 17
#define CAT_CATEGORY_SOFUS 18
#define CAT_CATEGORY_PHYS 19
#define CAT_CATEGORY_UNIT 20
#define CAT_CATEGORY_2D 21
#define CAT_CATEGORY_3D 22
#define CAT_CATEGORY_LOGO 23 // should be the last one
#define XCAS_ONLY 0x80000000

using namespace giac;
extern giac::context * contextptr;

void reset_alpha(){
  SetSetupSetting( (unsigned int)0x14, 0);	
  DisplayStatusArea();
}

const char ram_filename[]="\\\\fls0\\khicas90.8c2";
int lang=1;
const catalogFunc completeCat[] = { // list of all functions (including some not in any category)
    {" boucle for (pour)", "for ", "Boucle definie pour un indice variant entre 2 valeurs fixees", "#\nfor ", 0, CAT_CATEGORY_PROG},
    {" boucle liste", "for in", "Boucle sur tous les elements d'une liste.", "#\nfor in", 0, CAT_CATEGORY_PROG},
    {" boucle while (tantque)", "while ", "Boucle indefinie tantque.", "#\nwhile ", 0, CAT_CATEGORY_PROG},
    {" test si alors", "if ", "Test", "#\nif ", 0, CAT_CATEGORY_PROG},
    {" test sinon", "else ", "Clause fausse du test", 0, 0, CAT_CATEGORY_PROG},
    {" fonction def.", "f(x):=", "Definition de fonction.", "#\nf(x):=", 0, CAT_CATEGORY_PROG},
    {" local j,k;", "local ", "Declaration de variables locales Xcas", 0, 0, CAT_CATEGORY_PROG | XCAS_ONLY},
    {" range(a,b)", "in range(", "Dans l'intervalle [a,b[ (a inclus, b exclus)", "# in range(1,10)", 0, CAT_CATEGORY_PROG},
    {" return res;", "return ", "return ou retourne quitte la fonction et renvoie le resultat res", 0, 0, CAT_CATEGORY_PROG},
    //{" edit list ", "list(", "Assistant creation de liste.", 0, 0, CAT_CATEGORY_LIST},
    //{" edit matrix ", "matrix(", "Assistant creation de matrice.", 0, 0, CAT_CATEGORY_MATRIX },
    {" mksa(x)", 0, "Conversion en unites MKSA", 0, 0, CAT_CATEGORY_PHYS | (CAT_CATEGORY_UNIT << 8)  | XCAS_ONLY},
    {" ufactor(a,b)", 0, "Factorise l'unite b dans a", "100_J,1_kW", 0, CAT_CATEGORY_PHYS | (CAT_CATEGORY_UNIT << 8) | XCAS_ONLY},
    {" usimplify(a)", 0, "Simplifie l'unite dans a", "100_l/10_cm^2", 0, CAT_CATEGORY_PHYS | (CAT_CATEGORY_UNIT << 8) | XCAS_ONLY},
    //{"fonction def Xcas", "fonction f(x) local y;   ffonction:;", "Definition de fonction.", "#fonction f(x) local y; y:=x^2; return y; ffonction:;", 0, CAT_CATEGORY_PROG},
    {"!", "!", "Non logique (prefixe) ou factorielle de n (suffixe).", "#7!", "#!b", CAT_CATEGORY_PROGCMD},
    {"#", "#", "Commentaire Python, en Xcas taper //.", 0, 0, CAT_CATEGORY_PROG},
    {"%", "%", "a % b signifie a modulo b", 0, 0, CAT_CATEGORY_ARIT | (CAT_CATEGORY_PROGCMD << 8)},
    {"&", "&", "Et logique ou +", "#1&2", 0, CAT_CATEGORY_PROGCMD},
    {":=", ":=", "Affectation vers la gauche (inverse de =>).", "#a:=3", 0, CAT_CATEGORY_PROGCMD|(CAT_CATEGORY_SOFUS<<8)|XCAS_ONLY},
    {"<", "<", "Inferieur strict. Raccourci SHIFT F2", 0, 0, CAT_CATEGORY_PROGCMD},
    {"=>", "=>", "Affectation vers la droite ou conversion en (touche ->). Par exemple 5=>a ou x^4-1=>* ou (x+1)^2=>+ ou sin(x)^2=>cos.", "#5=>a", "#15_m=>_cm", CAT_CATEGORY_PROGCMD | (CAT_CATEGORY_PHYS <<8) | (CAT_CATEGORY_UNIT << 16) | XCAS_ONLY},
    {">", ">", "Superieur strict. Raccourci F2.", 0, 0, CAT_CATEGORY_PROGCMD},
    {"\\", "\\", "Caractere \\", 0, 0, CAT_CATEGORY_PROGCMD},
    {"_", "_", "Caractere _. Prefixe d'unites.", 0, 0, CAT_CATEGORY_PROGCMD},
    {"_(km/h)", "_(km/h)", "Vitesse en kilometre/heure", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_(m/s)", "_(m/s)", "Vitesse en metre/seconde", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_(m/s^2)", "_(m/s^2)", "Acceleration en metre par seconde au carre", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_(m^2/s)", "_(m^2/s)", "Viscosite", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_A", 0, "Intensite electrique en Ampere", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_Bq", 0, "Radioactivite: Becquerel", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_C", 0, "Charge electrique en Coulomb", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_Ci", 0, "Radioactivite: Curie", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_F", 0, "Farad", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_F_", 0, "constante de Faraday (charge globale d'une mole de charges élémentaires).", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_G_", 0, "constante de gravitation universelle. Force=_G_*m1*m2/r^2", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_H", 0, "Henry", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_Hz", 0, "Hertz", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_J", 0, "Energie en Joule=kg*m^2/s^2", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_K", 0, "Temperature en Kelvin", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_Kcal", 0, "Energie en kilo-calorier", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_MeV", 0, "Energie en mega-electron-Volt", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_N", 0, "Force en Newton=kg*m/s^2", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_NA_", 0, "Avogadro", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_Ohm", 0, "Resistance electrique en Ohm", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_PSun_", 0, "puissance du Soleil", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_Pa", 0, "Pression en Pascal=kg/m/s^2", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_REarth_", 0, "Rayon de la Terre", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_RSun_", 0, "rayon du Soleil", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_R_", 0, "constante des gaz (de Boltzmann par mole)", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_S", 0, "", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_StdP_", 0, "Pression standard (au niveau de la mer)", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_StdT_", 0, "temperature standard (0 degre Celsius exprimes en Kelvins)", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_Sv", 0, "Radioactivite: Sievert", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_T", 0, "Tesla", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_V", 0, "Tension electrique en Volt", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_Vm_", 0, "Volume molaire", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_W", 0, "Puissance en Watt=kg*m^2/s^3", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_Wb", 0, "Weber", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_alpha_", 0, "constante de structure fine", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_c_", 0, "vitesse de la lumiere", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_cd", 0, "Luminosite en candela", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_cdf", "_cdf", "Suffixe pour obtenir une distribution cumulee. Taper F2 pour la distribution cumulee inverse.", "#_icdf", 0, CAT_CATEGORY_PROBA|XCAS_ONLY},
    {"_d", 0, "Temps: jour", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_deg", 0, "Angle en degres", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_eV", 0, "Energie en electron-Volt", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_epsilon0_", 0, "permittivite du vide", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_ft", 0, "Longueur en pieds", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_g_", 0, "gravite au sol", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_grad", 0, "Angle en grades", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_h", 0, "Heure", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_h_", 0, "constante de Planck", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_ha", 0, "Aire en hectare", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_hbar_", 0, "constante de Planck/(2*pi)", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_inch", 0, "Longueur en pouces", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_kWh", 0, "Energie en kWh", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_k_", 0, "constante de Boltzmann", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_kg", 0, "Masse en kilogramme", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_l", 0, "Volume en litre", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_m", 0, "Longueur en metre", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_mEarth_", 0, "masse de la Terre", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_m^2", 0, "Aire en m^2", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_m^3", 0, "Volume en m^3", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_me_", 0, "masse electron", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_miUS", 0, "Longueur en miles US", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_mn", 0, "Temps: minute", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_mp_", 0, "masse proton", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_mpme_", 0, "ratio de masse proton/electron", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_mu0_", 0, "permeabilite du vide", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_phi_", 0, "quantum flux magnetique", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_qe_", 0, "charge de l'electron", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_qme_", 0, "_q_/_me_", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_rad", 0, "Angle en radians", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_rem", 0, "Radioactivite: rem", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_s", 0, "Temps: seconde", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_sd_", 0, "Jour sideral", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_syr_", 0, "Annee siderale", 0, 0, CAT_CATEGORY_PHYS | XCAS_ONLY},
    {"_tr", 0, "Angle en tours", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
    {"_yd", 0, "Longueur en yards", 0, 0, CAT_CATEGORY_UNIT | XCAS_ONLY},
  {"a and b", " and ", "Et logique", 0, 0, CAT_CATEGORY_PROGCMD},
  {"a or b", " or ", "Ou logique", 0, 0, CAT_CATEGORY_PROGCMD},
  {"abs(x)", 0, "Valeur absolue, module ou norme de x", "-3", "[1,2,3]", CAT_CATEGORY_COMPLEXNUM | (CAT_CATEGORY_REAL<<8)},
    {"aire(objet)", 0, "Aire algebrique", "cercle(0,1)", "triangle(-1,1+i,3)", CAT_CATEGORY_2D  },
  {"append", 0, "Ajoute un element en fin de liste l","#l.append(x)", 0, CAT_CATEGORY_LIST},
  {"approx(x)", 0, "Valeur approchee de x. Raccourci S-D", "pi", 0, CAT_CATEGORY_REAL},
  {"arg(z)", 0, "Argument du complexe z.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"axes", "axes", "Axes visibles ou non axes=1 ou 0", "#axes=0", "#axes=1", CAT_CATEGORY_PROGCMD << 8},
    {"barycentre([pnt,coeff],...)", 0, "Barycentre d'une sequnence de [points,coefficients]. Utiliser isobarycenter si tous les coefficients sont egaux.", "[1,1],[i,1],[2,3]", 0, CAT_CATEGORY_2D | (CAT_CATEGORY_3D << 8) },
    {"bissectrice(A,B,C)", 0, "Bissectrice de l'angle AB,AC", "1,i,2+i", 0,CAT_CATEGORY_2D},
  {"bitxor", "bitxor", "Ou exclusif", "#bitxor(1,2)", 0, CAT_CATEGORY_PROGCMD},
  {"black", "black", "Option d'affichage", "#display=black", 0, CAT_CATEGORY_PROGCMD},
  {"blue", "blue", "Option d'affichage", "#display=blue", 0, CAT_CATEGORY_PROGCMD},
  {"ceiling(x)", 0, "Partie entiere superieure", "1.2", 0, CAT_CATEGORY_REAL},
    {"centre(objet)", 0, "Centre d'un cercle ou d'une sphere. Pour une conique a centre, renvoie le centre, un foyer et un point de la conique. Pour une parabole, renvoie le foyer et le sommet.", "cercle(0,1)", "sphere([0,0,0],[1,1,1])", CAT_CATEGORY_2D | (CAT_CATEGORY_3D << 8) },
    {"cercle(centre,rayon)", 0, "Cercle donne par centre et rayon ou par un diametre", "2+i,3", "1-i,1+i", CAT_CATEGORY_PROGCMD | (CAT_CATEGORY_2D << 8) | XCAS_ONLY},
    {"cercle_osculateur([x(t),y(t)],t,t0)", 0, "Cercle osculateur de la courbe parametree [x(t),y(t)] en t=t0", "[t,t^2],t,1", "[t,t^2],t", CAT_CATEGORY_PLOT | (CAT_CATEGORY_2D << 8) | XCAS_ONLY},
    {"circonscrit(A,B,C)", 0, "Cercle circonscrit", "-1,2+i,3", 0, CAT_CATEGORY_PROGCMD | (CAT_CATEGORY_2D << 8) | XCAS_ONLY},
  {"clearscreen()", "clearscreen()", "Efface l'ecran.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"coeff(p,x,n)", 0, "Coefficient de x^n dans le polynome p.", "(1+x)^6,x,3", 0, CAT_CATEGORY_POLYNOMIAL},
  {"cond(A,[1,2,inf])", 0, "Nombre de condition d'une matrice par rapport a la norme specifiee (par defaut 1)", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
    {"conique(expression)", 0, "Conique donnee par une equation polynomiale de degre 2 ou passant par 5 points", "x^2+x*y+y^2=5", "1,i,2+i,3-i,4+2i", CAT_CATEGORY_2D},
    {"coordonnees(object)", 0, "Coordonnees (cartesiennes)", "point(1,2)", "point(1,2,3)", CAT_CATEGORY_2D | (CAT_CATEGORY_3D << 8) },
    {"courbure([x(t),y(t)],t,t0)", 0, "Courbure de la courbe parametree [x(t),y(t)] en t=t0", "[t,t^2],t,1", "[t,t^2],t", CAT_CATEGORY_PLOT | (CAT_CATEGORY_2D << 8) | XCAS_ONLY},
  {"cross(u,v)", 0, "Produit vectoriel de u et v.","[1,2,3],[0,1,3]", 0, CAT_CATEGORY_LINALG | (CAT_CATEGORY_2D << 8) },
  {"cyan", "cyan", "Option d'affichage", "#display=cyan", 0, CAT_CATEGORY_PROGCMD},
  {"debug(f(args))", 0, "Execute la fonction f en mode pas a pas.", 0, 0, CAT_CATEGORY_PROG},
  {"degree(p,x)", 0, "Degre du polynome p en x.", "x^4-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"diff(f,var,[n])", 0, "Derivee de l'expression f par rapport a var (a l'ordre n, n=1 par defaut), exemple diff(sin(x),x) ou diff(x^3,x,2). Pour deriver f par rapport a x, utiliser f' (raccourci F3). Pour le gradient de f, var est la liste des variables.", "sin(x),x", "sin(x^2),x,3", CAT_CATEGORY_CALCULUS},
  {"discriminant(p,x)", 0, "Discriminant en x du polynome p.", "x^2+x+1,x", 0, CAT_CATEGORY_POLYNOMIAL},
  {"display", "display", "Option d'affichage", "#display=red", 0, CAT_CATEGORY_PROGCMD},
  {"distance(A,B)", 0, "Distance de 2 objets geometriques", "point(1,2,3),point(4,1,2)", 0, CAT_CATEGORY_3D | (CAT_CATEGORY_2D << 8) },
  {"dot(a,b)", 0, "Produit scalaire de 2 vecteurs. Raccourci: *", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_LINALG | (CAT_CATEGORY_2D << 8) },
  {"draw_string(s,x,y,c)", 0, "Affiche la chaine s en x,y", "\"Bonjour\",80,60", 0, CAT_CATEGORY_PROGCMD},
    {"droite(equation)", 0, "Droite donnee par une equation ou 2 points", "y=2x+1", "1+i,2-i", CAT_CATEGORY_PROGCMD | (CAT_CATEGORY_2D << 8) | (CAT_CATEGORY_2D << 16) | XCAS_ONLY},
#ifndef TURTLETAB
#endif
  {"elif (test)", "elif", "Tests en cascade", 0, 0, CAT_CATEGORY_PROG},
  //{"end", "end", "Fin de bloc", 0, 0, CAT_CATEGORY_PROG},
    {"ellipse(F1,F2,M)", 0, "Ellipse donnee par les 2 foyers et un point", "-1,1,2", 0, CAT_CATEGORY_2D},
    {"equation(objet)", 0, "Equation cartesienne. Utiliser parameq pour parametrique.", "circle(0,1)", "ellipse(-1,1,3)", CAT_CATEGORY_2D | (CAT_CATEGORY_3D << 8) },
  {"erf(x)", 0, "Fonction erreur en x.", "1.2", 0, CAT_CATEGORY_PROBA},
  {"erfc(x)", 0, "Fonction erreur complementaire en x.", "1.2", 0, CAT_CATEGORY_PROBA},
  {"est_aligne(A,B,C)", 0, "Renvoie 1 ou 2 si A, B, C sont alignes, 0 sinon.", "1,i,-1", "i,0,-i", CAT_CATEGORY_2D | XCAS_ONLY },
  {"est_cocyclique(A,B,C,D)", 0, "Renvoie 1 si A, B, C, D sont cocyliques, 0 sinon.", "1,i,-1,-i", "1,i,0,-i", CAT_CATEGORY_2D | XCAS_ONLY },
  {"est_element(A,G)", 0, "Renvoie 1 si A appartient a G, 0 sinon.", "point(0),circle(0,1)", "point(i),square(0,1)", CAT_CATEGORY_2D | XCAS_ONLY },
  {"est_orthogonal(D,E)", 0, "Renvoie 1 si D et E sont perpendiculaires, 0 sinon.", "line(y=x),line(y=-x)", "line(y=x),line(y=x+1)", CAT_CATEGORY_2D | XCAS_ONLY },
  {"est_parallele(D,E)", 0, "Renvoie 1 si D et E sont paralleles, 0 sinon.", "line(y=x),line(y=-x)", "line(y=x),line(y=x+1)", CAT_CATEGORY_2D | XCAS_ONLY },
  {"eval(f)", 0, "Evalue f.", 0, 0, CAT_CATEGORY_PROGCMD},
    //  {"evolute([x(t),y(t)],t)", 0, "Developpee de la courbe parametree [x(t),y(t)] (on peut ajouter 2 parametres tmin,tmax pour tracer la developpee sur l'intervalle tmin..tmax)", "[t,t^2],t", "[t,t^2],t,-1,1", CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_2D << 8) | XCAS_ONLY},
  {"exact(x)", 0, "Convertit x en rationnel. Raccourci shift S-D", "1.2", 0, CAT_CATEGORY_REAL},
  {"factor(p,[x])", 0, "Factorisation du polynome p (utiliser ifactor pour un entier). Raccourci: p=>*", "x^4-1", "x^6+1,sqrt(3)", CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"filled", "filled", "Option d'affichage", 0, 0, CAT_CATEGORY_PROGCMD},
  {"float(x)", 0, "Convertit x en nombre approche (flottant).", "pi", 0, CAT_CATEGORY_REAL},
  {"floor(x)", 0, "Partie entiere de x", "pi", 0, CAT_CATEGORY_REAL},
  {"fonction f(x)", "fonction", "Definition de fonction (Xcas). Exemple\nfonction f(x)\n local y;\ny:=x*x;\nreturn y;\nffonction", 0, 0, CAT_CATEGORY_PROG},
  // {"from math/... import *", "from math import *", "Instruction pour utiliser les fonctions de maths ou des fonctions aleatoires [random] ou la tortue en anglais [turtle]. Importer math n'est pas necessaire dans KhiCAS", "#from random import *", "#from turtle import *", CAT_CATEGORY_PROG},
    {"frenet([x(t),y(t)],t,t0)", 0, "Courbure, centre de courbure et repere de Frenet de la courbe parametree [x(t),y(t)] en t=t0", "[t,t^2],t,1", "[t,t^2],t", CAT_CATEGORY_PLOT | (CAT_CATEGORY_2D << 8) | XCAS_ONLY},
  {"fsolve(equation,x=a[..b])", 0, "Resolution approchee de equation pour x dans l'intervalle a..b ou en partant de x=a.","cos(x)=x,x=0..1", "cos(x)-x,x=0.0", CAT_CATEGORY_SOLVE},
  {"gauss(q)", 0, "Reduction d'une forme quadratique", "x^2+x*y+x*z+y^2+z^2,[x,y,z]", 0, CAT_CATEGORY_LINALG},
  {"gcd(a,b,...)", 0, "Plus grand commun diviseur.", "23,13", "x^2-1,x^3-1", CAT_CATEGORY_ARIT | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"green", "green", "Option d'affichage", "#display=green", 0, CAT_CATEGORY_PROGCMD},
    {"hauteur(A,B,C)", 0, "Hauteur du triangle ABC issue de A", "1,i,2+i", 0,CAT_CATEGORY_2D},
  {"hilbert(n)", 0, "Matrice de Hilbert de taille n.", "4", 0, CAT_CATEGORY_MATRIX},
    {"homothetie(centre,rapport,objet)", 0, "Image de l'objet par homothetie de rapport", "0,2,circle(1,1)", 0, CAT_CATEGORY_2D },
    {"hyperbole(F1,F2,M)", 0, "Hyperbole donnee par 2 foyers et un point", "-2-i,2+i,1", 0, CAT_CATEGORY_2D},
  {"idivis(n)", 0, "Liste des diviseurs d'un entier n.", "10", 0, CAT_CATEGORY_ARIT},
  {"idn(n)", 0, "matrice identite n * n", "4", 0, CAT_CATEGORY_MATRIX},
  {"ifactor(n)", 0, "Factorisation d'un entier (pas trop grand!). Raccourci n=>*", "1234", 0, CAT_CATEGORY_ARIT},
    {"inscrit(A,B,C)", 0, "Cercle inscrit", "-1,2+i,3", 0, CAT_CATEGORY_PROGCMD | (CAT_CATEGORY_2D << 8) | XCAS_ONLY},
  {"inf", "inf", "Plus l'infini. Utiliser -inf pour moins l'infini ou infinity pour l'infini complexe. Raccourci shift INS.", "-inf", "infinity", CAT_CATEGORY_CALCULUS},
  {"integrate(f,x,[,a,b])", 0, "Primitive de f par rapport a la variable x, par ex. integrate(x*sin(x),x). Pour calculer une integrale definie, entrer les arguments optionnels a et b, par ex. integrate(x*sin(x),x,0,pi). Pour une integrale curviligne, integrate([champ_x,champ_y],[x,y],courbe,tmin,tmax), par ex. aire ellipse G:=plotparam([2*cos(t),sin(t)],t):; integrate([0,x],[x,y],G,0,2*pi). Raccourci SHIFT F3.", "x*sin(x),x", "cos(x)/(1+x^4),x,0,inf", CAT_CATEGORY_CALCULUS},
    {"inter(A,B)", 0, "Liste des intersections. Utiliser single_inter si l'intersection est unique.", "line(y=x),circle(0,1)", 0, CAT_CATEGORY_3D | (CAT_CATEGORY_2D << 8) | XCAS_ONLY},
    {"inter_unique(A,B)", 0, "Premiere intersection. Utiliser inter pour une liste d'intersections.", "line(y=x),line(x+y=3)", 0, CAT_CATEGORY_3D | (CAT_CATEGORY_2D << 8) | XCAS_ONLY},
  {"inv(A)", 0, "Inverse de A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX|  (CAT_CATEGORY_LINALG<<8)},
  //{"inverser(v)", "inverser ", "La variable v est remplacee par son inverse", "#v:=3; inverser v", 0, CAT_CATEGORY_SOFUS},
  {"iquo(a,b)", 0, "Quotient euclidien de deux entiers.", "23,13", 0, CAT_CATEGORY_ARIT},
  {"isprime(n)", 0, "Renvoie 1 si n est premier, 0 sinon.", "11", "10", CAT_CATEGORY_ARIT},
  {"lcm(a,b,...)", 0, "Plus petit commun multiple.", "23,13", "x^2-1,x^3-1", CAT_CATEGORY_ARIT | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"lcoeff(p,x)", 0, "Coefficient dominant du polynome p.", "x^4-1", 0, CAT_CATEGORY_POLYNOMIAL},
#ifdef RELEASE
  {"len(l)", 0, "Taille d'une liste.", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_LIST},
#endif
  {"limit(f,x=a)", 0, "Limite de f en x = a. Ajouter 1 ou -1 pour une limite a droite ou a gauche, limit(sin(x)/x,x=0) ou limit(abs(x)/x,x=0,1). Raccourci: SHIFT MIXEDFRAC", "sin(x)/x,x=0", "exp(-1/x),x=0,1", CAT_CATEGORY_CALCULUS},
  {"line_width_", "line_width_", "Prefixe d'epaisseur (2 a 8)", 0, 0, CAT_CATEGORY_PROGCMD},
  {"linsolve([eq1,eq2,..],[x,y,..])", 0, "Resolution de systeme lineaire. Peut utiliser le resultat de lu pour resolution en O(n^2).","[x+y=1,x-y=2],[x,y]", "#p,l,u:=lu([[1,2],[3,4]]); linsolve(p,l,u,[5,6])", CAT_CATEGORY_SOLVE | (CAT_CATEGORY_LINALG <<8) | (CAT_CATEGORY_MATRIX << 16)},
  {"lu(A)", 0, "decomposition LU de la matrice A, P*A=L*U, renvoie P permutation, L et U triangulaires inferieure et superieure", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"magenta", "magenta", "Option d'affichage", "#display=magenta", 0, CAT_CATEGORY_PROGCMD},
  {"map(l,f)", 0, "Applique f aux elements de la liste l.","[1,2,3],x->x^2", 0, CAT_CATEGORY_LIST},
  {"matpow(A,n)", 0, "Renvoie la matrice A la puissance n", "[[1,2],[3,4]],n","#assume(n>=1);matpow([[0,2],[0,4]],n)", CAT_CATEGORY_MATRIX},
    {"mediane(A,B,C)", 0, "Mediane du triangle ABC issue de A", "1,i,2+i", 0,CAT_CATEGORY_2D},
    {"mediatrice(A,B)", 0, "Mediatrice du segment AB", "1,i", 0,CAT_CATEGORY_2D},
    {"milieu(A,B)", 0, "Milieu de AB", "1,i", 0,CAT_CATEGORY_2D | (CAT_CATEGORY_3D << 8)},
  {"mult_conjugate", 0, "Multiplier par le conjugue (sqrt).", "sqrt(2)-sqrt(3)", 0, CAT_CATEGORY_ALGEBRA},
  {"not(x)", 0, "Non logique.", 0, 0, CAT_CATEGORY_PROGCMD},
    {"parabole(F,A)", 0, "Parabole donnee par foyer et sommet", "-2-i,2+i", 0, CAT_CATEGORY_2D},
    {"parameq(objet)", 0, "Equations parametriques. Utiliser equation pour une equation cartesienne", "circle(0,1)", "ellipse(-1,1,3)", CAT_CATEGORY_2D | (CAT_CATEGORY_3D << 8) },
  {"partfrac(p,x)", 0, "Decomposition en elements simples. Raccourci p=>+", "1/(x^4-1)", 0, CAT_CATEGORY_ALGEBRA},
    {"pcoeff(p)", 0, "Polynome unitaire dont on donne la liste des racines (fonction reciproque de proot)", "[1,2,3]", 0, CAT_CATEGORY_POLYNOMIAL},
    {"peval(p,x)", 0, "Valeur d'un polynome en un point", "[1,2,3],4", 0, CAT_CATEGORY_POLYNOMIAL},
    {"piecewise(test1,val1,...)", 0, "Fonction definie par morceaux", "x<0,-x,x<1,x+1,x^2", 0, CAT_CATEGORY_CALCULUS},
    {"plan(equation)", 0, "Plan donne par equation ou 3 points", "z=x+y-1", "[0,0,0],[1,0,0],[0,1,0]", CAT_CATEGORY_3D | XCAS_ONLY},
  {"plus_point", "plus_point", "Option d'affichage", "#display=blue+plus_point", 0, CAT_CATEGORY_PROGCMD },
    {"polygone(list)", 0, "Polygone ferme donne par la liste de ses sommets.", "1-i,2+i,3,3-2i", 0, CAT_CATEGORY_PROGCMD | (CAT_CATEGORY_2D << 8) | XCAS_ONLY},
    {"polyhedron(A,B,C,D,...)", 0, "Polyedre convexe dont les sommets sont parmi A,B,C,D,...", "[0,0,0],[0,5,0],[0,0,5],[1,2,6]", 0, CAT_CATEGORY_3D},
  {"pour (boucle Xcas)", "pour  de  jusque  faire  fpour;", "Boucle definie.","#pour j de 1 jusque 10 faire print(j,j^2); fpour;", 0, CAT_CATEGORY_PROG},
  {"powmod(a,n,p[,P,x])", 0, "Renvoie a^n mod p, ou a^n mod un entier p et un polynome P en x.","123,456,789", "x+1,452,19,x^4+x+1,x", CAT_CATEGORY_ARIT},
  {"print(expr)", 0, "Afficher dans la console", 0, 0, CAT_CATEGORY_PROG},
    {"projection(obj1,obj2)", 0, "Projection sur obj1 de obj2", "line(y=x),point(2,3)", 0, CAT_CATEGORY_2D },
  {"proot(p)", 0, "Racines reelles et complexes approchees d'un polynome. Exemple proot([1,2.1,3,4.2]) ou proot(x^3+2.1*x^2+3x+4.2)", "x^3+2.1*x^2+3x+4.2", 0, CAT_CATEGORY_POLYNOMIAL},
  {"purge(x)", 0, "Purge le contenu de la variable x. Raccourci SHIFT-FORMAT", 0, 0, CAT_CATEGORY_PROGCMD | (CAT_CATEGORY_SOFUS<<8)},
    {"pyramid(A,B,C)", 0, "Tetraedre d'arete AB avec une face dans le plan ABC", "[0,0,0],[3,0,0],[0,1,0]", "[0,0,0],[3,0,0],[0,3,0],[0,0,4]", CAT_CATEGORY_3D},
  {"qr(A)", 0, "Factorisation A=Q*R avec Q orthogonale et R triangulaire superieure", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
    {"_rpig(equation)", 0, "Quadrique donnee par une equation (ou 9 points)", "x^2-y^2+z^2", "x^2+x*y+y^2+z^2-3", CAT_CATEGORY_3D},
  {"quo(p,q,x)", 0, "Quotient de division euclidienne polynomiale en x.", 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"quote(x)", 0, "Renvoie l'expression x non evaluee.", 0, 0, CAT_CATEGORY_ALGEBRA},
  {"ratnormal(x)", 0, "Ecrit sous forme d'une fraction irreductible.", "(x+1)/(x^2-1)^2", 0, CAT_CATEGORY_ALGEBRA},
    {"rayon(objet)", 0, "Rayon d'un cercle ou d'une sphere", "circle(0,1)", "sphere([0,0,0],[1,1,1])", CAT_CATEGORY_2D | (CAT_CATEGORY_3D << 8) },
  {"red", "red", "Option d'affichage", "#display=red", 0, CAT_CATEGORY_PROGCMD},
    {"reflection(obj1,obj2)", 0, "Symetrique de obj2", "line(y=x),cercle(1,1)", 0, CAT_CATEGORY_2D },
  {"rem(p,q,x)", 0, "Reste de division euclidienne polynomiale en x", 0, 0, CAT_CATEGORY_POLYNOMIAL},
#ifdef RELEASE
#endif
  {"revert(p[,x])", 0, "Developpement de Taylor reciproque, p doit etre nul en 0","x+x^2+x^4", 0, CAT_CATEGORY_CALCULUS},
  {"rgb(r,g,b)", 0, "couleur definie par niveau de rouge, vert, bleu entre 0 et 255", "255,0,255", 0, CAT_CATEGORY_PROGCMD},
  {"rhombus_point", "rhombus_point", "Option d'affichage", "#display=magenta+rhombus_point", 0, CAT_CATEGORY_PROGCMD },
    {"rotation(centre,angle,objet)", 0, "Image de l'objet par la rotation de centre et angle donnes en argyment", "2-i,pi/2,circle(0,1)", "sphere([0,0,0],[1,1,1])", CAT_CATEGORY_2D | (CAT_CATEGORY_3D << 8) },
  {"rsolve(equation,u(n),[init])", 0, "Expression d'une suite donnee par une recurrence.","u(n+1)=2*u(n)+3,u(n),u(0)=1", "([u(n+1)=3*v(n)+u(n),v(n+1)=v(n)+u(n)],[u(n),v(n)],[u(0)=1,v(0)=2]", CAT_CATEGORY_SOLVE},
  {"seq(expr,var,a,b[,pas])", 0, "Liste de terme general donne.","j^2,j,1,10", "j^2,j,1,10,2", CAT_CATEGORY_LIST},
  {"si (test Xcas)", "si  alors  sinon  fsi;", "Test.", "#f(x):=si x>0 alors x; sinon -x; fsi;// valeur absolue", 0, CAT_CATEGORY_PROG},
    {"similitude(centre,rapport,angle,objet)", 0, "Image de l'objet par similitude", "0,2,pi/2,circle(1,1)", 0, CAT_CATEGORY_2D },
  {"simplify(expr)", 0, "Renvoie en general expr sous forme simplifiee. Raccourci expr=>/", "sin(3x)/sin(x)", "ln(4)-ln(2)", CAT_CATEGORY_ALGEBRA},
  {"solve(equation,x)", 0, "Resolution exacte d'une equation en x (ou d'un systeme polynomial). Utiliser csolve pour les solutions complexes, linsolve pour un systeme lineaire. Raccourci SHIFT XthetaT", "x^2-x-1=0,x", "[x^2-y^2=0,x^2-z^2=0],[x,y,z]", CAT_CATEGORY_SOLVE},
    {"sommets(objet)", 0, "Liste des sommets d'un polygone ou polyedre", "triangle(1,i,2)", "cube([0,0,0],[1,0,0],[0,1,0])", CAT_CATEGORY_2D | (CAT_CATEGORY_3D << 8) },
  {"sort(l)", 0, "Trie une liste.","[3/2,2,1,1/2,3,2,3/2]", "[[1,2],[2,3],[4,3]],(x,y)->when(x[1]==y[1],x[0]>y[0],x[1]>y[1]", CAT_CATEGORY_LIST},
  {"square_point", "square_point", "Option d'affichage", "#display=cyan+square_point", 0, CAT_CATEGORY_PROGCMD },
  {"star_point", "star_point", "Option d'affichage", "#display=magenta+star_point", 0, CAT_CATEGORY_PROGCMD },
  {"subst(a,b=c)", 0, "Remplace b par c dans a. Raccourci a(b=c). Pour faire plusieurs remplacements, saisir subst(expr,[b1,b2...],[c1,c2...])", "x^2,x=3", "x+y^2,[x,y],[1,2]", CAT_CATEGORY_ALGEBRA},
  {"sum(f,k,m,M)", 0, "Somme de l'expression f dependant de k pour k variant de m a M. Exemple sum(k^2,k,1,n)=>*. Raccourci ALPHA F3", "k,k,1,n", "k^2,k", CAT_CATEGORY_CALCULUS},
  {"tantque (boucle Xcas)", "tantque  faire  ftantque;", "Boucle indefinie.", "#j:=13; tantque j!=1 faire j:=when(even(j),j/2,3j+1); print(j); ftantque;", 0, CAT_CATEGORY_PROG},
  {"taylor(f,x=a,n,[polynom])", 0, "Developpement de Taylor de l'expression f en x=a a l'ordre n, ajouter polynom pour enlever le terme de reste.","sin(x),x=0,5", "sin(x),x=0,5,polynom", CAT_CATEGORY_CALCULUS},
  {"tcollect(expr)", 0, "Linearisation trigonometrique et regroupement.","sin(x)+cos(x)", 0, CAT_CATEGORY_TRIG},
  {"texpand(expr)", 0, "Developpe les fonctions trigonometriques, exp et ln.","sin(3x)", "ln(x*y)", CAT_CATEGORY_TRIG},
  {"time(cmd)", 0, "Temps pour effectuer une commande ou mise a l'heure de horloge","int(1/(x^4+1),x)","8,0", CAT_CATEGORY_PROG},
  {"tlin(expr)", 0, "Linearisation trigonometrique de l'expression.","sin(x)^3", 0, CAT_CATEGORY_TRIG},
  {"_rmh61(A)", 0, "Trace de la matrice A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"tran(A)", 0, "Transposee de la matrice A. Pour la transconjuguee utiliser trn(A) ou A^*.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX|  (CAT_CATEGORY_LINALG<<8)},
    {"translation(vect,obj)", 0, "Translation par vect de obj", "[1,2],cercle(0,1)", 0, CAT_CATEGORY_2D },
    {"triangle(A,B,C)", 0, "Triangle donne par 3 sommets", "1+i,1-i,-1", "A,B,C", CAT_CATEGORY_2D},
  {"triangle_point", "triangle_point", "Option d'affichage", "#display=yellow+triangle_point", 0, CAT_CATEGORY_PROGCMD},
  {"trigcos(expr)", 0, "Exprime sin^2 et tan^2 avec cos^2.","sin(x)^4", 0, CAT_CATEGORY_TRIG},
  {"trigsin(expr)", 0, "Exprime cos^2 et tan^2 avec sin^2.","cos(x)^4", 0, CAT_CATEGORY_TRIG},
  {"trigtan(expr)", 0, "Exprime cos^2 et sin^2 avec tan^2.","cos(x)^4", 0, CAT_CATEGORY_TRIG},
  {"v augmente_de n", " augmente_de ", "La variable v augmente de n, ou de n %", "#v:=3; v augmente_de 1", 0, CAT_CATEGORY_SOFUS},
  {"v diminue_de n", " diminue_de ", "La variable v diminue de n, ou de n %", "#v:=3; v diminue_de 1", 0, CAT_CATEGORY_SOFUS},
  //{"v est_divise_par n", " est_divise_par ", "La variable v est divisee par n", "#v:=3; v est_divise_par 2", 0, CAT_CATEGORY_SOFUS},
  //{"v est_eleve_puissance n", " est_eleve_puissance ", "La variable v est eleveee a la puissance n", "#v:=3; v est_eleve_puissance 2", 0, CAT_CATEGORY_SOFUS},
  // {"v est_multiplie_par n", " est_multiplie_par ", "La variable v est multipliee par n", "#v:=3; v est_multiplie_par 2", 0, CAT_CATEGORY_SOFUS},
  //{"version", "version()", "Khicas 1.5.0, (c) B. Parisse et al. www-fourier.ujf-grenoble.fr/~parisse. License GPL version 2. Interface adaptee d'Eigenmath pour Casio, G. Maia, http://gbl08ma.com", 0, 0, CAT_CATEGORY_PROGCMD},
  {"vector(A,B)", 0, "vecteur AB", 0, 0, CAT_CATEGORY_2D | (CAT_CATEGORY_3D << 8)},
  {"yellow", "yellow", "Option d'affichage", "#display=yellow", 0, CAT_CATEGORY_PROGCMD},
  {"|", "|", "Ou logique", "#1|2", 0, CAT_CATEGORY_PROGCMD},
  {"~", "~", "Complement", "#~7", 0, CAT_CATEGORY_PROGCMD},

};

const char chk_restart_string1[]="Conserver les variables?";
const char chk_restart_string2[]="F1: conserver,   F6: effacer";
const char aide_khicas_string[]="Aide Khicas";
const char main_string1[]="Effacer variables?";
const char main_string2[]="F1: annul,  F6: confirmer";
const char shortcuts_string[]="";
const char apropos_string[]="";

int CAT_COMPLETE_COUNT=sizeof(completeCat)/sizeof(catalogFunc);

ustl::string insert_string(int index){
  ustl::string s;
  if (completeCat[index].insert)
    s=completeCat[index].insert;
  else {
    s=completeCat[index].name;
    int pos=s.find('(');
    if (pos>=0 && pos<s.size())
      s=s.substr(0,pos+1);
  }
  return s; //s+' ';
}

int showCatalog(char* insertText,int preselect,int menupos) {
  // returns 0 on failure (user exit) and 1 on success (user chose a option)
  MenuItem menuitems[CAT_CATEGORY_LOGO+1];
    menuitems[CAT_CATEGORY_ALL].text = (char*)((lang==1)?"Tout":"All");
    menuitems[CAT_CATEGORY_ALGEBRA].text = (char*)((lang==1)?"Algebre":"Algebra");
    menuitems[CAT_CATEGORY_LINALG].text = (char*)((lang==1)?"Alg. (bi)lineaire":"Linear algebra");
    menuitems[CAT_CATEGORY_CALCULUS].text = (char*)((lang==1)?"Analyse":"Calculus");
    menuitems[CAT_CATEGORY_ARIT].text = (char*)"Arithmetic";
    menuitems[CAT_CATEGORY_COMPLEXNUM].text = (char*)"Complexes";
    menuitems[CAT_CATEGORY_PLOT].text = (char*)((lang==1)?"Courbes":"Curves");
    menuitems[CAT_CATEGORY_POLYNOMIAL].text = (char*)((lang==1)?"Polynomes":"Polynomials");
    menuitems[CAT_CATEGORY_PROBA].text = (char*)((lang==1)?"Probabilites":"Probabilities");
    menuitems[CAT_CATEGORY_PROGCMD].text = (char*)((lang==1)?"Programme cmds (0)":"Program cmds (0)");
    menuitems[CAT_CATEGORY_REAL].text = (char*)((lang==1)?"Reels (x,theta,t)":"Reals");
    menuitems[CAT_CATEGORY_SOLVE].text = (char*)((lang==1)?"Resoudre (log)":"Solve (log)");
    menuitems[CAT_CATEGORY_STATS].text = (char*)((lang==1)?"Statistiques (ln)":"Statistics (ln)");
    menuitems[CAT_CATEGORY_TRIG].text = (char*)((lang==1)?"Trigonometrie (sin)":"Trigonometry (sin)");
    menuitems[CAT_CATEGORY_OPTIONS].text = (char*)"Options (cos)";
    menuitems[CAT_CATEGORY_LIST].text = (char*)((lang==1)?"Listes (tan)":"Lists (tan)");
    menuitems[CAT_CATEGORY_MATRIX].text = (char*)"Matrices ";
    menuitems[CAT_CATEGORY_PROG].text = (char*)((lang==1)?"Programmes (S<>D)":"Programs (S<>D)");
    menuitems[CAT_CATEGORY_SOFUS].text = (char*)((lang==1)?"Modif. variables (":"Change variables (()");
    menuitems[CAT_CATEGORY_PHYS].text = (char*)((lang==1)?"Constantes phys. )":"Physics constants ())");
    menuitems[CAT_CATEGORY_UNIT].text = (char*)((lang==1)?"Unites physiques ,":"Units (,)");
    menuitems[CAT_CATEGORY_2D].text = (char*)((lang==1)?"Geometrie (->)":"Geometry (->)");
    menuitems[CAT_CATEGORY_3D].text = (char*)((lang==1)?"3D (*)":"3D (*)");
    menuitems[CAT_CATEGORY_LOGO].text = (char*)((lang==1)?"Tortue (/)":"Turtle (/)");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=sizeof(menuitems)/sizeof(MenuItem);
  menu.scrollout=1;
  menu.title = (char*)"Liste de commandes";
  //puts("catalog 1");
  while(1) {
    if (preselect)
      menu.selection=preselect;
    else {
      if (menupos>0)
	menu.selection=menupos;
      int sres = doMenu(&menu);
    Bdisp_PutDisp_DD();
    drawCasioCasBorder();
      if (sres != MENU_RETURN_SELECTION)
	return 0;
    }
    // puts("catalog 3");
    if (doCatalogMenu((char *)insertText, (char *)menuitems[menu.selection-1].text, menu.selection-1)) {
      const char * ptr=0;
      if (strcmp("_removed_array ",insertText)==0 && (ptr=input_matrix(false)) )
	return 0;
      if (strcmp("list ",insertText)==0 && (ptr=input_matrix(true)) )
	return 0;
      return 1;
    }
    if (preselect)
      return 0;
  }
  return 0;
}

  string remove_accents(const string & s){
    string r;
    for (int i=0;i<s.size();++i){
      unsigned char ch=s[i];
      if (ch==195 && i<s.size()-1){
	++i;
	switch ((unsigned char)s[i]){
	case 160: case 161: case 162:
	  r+='a';
	  continue;
	case 168: case 169: case 170:
	  r+='e';
	  continue;
	case 172: case 173: case 174:
	  r+='i';
	  continue;
	case 178: case 179: case 180:
	  r += 'o';
	  continue;
	case 185: case 186: case 187:
	  r+='u';
	  continue;
	}
	r += '?';
	continue;
      }
      r+=(char)ch;
    }
    return r;
  }

string extract_example(const string & s,int n){
  int l=s.size(),j=0;
  string res;
  for (int i=0;i<l;++i){
    if (s[i]==';'){
      if (j==n)
	break;
      ++j;
      continue;
    }
    if (j==n)
      res+=s[i];
  }
  return res;
}

ustl::string longhelp(const char * s);
int QRdisp(const char * text,const char *msg1,const char * msg2,const char * msg3,const char * msg4);

// 0 on exit, 1 on success
int doCatalogMenu(char* insertText, char* title, int category,const char * cmdname) {
  int allcmds=builtin_lexer_functions_end()-builtin_lexer_functions_begin();
  int allopts=lexer_tab_int_values_end-lexer_tab_int_values_begin;
  bool isall=category==CAT_CATEGORY_ALL;
  bool isopt=category==CAT_CATEGORY_OPTIONS;
  int nitems = isall? allcmds:(isopt?allopts:CAT_COMPLETE_COUNT);
  MenuItem* menuitems = (MenuItem*)alloca(sizeof(MenuItem)*nitems);
  int cur = 0,curmi = 0,i=0,menusel=-1,cmdl=cmdname?strlen(cmdname):0;
  gen g;
  while(cur<nitems) {
    MenuItem & curmenuitem= menuitems[curmi];
    curmenuitem.type = MENUITEM_NORMAL;
    curmenuitem.color = TEXT_COLOR_BLACK;    
    if (isall || isopt) {
      const char * text=isall?(builtin_lexer_functions_begin()+curmi)->first:(lexer_tab_int_values_begin+curmi)->keyword;
      if (menusel<0 && cmdname && !strncmp(cmdname,text,cmdl))
	menusel=curmi;
      curmenuitem.text = text;
      curmenuitem.isfolder = allcmds; // assumes allcmds>allopts
      curmenuitem.token=isall?((builtin_lexer_functions_begin()+curmi)->second.subtype+256):((lexer_tab_int_values_begin+curmi)->subtype+(lexer_tab_int_values_begin+curmi)->return_value*256);
      // curmenuitem.token=isall?find_or_make_symbol(text,g,0,false,contextptr):((lexer_tab_int_values_begin+curmi)->subtype+(lexer_tab_int_values_begin+curmi)->return_value*256);
      for (;i<CAT_COMPLETE_COUNT;++i){
	const char * catname=completeCat[i].name;
	int tmp=strcmp(catname,text);
	if (tmp>=0){
	  size_t st=strlen(text),j=tmp?0:st;
	  for (;j<st;++j){
	    if (catname[j]!=text[j])
	      break;
	  }
	  if (j==st && (!isalphanum(catname[j]))){
	    curmenuitem.isfolder = i;
	    ++i;
	  }
	  break;
	}
      }
      // compare text with completeCat
      ++curmi;
    }
    else {
      int cat=completeCat[cur].category;
      if ( (xcas_python_eval==0 || cat>0) &&
	   ((cat & 0xff) == category ||
	    (cat & 0xff00) == (category<<8) ||
	    (cat & 0xff0000) == (category <<16)
	    )
	   ){
	curmenuitem.isfolder = cur; // little hack: store index of the command in the full list in the isfolder property (unused by the menu system in this case)
	curmenuitem.text = completeCat[cur].name;
	curmi++;
      }
    }
    cur++;
  }
  if (curmi==0)
    return 0;
  Menu menu;
  if (menusel>=0)
    menu.selection=menusel+1;
  menu.items=menuitems;
  menu.numitems=curmi;
  if (isopt){ menu.selection=5; menu.scroll=4; }
  if (curmi>=100)
    SetSetupSetting( (unsigned int)0x14, 0x88);	
  DisplayStatusArea();
  menu.scrollout=1;
  menu.title = title;
  menu.type = MENUTYPE_FKEYS;
  menu.height = 7;
  while(1) {
    int fkeyw=LCD_WIDTH_PX/6;
    drawRectangle(0,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(0,LCD_HEIGHT_PX-STATUS_AREA_PX-19,"  INPUT",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    drawRectangle(fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19," EXEMPL1",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    drawRectangle(2*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(2*fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19," EXEMPL2",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    drawRectangle(3*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    drawRectangle(4*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(4*fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19,"  HELP",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    drawRectangle(5*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(5*fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19," QRHELP",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    int sres = doMenu(&menu);
    if(sres == MENU_RETURN_EXIT){
      reset_alpha();
      return sres;
    }
    int index=menuitems[menu.selection-1].isfolder;
    const char *fcmdname=menuitems[menu.selection-1].text,* fhowto=0,*fsyntax=0,*fexamples=0,*frelated=0;
    bool stathelp=has_static_help(fcmdname,lang,fhowto,fsyntax,fexamples,frelated);
    if (sres==KEY_CTRL_F6 && fcmdname){
      string url="https://www-fourier.ujf-grenoble.fr/~parisse/giac/doc/fr/cascmd_fr/"+longhelp(fcmdname);
      QRdisp(url.c_str(),"Xcas doc qrcode",fcmdname,"EXE/EXIT: OK",0);
    }
    if (sres == KEY_CTRL_F5) {
      char * example=index<allcmds?completeCat[index].example:0;
      char * example2=index<allcmds?completeCat[index].example2:0;
      textArea text;
      text.editable=false;
      text.clipline=-1;
      text.title = (char*)"Aide sur la commande";
      text.allowF1=true;
      text.python=python_compat(contextptr);
      ustl::vector<textElement> & elem=text.elements;
      elem = ustl::vector<textElement> (example2?4:3);
      elem[0].s = index<allcmds?completeCat[index].name:menuitems[menu.selection-1].text;
      elem[0].color = COLOR_BLUE;
      elem[1].newLine = 1;
      elem[1].lineSpacing = 3;
      ustl::string autoexample;
      if (index<allcmds)
	elem[1].s = completeCat[index].desc;
      else {
	int token=menuitems[menu.selection-1].token;
	elem[1].s="Desole, pas d'aide disponible...";
	// *logptr(contextptr) << token << endl;
	if (stathelp){
	  elem[1].s=remove_accents(fhowto);
	  //cout << fexamples << '\n';
	  example=(char *)fexamples;
	}
	else {
	  if (isopt){
	    if (token==_INT_PLOT+T_NUMBER*256){
	      autoexample="display="+elem[0].s;
	      elem[1].s ="Option d'affichage: "+ autoexample;
	    }
	    if (token==_INT_COLOR+T_NUMBER*256){
	      autoexample="display="+elem[0].s;
	      elem[1].s="Option de couleur: "+ autoexample;
	    }
	    if (token==_INT_SOLVER+T_NUMBER*256){
	      autoexample=elem[0].s;
	      elem[1].s="Option de fsolve: " + autoexample;
	    }
	    if (token==_INT_TYPE+T_TYPE_ID*256){
	      autoexample=elem[0].s;
	      elem[1].s="Type d'objet: " + autoexample;
	    }
	  }
	  if (isall){
	    if (token==T_UNARY_OP || token==T_UNARY_OP_38)
	      elem[1].s=elem[0].s+"(args)";
	  }
	}
      }
      ustl::string ex("Ex. (F2): "),ex2("Ex. (F3): ");
      elem[2].newLine = 1;
      elem[2].lineSpacing = 3;
      if (example){
	if (example[0]=='#')
	  ex += example+1;
	else {
	  if (index<allcmds){
	    ex += insert_string(index);
	    ex += example;
	    ex += ")";
	  }
	  else {
	    string allex(example);
	    ex += extract_example(allex,0); 
	    string tmp=extract_example(allex,1);
	    if (tmp.size()>1){
	      elem.push_back(elem[2]);
	      //cout << tmp << '\n' ;
	      elem[3].newLine = 1;
	      elem[3].lineSpacing = 3;
	      elem[3].s=ex2+tmp;
	    }
	  }
	}
	elem[2].s = ex;
	if (example2){
	  if (example2[0]=='#')
	    ex2 += example2+1;
	  else {
	    if (index<allcmds){
	      ex2 += insert_string(index);
	      ex2 += example2;
	      ex2 += ")";
	    }
	  }
	  elem[3].newLine = 1;
	  elem[3].lineSpacing = 3;
	  elem[3].s=ex2;
	}
      }
      else {
	if (autoexample.size())
	  elem[2].s=ex+autoexample;
	else
	  elem.pop_back();
      }
      sres=doTextArea(&text);
      if (sres==KEY_CTRL_F4 && fcmdname){
        string url="https://www-fourier.ujf-grenoble.fr/~parisse/giac/doc/fr/cascmd_fr/"+longhelp(fcmdname);
        QRdisp(url.c_str(),"Xcas doc qrcode",fcmdname,"EXE/EXIT: OK",0);
      }
    }
    if (sres == KEY_CTRL_F2 || sres==KEY_CTRL_F3) {
      reset_alpha();
      if (isopt){
	int token=menuitems[menu.selection-1].token;
	if (token==_INT_PLOT+T_NUMBER*256 || token==_INT_COLOR+T_NUMBER*256)
	  strcpy(insertText,"display=");
	else
	  *insertText=0;
	strcat(insertText,menuitems[menu.selection-1].text);
	return 1;
      }
      ustl::string s;
      char * example=0;
      string ex;
      if (stathelp){
	ex=extract_example(fexamples,sres==KEY_CTRL_F2?0:1);
	//cout << ex << "\n";
	example=(char *)ex.c_str();
      }
      if (index<allcmds && completeCat[index].example){
	s=insert_string(index);
	if (sres==KEY_CTRL_F2)
	  example=completeCat[index].example;
	else
	  example=completeCat[index].example2;
      }
      if (example){
	if (example[0]=='#')
	  s=example+1;
	else {
	  s += example;
	  if (!stathelp)
	    s += ")";
	}
	strcpy(insertText, s.c_str());
	return 1;
      }
      sres=KEY_CTRL_F1;
    }
    if(sres == MENU_RETURN_SELECTION || sres == KEY_CTRL_F1) {
      reset_alpha();
      strcpy(insertText,index<allcmds?insert_string(index).c_str():menuitems[menu.selection-1].text);
      return 1;
    }
  }
  return 0;
}
