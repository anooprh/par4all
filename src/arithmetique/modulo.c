/* package arithmetique 
 *
 * $RCSfile: modulo.c,v $ (version $Revision$)
 * $Date: 1996/07/18 19:22:06 $, 
 */

/*LINTLIBRARY*/

#include <stdio.h>

#include "assert.h"
#include "arithmetique.h"

/* int modulo_fast(int a, int b): calcul du modulo de a par b;
 * le modulo retourne est toujours positif
 *
 * Il y a quatre configuration de signe a traiter:
 *  1. a>0 && b>0: a % b
 *  2. a<0 && b>0: a % b == 0 ? 0 : b - (-a)%b
 *  3. a>0 && b<0: cf. 1. apres changement de signe de b
 *  4. a<0 && b<0: cf. 2. apres changement de signe de b
 */
Value modulo_fast(Value a, Value b)
{
    /* definition d'une look-up table pour les valeurs de a appartenant
       a [-MODULO_MAX_A..MODULO_MAX_A] et pour les valeurs de b
       appartenant a [1..MODULO_MAX_B] (en fait [-MODULO_MAX_B..MODULO_MAX_B]
       a cause du changement de signe)
       
       Serait-il utile d'ajouter une test b==1 pour supprimer une colonne?
       */
#define MODULO_MAX_A 5
#define MODULO_MAX_B 5
    static Value /* should be generated automatically */
	modulo_look_up[2*MODULO_MAX_A+1][MODULO_MAX_B]= {
	/*  b ==        1  2  3  4  5 */
	{/* a == - 5 */ 0, 1, 1, 3, 0},
        {/* a == - 4 */ 0, 0, 2, 0, 1},
        {/* a == - 3 */ 0, 1, 0, 1, 2},
        {/* a == - 2 */ 0, 0, 1, 2, 3},
        {/* a == - 1 */ 0, 1, 2, 3, 4},
        {/* a ==   0 */ 0, 0, 0, 0, 0},
        {/* a ==   1 */ 0, 1, 1, 1, 1},
        {/* a ==   2 */ 0, 0, 2, 2, 2},
        {/* a ==   3 */ 0, 1, 0, 3, 3},
        {/* a ==   4 */ 0, 0, 1, 0, 4},
        {/* a ==   5 */ 0, 1, 2, 1, 0}
	};
    /* translation de a pour acces a la look-up table */

    Value mod;    /* valeur du modulo C */
    assert(value_notzero_p(b));

    /* premier changement de signe, ne changeant pas le resultat */
    b = value_abs(b);

    /* traitement des cas particuliers */
    /* supprime pour cause de look-up table
     * if(a==1 || a== 0)
     *     return(a);
     *
     * if(b==1) 
     *     return(0);
     */

    if (value_le(a, (Value) MODULO_MAX_A) &&
	value_ge(a, (Value)-MODULO_MAX_A) &&
	value_le(b, (Value) MODULO_MAX_B))
    {
	/* acceleration par une look-up table 
	 */
	mod = modulo_look_up[VALUE_TO_INT(a)+MODULO_MAX_A][VALUE_TO_INT(b)-1];
    }
    else {
	/* calcul effectif du modulo: attention, portabilite douteuse */
	mod = value_mod(a,b);
	mod = value_neg_p(mod)? value_minus(b,mod): mod;
    }

    return mod;
}
