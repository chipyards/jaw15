/* popup dialog "modal" pour info ou exception
   il bloque tout le reste.
   modpop2 : version amelioree :
	- popup apparait pres du curseur
	- popup ne peut etre cache par fenetre parente
	- popup ne peut pas etre minimise
	- accepte pango markup dans txt
   mais :
	- gasp utilise variable globale
	  extern GtkWindow * global_main_window;
   modpop3 : ajout modpo_entry pour saisie texte

 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include "modpop3.h"

/* il faut intercepter le delete event pour que si l'utilisateur
   ferme la fenetre on revienne de la fonction modpop() sans engendrer
   de destroy signal. A cet effet ce callback doit rendre TRUE
   (ce que la fonction gtk_main_quit() ne fait pas)
 */

static gint delete_call( GtkWidget *widget,
                  GdkEvent  *event,
                  gpointer   data )
 {
 gtk_main_quit();
 return (TRUE);
 }

void modpop( const char *title, const char *txt, GtkWindow *parent )
{
GtkWidget * curwin;
GtkWidget * curbox;
GtkWidget * curwidg;

curwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );/* DIALOG est deprecated, POPUP est autre chose */
/* ATTENTION c'est serieux : modal veut dire que la fenetre devient la
   seule a capturer les evenements ( i.e. les autres sont bloquees ) */
gtk_window_set_modal( GTK_WINDOW(curwin), TRUE );
gtk_window_set_position( GTK_WINDOW(curwin), GTK_WIN_POS_MOUSE );
gtk_window_set_transient_for(  GTK_WINDOW(curwin), parent );
gtk_window_set_type_hint( GTK_WINDOW(curwin), GDK_WINDOW_TYPE_HINT_DIALOG );

gtk_window_set_title( GTK_WINDOW(curwin), title );
gtk_container_set_border_width( GTK_CONTAINER(curwin), 30 );

gtk_signal_connect( GTK_OBJECT(curwin), "delete_event",
                    GTK_SIGNAL_FUNC( delete_call ), NULL );

/* creer boite verticale */
curbox = gtk_vbox_new( FALSE, 5 );
gtk_container_add( GTK_CONTAINER( curwin ), curbox );
gtk_widget_show(curbox);
/* placer le message (PANGO MARKUP) */
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg), txt );
gtk_label_set_line_wrap( GTK_LABEL(curwidg), TRUE );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0);
gtk_widget_show( curwidg );
/* le bouton ok */
curwidg = gtk_button_new_with_label (" Ok ");
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, FALSE, FALSE, 0);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );
gtk_widget_show(curwidg);
gtk_widget_show( curwin );

/* on est venu ici alors qu'on est deja dans 1 boucle gtk_main
   alors donc on en imbrique une autre. Le prochain appel a
   gtk_main_quit() fera sortir de cell-ci (innermost)
 */
gtk_main();
gtk_widget_destroy( curwin );
}


/* modpopYN : popup modal de confirmation Y/N
 */

// callbacks de modpopYN : pas besoin pour N car la reponse
// par defaut est deja N (ce qui resoud le pb du delete event)

static void Y_call( GtkWidget *widget, int * data )
{
*data = 1; gtk_main_quit();
}

int modpopYN( const char *title, const char *txt, const char *txtY, const char *txtN,
	      GtkWindow *parent )
{
GtkWidget * curwin;
GtkWidget * curbox;
GtkWidget * curwidg;
int resu = 0;

curwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );
gtk_window_set_modal( GTK_WINDOW(curwin), TRUE );
gtk_window_set_position( GTK_WINDOW(curwin), GTK_WIN_POS_MOUSE );
gtk_window_set_transient_for(  GTK_WINDOW(curwin), parent );
gtk_window_set_type_hint( GTK_WINDOW(curwin), GDK_WINDOW_TYPE_HINT_DIALOG );

gtk_window_set_title( GTK_WINDOW(curwin), title );
gtk_container_set_border_width( GTK_CONTAINER(curwin), 30 );

gtk_signal_connect( GTK_OBJECT(curwin), "delete_event",
                    GTK_SIGNAL_FUNC( delete_call ), NULL );

/* creer boite verticale */
curbox = gtk_vbox_new( FALSE, 5 );
gtk_container_add( GTK_CONTAINER( curwin ), curbox );
gtk_widget_show(curbox);
/* placer le message (PANGO MARKUP) */
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg), txt );
gtk_label_set_line_wrap( GTK_LABEL(curwidg), TRUE );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0);
gtk_widget_show( curwidg );
/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 5 );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0);
gtk_widget_show(curwidg);
curbox = curwidg;

/* le bouton N */
curwidg = gtk_button_new_with_label( txtN );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );
gtk_widget_show(curwidg);

/* le bouton Y */
curwidg = gtk_button_new_with_label( txtY );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( Y_call ), &resu );
gtk_widget_show(curwidg);

gtk_widget_show( curwin );

/* on est venu ici alors qu'on est deja dans 1 boucle gtk_main
   alors donc on en imbrique une autre. Le prochain appel a
   gtk_main_quit() fera sortir de cell-ci (innermost)
 */
gtk_main();
gtk_widget_destroy( curwin );
return(resu);
}

/* modpop_entry : popup modal de saisie de parametre (text)
 */
typedef struct
{
char *buf;
unsigned int size;
GtkWidget * entry;
} modpop_entry_struct;

static modpop_entry_struct modpop_entry_data;

// callbacks de modpop_entry : pas besoin pour Cancel car la reponse
// par defaut est deja cancel (ce qui resoud le pb du delete event)
static void Ok_call( GtkWidget *widget, int * data )
{
memcpy( modpop_entry_data.buf, gtk_entry_get_text( GTK_ENTRY(modpop_entry_data.entry) ), modpop_entry_data.size );
modpop_entry_data.buf[ modpop_entry_data.size - 1 ] = 0;
gtk_main_quit();
}

void modpop_entry( const char *title, const char *txt, char *buf, unsigned int size, GtkWindow *parent )
{
GtkWidget * curwin;
GtkWidget * curbox;
GtkWidget * curwidg;

modpop_entry_data.buf = buf;
modpop_entry_data.size = size;

curwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );/* DIALOG est deprecated, POPUP est autre chose */
/* ATTENTION c'est serieux : modal veut dire que la fenetre devient la
   seule a capturer les evenements ( i.e. les autres sont bloquees ) */
gtk_window_set_modal( GTK_WINDOW(curwin), TRUE );
gtk_window_set_position( GTK_WINDOW(curwin), GTK_WIN_POS_MOUSE );
gtk_window_set_transient_for(  GTK_WINDOW(curwin), parent );
gtk_window_set_type_hint( GTK_WINDOW(curwin), GDK_WINDOW_TYPE_HINT_DIALOG );

gtk_window_set_title( GTK_WINDOW(curwin), title );
gtk_container_set_border_width( GTK_CONTAINER(curwin), 30 );

gtk_signal_connect( GTK_OBJECT(curwin), "delete_event",
                    GTK_SIGNAL_FUNC( delete_call ), NULL );

/* creer boite verticale */
curbox = gtk_vbox_new( FALSE, 5 );
gtk_container_add( GTK_CONTAINER( curwin ), curbox );
gtk_widget_show(curbox);
/* placer le message (PANGO MARKUP) */
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg), txt );
gtk_label_set_line_wrap( GTK_LABEL(curwidg), TRUE );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0 );
gtk_widget_show( curwidg );
/* l'entree de texte */
curwidg = gtk_entry_new();
gtk_entry_set_editable( GTK_ENTRY(curwidg), TRUE );
gtk_entry_set_text( GTK_ENTRY(curwidg), buf );
g_signal_connect( curwidg, "activate",
                  G_CALLBACK( Ok_call ), NULL );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0 );
modpop_entry_data.entry = curwidg;
gtk_widget_show(curwidg);
/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 5 );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0);
gtk_widget_show(curwidg);
curbox = curwidg;
/* le bouton Cancel */
curwidg = gtk_button_new_with_label( "Cancel" );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );
gtk_widget_show(curwidg);
/* le bouton Ok */
curwidg = gtk_button_new_with_label( " Ok " );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( Ok_call ), NULL );
gtk_widget_show(curwidg);


gtk_widget_show( curwin );

gtk_main();
gtk_widget_destroy( curwin );
}

// fatal error handling
extern GtkWindow * global_main_window;

void gasp( const char *fmt, ... )
{
char tbuf[1024];
va_list  argptr;
va_start( argptr, fmt );
vsprintf( tbuf, fmt, argptr );
va_end( argptr );
modpop( "fatale erreur", tbuf, global_main_window );
// gtk_main_quit est seulement pour forcer un retour de gtk_main();
// gtk_exit(1);  // gtk_exit() ne peut pas etre intercepte
// ici gtk_widget_destroy() force la sortie de la boucle gtk_main() principale
gtk_widget_destroy( (GtkWidget *)global_main_window );
}

