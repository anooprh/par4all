/* package pour la structure de donnees sommet (sommet d'un systeme generateur)
 *
 * Francois Irigoin, Mai 1989
 */

#include <stdio.h>
#include <sys/stdtypes.h>  /* for debug with dbmalloc */
#include <malloc.h>

#include "boolean.h"
#include "assert.h"
#include "arithmetique.h"
#include "vecteur.h"

#include "ray_dte.h"
#include "sommet.h"

#define MALLOC(s,t,f) malloc(s)
#define FREE(s,t,f) free(s)

/* 
 * Creation d'un sommet    
 */
Psommet sommet_new()
{
    Psommet s;

    s = (Psommet) MALLOC(sizeof(Ssommet), SOMMET, "sommet_dup");
    s->denominateur =0;
    s->vecteur = NULL;
    s->eq_sat =NULL;
    s->succ = NULL;
    return (s);
}

/* Psommet sommet_make(int d, Pvecteur v): allocation et initialisation
 * d'un sommet de denominateur d et de vecteur v; le vecteur v est
 * utilise directement; ca peut introduire du sharing;
 */
Psommet sommet_make(d, v)
int d;
Pvecteur v;
{
    Psommet s;

    s = sommet_new();
    s->denominateur = d;
    s->vecteur = v;
    return (s);
}

/* Psommet sommet_dup(Psommet s_in): allocation et copie de la valeur
 * d'un sommet
 */
Psommet sommet_dup(s_in)
Psommet s_in;
{
    Psommet s_out;

    s_out = (Psommet) MALLOC(sizeof(Ssommet), SOMMET, "sommet_dup");

    if(s_in->eq_sat!=NULL) {
	(void) fprintf(stderr,
		       "sommet_dup: warning eq_sat is not duplicated\n");
    }

    if(s_in->denominateur==0) {
	(void) fprintf(stderr,"sommet_dup: denominateur nul\n");
	abort();
    }

    s_out = (Psommet) MALLOC(sizeof(Ssommet),SOMMET,"sommet_dup");
    s_out->succ = NULL;
    s_out->eq_sat = NULL;
    s_out->vecteur = vect_dup(s_in->vecteur);
    s_out->denominateur = s_in->denominateur;

    return s_out;
}

/* void sommet_rm(Psommet s): desallocation complete d'une structure sommet
 */
void sommet_rm(s)
Psommet s;
{
    vect_rm(s->vecteur);
    FREE((char *)s,SOMMET,"sommet_rm");
}

void dbg_sommet_rm(s,f)
Psommet s;
char *f;
{
    (void) fprintf(stderr,"destruction de sommet dans %s : ",f);
    sommet_fprint(stderr, s, variable_dump_name);
    dbg_vect_rm(s->vecteur,f);
    FREE((char *)s,SOMMET,f);
}

/* void sommet_fprint(FILE * f, Psommet s, char * (*nom_var)()):
 * impression d'un sommet
 */
void sommet_fprint(f, s, nom_var)
FILE * f;
Psommet s;
char * (*nom_var)();
{
    (void) fprintf(f,"denominator = %d\t",s->denominateur);
    vect_fprint(f,s->vecteur,nom_var); 
    /* malgre le clash de type, je kludge... */
   /* ray_dte_fprint(f, s, nom_var);*/
}

/* void sommet_dump(Psommet s): impression d'un sommet sur stderr avec
 * variable_dump_name()
 */
void sommet_dump(s)
Psommet s;
{
    sommet_fprint(stderr, s, variable_dump_name);
}

/* void fprint_lsom(FILE * f, Psommet s, char * (*nom_var)()):
 * impression d'une liste de sommets
 */
void fprint_lsom(f, ls, nom_var)
FILE * f;
Psommet ls;
char * (*nom_var)();
{
    Psommet e;
    for (e = ls; e != NULL; e = e->succ) {
	sommet_fprint(f, e, nom_var);
    }
}

/* void sommet_normalize(Psommet ns): normalisation des coordonnees d'un sommet
 * par le pgcd des coordonnees et du denominateur
 */
void sommet_normalize(ns)
Psommet ns;
{
    int div = vect_pgcd_all(ns->vecteur);

    assert(div>0);
    div = pgcd(div,ns->denominateur);
    ns->denominateur /= div;
    (void) vect_div(ns->vecteur,div);
}

/* boolean som_in_liste(Psommet s, Psommet l): test de l'appartenance
 * du sommet s a la liste de sommets l
 *
 * Les coordonnees du sommet s et des sommets de la liste l sont
 * supposees normalisees (fractions reduites)
 */
boolean som_in_liste(s,listes)
Psommet s;
Psommet listes;
{
    Psommet s1;

    for (s1=listes;s1!=NULL;s1=s1->succ) {
	if ((s1->denominateur)==(s->denominateur)) {
	    if (vect_equal((s1->vecteur),(s->vecteur))) return(TRUE);
	}
    }
    return(FALSE);
}

/* boolean sommet_egal(Psommet s1, Psommet s2): test de l'egalite
 * de representation de deux sommets
 *
 * Il faut en normaliser les coordonnees d'abord si on veut une egalite
 * de valeur
 */
boolean sommet_egal(s1,s2)
Psommet s1,s2;
{
    if (sommet_denominateur(s1)!=sommet_denominateur(s2))
	return(FALSE);
    else
	return(vect_equal(s1->vecteur,s2->vecteur));
}

/* boolean egaliste_s(Psommet l1, Psommet * ad_l2): test d'egalite
 * de listes de sommets
 *
 *	nous proposons un test direct au lieu d'un test en deux etapes 
 *	d'inclusion dans les deux sens; ceci justifie le second parametre
 *	qui est l'adresse du pointeur de liste (Malik Imadache)
 */
boolean egaliste_s(l1,ad_l2)
Psommet l1,*ad_l2;
{
    int egalite;
    Psommet eq1,eq2,eq21,eq23,*ad_aux;
 
    if (l1==(*ad_l2)) return(TRUE);

    /* elements pour lesquels il reste a trouver un "jumeau" dans l1 */
    eq2 = *ad_l2;
    /* adresse a laquelle il faudra raccrocher les elements de l2
       successivement testes */
    ad_aux = ad_l2;

    (*ad_l2) = NULL;

    for(eq1=l1;eq1!=NULL;eq1=eq1->succ)  {
	egalite = 0;
	for(eq21=eq2,eq23=eq2;eq21!=NULL;) {
	    if (sommet_egal(eq21,eq1)) {
		/* on a trouve un element (eq21) de eq2 egal a
		   l'element courant de l1; on remet donc
		   eq21 dans l2, en distingant 2
		   cas suivant qu'il est en tete de
		   eq2 ou pas
		   */
		/* eq23 est le predecesseur de eq21 dans eq2
		   il faut le conserver pour enlever eq21
		   de eq2
		   */
		if (eq21==eq2) {
		    eq2=eq2->succ;
		    eq21->succ = NULL;
		    (*ad_aux) = eq21;
		    ad_aux = &(eq21->succ);
		    eq21 = eq23 = eq2;
		}
		else {
		    eq23->succ = eq21->succ;
		    eq21->succ = NULL;
		    (*ad_aux) = eq21;
		    ad_aux = &(eq21->succ);
		    eq21 = eq23->succ;
		}
		egalite = 1;
		break;
	    }
	    else {
		/* eq21 est different de l'element de l1
		   qui est teste; il faut voir le reste de
		   la liste eq2
		   */
		eq23 = eq21;
		eq21 = eq21->succ;
	    }
	}
	if (egalite == 0) {
	    /* on a trouve un element de l1 qui n'a pas
	       de "jumeau" dans l2 => reformer l2 et sortir
	       */
	    (* ad_aux) = eq2;
	    return(FALSE);
	}
	else egalite = 0;
    }
    if (eq2==NULL)
	/* tous les elements de l1 ont un jumeau
	   (=> l1 est inclus dans l2)
	   si tous les elements de l2 sont jumeaux
	   (inclusion inverse) => egalite
	   */
	return(TRUE);
    else 
	/* dans le cas inverse reformer l2 et sortir */
	(*ad_aux) = eq2;
    return(FALSE);
}


/* void sommet_add(Psommet *ps, Psommet som, int *nb_soms):
 * Ajout d'un sommet a une liste de sommets
 * Le sommet est ajoute a la fin de la liste.
 *
 */
void sommet_add(ps,som,nb_som)
Psommet *ps,som;
int *nb_som;
{

    if (som != NULL)
    {
	Psommet pred,ps1;

	pred = *ps;
	for (ps1 = pred; ps1 != NULL; pred = ps1, ps1 = ps1->succ);
	pred->succ = som;
	som->succ = NULL;
	*nb_som = (*nb_som) +1;
    }
}


