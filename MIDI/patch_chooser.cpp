#include <gtk/gtk.h>
#include <stdio.h>

#include "patch_chooser.h"

// #define DOUBLE_CLIC

/** ============================ raw data ======================= */

// les noms de categories doivent commencer par '_'
// l'element 0 DOIT etre le titre
// l'element 1 DOIT etre 1 categorie
const char * GM_programs[] = {
"General Midi",
"_PIANO",
"Acoustic Grand",
"Bright Acoustic",
"Electric Grand",
"Honky-Tonk",
"Electric Piano 1",
"Electric Piano 2",
"Harpsichord",
"Clavinet",
"_CHROMATIC PERCUSSION",
"Celesta",
"Glockenspiel",
"Music Box",
"Vibraphone",
"Marimba",
"Xylophone",
"Tubular Bells",
"Dulcimer",
"_ORGAN",
"Drawbar Organ",
"Percussive Organ",
"Rock Organ",
"Church Organ",
"Reed Organ",
"Accoridan",
"Harmonica",
"Tango Accordion",
"_GUITAR",
"Nylon String Guitar",
"Steel String Guitar",
"Electric Jazz Guitar",
"Electric Clean Guitar",
"Electric Muted Guitar",
"Overdriven Guitar",
"Distortion Guitar",
"Guitar Harmonics",
"_BASS",
"Acoustic Bass",
"Electric Bass(finger)",
"Electric Bass(pick)",
"Fretless Bass",
"Slap Bass 1",
"Slap Bass 2",
"Synth Bass 1",
"Synth Bass 2",
"_SOLO STRINGS",
"Violin",
"Viola",
"Cello",
"Contrabass",
"Tremolo Strings",
"Pizzicato Strings",
"Orchestral Strings",
"Timpani",
"_ENSEMBLE",
"String Ensemble 1",
"String Ensemble 2",
"SynthStrings 1",
"SynthStrings 2",
"Choir Aahs",
"Voice Oohs",
"Synth Voice",
"Orchestra Hit",
"_BRASS",
"Trumpet",
"Trombone",
"Tuba",
"Muted Trumpet",
"French Horn",
"Brass Section",
"SynthBrass 1",
"SynthBrass 2",
"_REED",
"Soprano Sax",
"Alto Sax",
"Tenor Sax",
"Baritone Sax",
"Oboe",
"English Horn",
"Bassoon",
"Clarinet",
"_PIPE",
"Piccolo",
"Flute",
"Recorder",
"Pan Flute",
"Blown Bottle",
"Skakuhachi",
"Whistle",
"Ocarina",
"_SYNTH LEAD",
"Lead 1 (square)",
"Lead 2 (sawtooth)",
"Lead 3 (calliope)",
"Lead 4 (chiff)",
"Lead 5 (charang)",
"Lead 6 (voice)",
"Lead 7 (fifths)",
"Lead 8 (bass+lead)",
"_SYNTH PAD",
"Pad 1 (new age)",
"Pad 2 (warm)",
"Pad 3 (polysynth)",
"Pad 4 (choir)",
"Pad 5 (bowed)",
"Pad 6 (metallic)",
"Pad 7 (halo)",
"Pad 8 (sweep)",
"_SYNTH EFFECTS",
"FX 1 (rain)",
"FX 2 (soundtrack)",
"FX 3 (crystal)",
"FX 4 (atmosphere)",
"FX 5 (brightness)",
"FX 6 (goblins)",
"FX 7 (echoes)",
"FX 8 (sci-fi)",
"_ETHNIC",
"Sitar",
"Banjo",
"Shamisen",
"Koto",
"Kalimba",
"Bagpipe",
"Fiddle",
"Shanai",
"_PERCUSSIVE",
"Tinkle Bell",
"Agogo",
"Steel Drums",
"Woodblock",
"Taiko Drum",
"Melodic Tom",
"Synth Drum",
"Reverse Cymbal",
"_SOUND EFFECTS",
"Guitar Fret Noise",
"Breath Noise",
"Seashore",
"Bird Tweet",
"Telephone Ring",
"Helicopter",
"Applause",
"Gunshot"
};

const char * Lucina_programs[] = {
"Roland AX09 \"Lucina\"",
"_SYNTH / PAD",
"Saw Lead (M)",
"Brite Square (M)",
"Sync Lead (M)",
"GR Lead (M)",
"JupiterLead (M)",
"ResoSawLead (M)",
"Simple Tri (M)",
"Sine Lead (M)",
"Sqr Lead (M)",
"Octa-Juice",
"Octa-Square",
"Vintager",
"SuperSaw Pad",
"OB Strings",
"Poly Synth 1",
"Poly Synth 2",
"Poly Synth 3",
"Euro Express",
"Soft Pad",
"Hollow Pad",
"Phaser Pad",
"Heaven Pad",
"Cosmic Rays",
"Voyager",
"_PIANO / KEYBOARD",
"Grand Piano",
"E-Grand",
"JD Piano",
"Honky Tonk",
"Phase EP",
"Stage EP",
"FM EP",
"Wurly EP",
"Harpsichord",
"Phase Clav",
"Amped Clav",
"AX Clav",
"Pulse Clav",
"Celesta",
"Music Box",
"JUNO Bell",
"Fantasia",
"Dreaming Box",
"Vibraphone",
"Glocken",
"Marimba",
"Xylophone",
"Steel Drums",
"Santur Stack",
"_ORGAN / ACCORDION",
"VK Organ 1",
"D-Bars 1",
"D-Bars 2",
"Jazz Organ 1",
"Rotary Org 1",
"Vintage Org1",
"VK Organ 2",
"Jazz Organ 2",
"Rotary Org 2",
"Vintage Org2",
"Old Fashion",
"JL Organ 1",
"JL Organ 2",
"Dist Org",
"Positive Org",
"Grand Pipes",
"Musette Acd",
"Accordion 1",
"It.Accordion 1",
"Accordion 2",
"It.Accordion 2",
"Harmonica 1",
"Blues harp",
"Harmonica 2",
"_STRINGS / CHOIR",
"BrightViolin",
"Bright Cello",
"Strings 1",
"Strings 2",
"Staccato",
"Pizzicato",
"JP Strings",
"Synth Str 1",
"Synth Str 2",
"106 Strings",
"JX Strings",
"Sawtooth Str",
"Tape Strings",
"Movie Scene",
"Vox Pad 1",
"Aerial Choir",
"Angels Choir 1",
"Synth Vox 1",
"Vox Pad 2",
"Synth Vox 2",
"Gospel Hum",
"Angels Choir 2",
"3D Vox",
"Jazz Doos",
"_BRASS / W WINDS",
"Trumpet",
"Solo Tb",
"Breathy Sax",
"Soprano Sax",
"Alto Sax",
"Tenor Sax",
"Baritone Sax",
"BrassSection",
"Sax Section",
"Sousaphone",
"80s Brass 1",
"Soft SynBrs",
"80s Brass 2",
"Analog Brass",
"Poly Brass",
"106 Brass",
"Octa Brass",
"Flute",
"Piccolo",
"Clarinet",
"Oboe",
"Pan Pipes",
"Shakuhachi",
"Tape Flute",
"_GUITAR / BASS",
"Nylon Gtr",
"Folk Gtr",
"Jazz E.Gtr",
"Pick E.Gtr",
"Dist Gtr",
"SearingGtr",
"Fuzz Gtr",
"Aerial Harp",
"Sitar",
"Pick E.Bs",
"Finger E.Bs",
"Acoustic Bs",
"Slap Bs",
"Fat Analog (M)",
"Moogue Bs 1 (M)",
"Punch MG (M)",
"Round Bs (M)",
"Moogue Bs 2 (M)",
"Acdg Bs (M)",
"MKS SynBs (M)",
"TB Dist Bs (M)",
"JunoSqu Bs (M)",
"Drums",
"Percussion",
"_Special Tone",
"Synth 1 (Synth Lead) (M)",
"Synth 2 (Poly Synth)",
"Synth Bass (M)",
"Violin",
"Trombone (M)",
"Jazz Scat"
};

/** ============================ call backs ======================= */

/* il faut intercepter le delete event pour que si l'utilisateur
   ferme la fenetre on revienne de la fonction choose() sans engendrer
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


// une callbaque type data_func
// le but c'est de pouvoir inserer un sprintf entre le model et la view en fait
static void fmt_data_call( GtkTreeViewColumn * tree_column,	// sert pas !
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     patch_chooser     * glo )
{
int i;
gchar *text;

// recuperer une donnee dans la colonne 1
// (pairs of column number and value return locations, terminated by -1)
gtk_tree_model_get( tree_model, iter, 1, &i, -1 );

// ici on gere les singularites de Lucina (3 "fine" banks)
int pc, lsb;
if	( i < 128 )
	{ pc = i; lsb = 0; }
else if	( i < 144 )
	{ pc = i - 128; lsb = 1; }	// de "Sitar" a "Percussion"
else	{ pc = i - 144; lsb = 64; }	// special tones

// convertir en texte
if	( lsb == 0 )
	text = g_strdup_printf( "%d", pc+1 );
else	text = g_strdup_printf( "%2d %d", lsb, pc+1 );

// donner cela comme attribut au renderer
g_object_set( rendy, "text", text, NULL );
g_free( text );
// manipuler quelques autres attributs
// ici les categories n'ont pas de valeur i, alors on convient de leur donner
// un i negatif
g_object_set( rendy, "visible", ( i >= 0 ), NULL );
// g_object_set( rendy, "cell-background", "#60FF60", "cell-background-set", TRUE, NULL );
}

#ifndef DOUBLE_CLIC
// une call back pour le simple-clic
// le but principal est de rendre la main si on a clique sur un patch ( i >=0 )
// accessoirement si on clique sur une categorie on veut qu'elle soit expandee
// (et que les autres soient collapsees)
// N.B. ce n'est pas la meme chose que le service d'expansion offert par GTK si on clique
// sur le triangle, c'est en plus.
// Ce qui complique la chose c'est que GTK veut absolument selectionner le premier element
// au demarrage, mais nous on ne veut pas qu'il soit expande
static void select_call ( GtkTreeSelection *treeselection,
		   patch_chooser * glo )
{
GtkTreeIter   iter;
GtkTreeSelection * cursel;
GtkTreeModel * curmod;
int i;

cursel = gtk_tree_view_get_selection( (GtkTreeView*)glo->tarb );
if	( gtk_tree_selection_get_selected( cursel, &curmod, &iter ) )
	{
	gtk_tree_model_get( curmod, &iter, 1, &i, -1);
	// printf("selected %d\n", i );
	if	( i >= 0 )
		{
		glo->patch_number = i; gtk_main_quit();
		}
	else	{
		gtk_tree_view_collapse_all( (GtkTreeView*)glo->tarb );
		if	( glo->tree_first_time )
			{
			glo->tree_first_time = 0;
			gtk_tree_selection_unselect_all( cursel );
			}
		else	{
			GtkTreePath *path;
			path = gtk_tree_model_get_path( curmod, &iter );
			gtk_tree_view_expand_row( (GtkTreeView*)glo->tarb, path, FALSE );
			gtk_tree_path_free( path );
			}
		}
	}
}
#else
/* une call back pour le double-clic
   en fait cela fait la meme chose que le simple clic, on l'a mis en reserve seulement
   pour le cas ou le simple clic ne marcherait pas bien */
static void double_call ( GtkTreeView *curwidg,
		   GtkTreePath *path,
		   GtkTreeViewColumn *col,
		   patch_chooser * glo )
{
GtkTreeModel *curmod;
GtkTreeIter iter;
curmod = gtk_tree_view_get_model(curwidg);
if	( gtk_tree_model_get_iter( curmod, &iter, path ) )
	{
	int i;
	gtk_tree_model_get( curmod, &iter, 1, &i, -1);
	printf("double clic %d\n", i );
	if	( i >= 0 )
		{
		glo->patch_number = i; gtk_main_quit();
		}
	else	{
		gtk_tree_view_collapse_all( (GtkTreeView*)glo->tarb );
		gtk_tree_view_expand_row( (GtkTreeView*)glo->tarb, path, FALSE );
		}
	}
}
#endif

/** ========================= constr. database ===================== */

// le store va etre rempli selon le contenu du tableau de chaines patch_names
// ou les noms de categories sont prefixes par _ (une regle locale a cette appli)
// les valeurs pn (patch number) des elements sont crees par simple incrementation
static GtkTreeStore * makestore( patch_chooser * glo, const char ** patch_names, unsigned int qnames )
{
GtkTreeStore *mymod;
GtkTreeIter   parent_iter, child_iter;
GtkTreeIter  *curiter;


// ici on veut 2 colonnes de 2 types
mymod = gtk_tree_store_new( 2, G_TYPE_STRING, G_TYPE_INT );

unsigned int i; const char * name;
int pn = 0;
for	( i = 1; i < qnames; ++i )	// sauter l'element zero (titre)
	{
	name = patch_names[i];
	if	( name[0] == '_' )
		{			// un top-level parent
		curiter = &parent_iter;
		gtk_tree_store_append( mymod, curiter, NULL );
		gtk_tree_store_set( mymod, curiter, 0, name+1, 1, -1, -1 );
		}
	else	{			// un child
		curiter = &child_iter;
		gtk_tree_store_append( mymod, curiter, &parent_iter );
		gtk_tree_store_set( mymod, curiter, 0, name, 1, pn++, -1 );
		}
	// printf("<%s>\n", patch_names[i] );
	}

return(mymod);
}

/** ============================ constr. GUI ======================= */

static GtkWidget * maketview( patch_chooser * glo )
{
GtkWidget *curwidg;
GtkCellRenderer *renderer;
GtkTreeViewColumn *curcol;
GtkTreeSelection* cursel;

curwidg = gtk_tree_view_new();

// une colonne, sans data_func, qui hebergera ausi les triangles
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, " Instrument " );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
// connecter l'attribut "text" (ou "markup" pour pango markup) a la colonne 0
gtk_tree_view_column_add_attribute( curcol, renderer, "markup", 0 );

gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );

// une colonne, avec data_func
// (c'est la data_func qui choisira la colonne du model)
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, " code " );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         (GtkTreeCellDataFunc)fmt_data_call,
                                         (gpointer)glo, NULL );

gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );


// configurer la selection
cursel = gtk_tree_view_get_selection( (GtkTreeView*)curwidg );
gtk_tree_selection_set_mode( cursel, GTK_SELECTION_SINGLE );

#ifdef DOUBLE_CLIC
// connecter callback pour double-clic */
g_signal_connect( curwidg, "row-activated", (GCallback)double_call, (gpointer)glo );
#else
// connecter callback pour simple-clic, i.e. selection changed
g_signal_connect( cursel, "changed", (GCallback)select_call, (gpointer)glo );
#endif

return(curwidg);
}

// methode principale de la classe patch_chooser
// machine 0 = GM, 1 = Lucina - rend un numero de patch( >=0 ) ou -1 si rien choisi
int patch_chooser::choose( int machine, GtkWindow *parent )
{
const char ** patch_names;
GtkTreeStore * patch_store;

switch	( machine )
	{
	case 0 :
		if	( this->tmod1 == NULL )
			this->tmod1 = makestore( this, GM_programs, sizeof(GM_programs)/sizeof(char *) );
		patch_names = GM_programs; patch_store = this->tmod1;
		break;
	case 1 :
		if	( this->tmod2 == NULL )
			this->tmod2 = makestore( this, Lucina_programs, sizeof(Lucina_programs)/sizeof(char *) );
		patch_names = Lucina_programs; patch_store = this->tmod2;
		break;
	default : return( -1 );
	}

GtkWidget *curwidg;

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );/* DIALOG est deprecated, POPUP est autre chose */
/* ATTENTION c'est serieux : modal veut dire que la fenetre devient la
   seule a capturer les evenements ( i.e. les autres sont bloquees ) */
gtk_window_set_modal( GTK_WINDOW(curwidg), TRUE );
gtk_window_set_position( GTK_WINDOW(curwidg), GTK_WIN_POS_MOUSE );
gtk_window_set_transient_for(  GTK_WINDOW(curwidg), parent );

gtk_window_set_title( GTK_WINDOW(curwidg), patch_names[0] );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( delete_call ), NULL );

gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 0 );
this->wmain = curwidg;

/* creer sous-fenetre scrollable */
curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(curwidg),
				     GTK_SHADOW_ETCHED_IN);
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(curwidg),
				GTK_POLICY_AUTOMATIC,
				GTK_POLICY_AUTOMATIC );
gtk_container_add( GTK_CONTAINER( this->wmain ), curwidg );
this->wscro = curwidg;

// creer la tview
curwidg = maketview( this );
gtk_container_add( GTK_CONTAINER( this->wscro ), curwidg );
this->tarb = curwidg;

// connecter le modele
gtk_tree_view_set_model( (GtkTreeView*)this->tarb, GTK_TREE_MODEL( patch_store ) );
this->tree_first_time = 1;
this->patch_number = -1;

gtk_window_set_default_size( GTK_WINDOW( this->wmain ), 500, 760 );

gtk_widget_show_all( this->wmain );

/* on est venu ici alors qu'on est deja dans 1 boucle gtk_main
   alors donc on en imbrique une autre. Le prochain appel a
   gtk_main_quit() fera sortir de cell-ci (innermost)
 */
gtk_main();
gtk_widget_destroy( this->wmain );

return( this->patch_number );
}
