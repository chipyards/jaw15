#ifdef  __cplusplus
extern "C" {
#endif

/* fatal error handling */

void gasp( const char *fmt, ... );

/* popup dialog "modal" pour info ou exception
   il bloque tout le reste.
 */

void modpop( const char *title, const char *txt, GtkWindow *parent );

/* modpopYN : popup modal de confirmation Y/N
 */
int modpopYN( const char *title, const char *txt, const char *txtY, const char *txtN, GtkWindow *parent );

/* modpop_entry : popup modal de saisie de parametre (text)
 */
void modpop_entry( const char *title, const char *txt, char *buf, unsigned int size, GtkWindow *parent );

#ifdef  __cplusplus
}
#endif
