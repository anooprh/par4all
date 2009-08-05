/*

  $Id$

  Copyright 1989-2009 MINES ParisTech

  This file is part of PIPS.

  PIPS is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.

  PIPS is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.

  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with PIPS.  If not, see <http://www.gnu.org/licenses/>.

*/

 /* package contrainte - allocations et desallocations */

/*LINTLIBRARY*/

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include "boolean.h"
#include "arithmetique.h"
#include "vecteur.h"
#include "contrainte.h"

/* Pcontrainte contrainte_new(): allocation et initialisation d'une contrainte
 * vide
 *
 * Anciens noms: init_eq(), creer_eq
 */
Pcontrainte contrainte_new()
{
    Pcontrainte c;

    /*c = (Pcontrainte)MALLOC(sizeof (Scontrainte),CONTRAINTE,
      "contrainte_new");*/
    c = (Pcontrainte) malloc(sizeof (Scontrainte));
    if (c == NULL) {
	(void) fprintf(stderr,"contrainte_new: Out of memory space\n");
	exit(-1);
    }
    c->eq_sat = NULL;
    c->s_sat = NULL;
    c->r_sat = NULL;
    c->vecteur = NULL;
    c->succ = NULL;

    return(c);
}

/* Pcontrainte contrainte_make(Pvecteur pv): allocation et
 * initialisation d'une contrainte avec un vecteur passe en parametre.
 *
 * Modifications:
 *  - le signe du terme constant n'est plus modifie (FI, 24/11/89)
 */
Pcontrainte contrainte_make(pv)
Pvecteur pv;
{
    Pcontrainte c = contrainte_new();
    contrainte_vecteur(c) = pv;
    return(c);
}

/* Pcontrainte contrainte_dup(Pcontrainte c_in): allocation d'une contrainte
 * c_out prenant la valeur de la contrainte c_in (i.e. duplication
 * d'une contrainte); les tableaux de saturations ne sont pas recopies
 * car on n'en connait pas la dimension. Le lien vers le successeur est
 * aussi ignore pour ne pas cree de sharing intempestif.
 *
 * allocate c_out;
 * c_out := c_in;
 * return c_out;
 *
 * Ancien nom: eq_dup() et cp_eq()
 */
Pcontrainte contrainte_dup(c_in)
Pcontrainte c_in;
{	
    Pcontrainte c_out = NULL;

    if(c_in!=NULL) {
	c_out = contrainte_new();
	c_out->vecteur = vect_dup(c_in->vecteur);
    }
    return(c_out);
}

/* Pcontrainte contraintes_dup(Pcontrainte c_in)
 * a list of constraints is copied
 */
Pcontrainte contraintes_dup(c_in)
Pcontrainte c_in;
{
    Pcontrainte 
	c_tmp = contrainte_dup(c_in),
	c_out = c_tmp,
	c = NULL;

    for (c=(c_in==NULL?NULL:c_in->succ); 
	 c!=NULL; 
	 c=c->succ)
	c_tmp->succ = contrainte_dup(c),
	c_tmp = c_tmp->succ;

    return(c_out);
}



/* Pcontrainte contrainte_free(Pcontrainte c): liberation de l'espace memoire
 * alloue a la contrainte c ainsi que de ses champs vecteur et saturations;
 * seul le lien vers la contrainte suivante est ignore.
 * 
 * Utilisation standard:
 *    c = contrainte_free(c);
 *
 * Autre utilisation possible:
 *    (void) contrainte_free(c);
 *    c = NULL;
 * 
 * comme toujours, les champs pointeurs sont remis a NULL avant la
 * desallocation pour detecter au plus tot les erreurs dues a l'allocation
 * dynamique de memoire.
 *
 * Modification:
 *  - renvoi systematique de CONTRAINTE_NULLE comme valeur de la fonction;
 *    ca permet de diminuer la taille du code utilisateur et d'assurer
 *    plus facilement la mise a CONTRAINTE_NULLE de pointeurs referencant
 *    une zone desallouee (FI, 24/11/89)
 */
Pcontrainte contrainte_free(c)
Pcontrainte c;
{
    if (!CONTRAINTE_UNDEFINED_P(c))
    {
	if (c->eq_sat != NULL) {
	    free((char *)c->eq_sat);
	    c->eq_sat = NULL;
	}

	if (c->r_sat != NULL) {
	    free((char *)c->r_sat);
	    c->r_sat = NULL;
	}

	if (c->s_sat != NULL) {
	    free((char *)c->s_sat);
	    c->s_sat = NULL;
	}

	if (c->vecteur != VECTEUR_UNDEFINED) {
	    vect_rm(c->vecteur);
	    c->vecteur = NULL;
	}

	c->succ = NULL;

	free((char *)c);
    }
    return(CONTRAINTE_UNDEFINED);
}

/* Pcontrainte contraintes_free(Pcontrainte pc): desallocation de toutes les
 * contraintes de la liste pc. 
 * 
 * chaque contrainte est detruite par un appel a contrainte_free.
 *
 * Ancien nom: elim_tte_ineg()
 */
Pcontrainte contraintes_free(pc)
Pcontrainte pc;
{
    while (!CONTRAINTE_UNDEFINED_P(pc)) {
	Pcontrainte pcs = pc->succ;
	(void) contrainte_free(pc);
	pc = pcs;
    }
    return(CONTRAINTE_UNDEFINED);
}

/* void dbg_contrainte_rm(Pcontrainte c): version debug de contrainte rm;
 * trace de la desallocation et impression de la contrainte sur stdout
 */
void dbg_contrainte_rm(c,f)
Pcontrainte c;
char *f;
{
    (void) printf("destruction de EQ dans %s\n",f);
    /*print_eq(c);*/
    dbg_vect_rm(c->vecteur,f);
    /*FREE((char *)c,CONTRAINTE,f);*/
    free((char *)c);
}

/* Have a look at contrainte_dup and contraintes_dup which reverse the
 * order of the list
 * This copy version (including vect_copy, sc_copy) maintains the order
 * (DN,24/6/02) 
 */

Pcontrainte contrainte_copy(c_in)
Pcontrainte c_in;
{	
    Pcontrainte c_out = NULL;

    if(c_in!=NULL) {
	c_out = contrainte_new();
	c_out->vecteur = vect_copy(c_in->vecteur);
    }
    return(c_out);
}

/* Pcontrainte contraintes_copy(Pcontrainte c_in)
 * a list of constraints is copied with the same order
 * In fact, here we only need to replace contrainte_dup by contrainte_copy
 * Have a look at contrainte_copy (DN,24/6/02)
 */
Pcontrainte contraintes_copy(c_in)
Pcontrainte c_in;
{
    Pcontrainte 
	c_tmp = contrainte_copy(c_in),
	c_out = c_tmp,
	c = NULL;

    for (c=(c_in==NULL?NULL:c_in->succ); 
	 c!=NULL; 
	 c=c->succ)
	c_tmp->succ = contrainte_copy(c),
	c_tmp = c_tmp->succ;

    return(c_out);
}
