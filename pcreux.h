/** ========================== search system ========================= */

class pcreux {
public :
// la R.E. compilee
pcre *compiled_re;
// les donnees pour eventuelle erreur de compilation
const char *error;
int erroffset;
// option pour pcre_compile
int options;
// options interessantes :
//	PCRE_NEWLINE_ANY DOS/Unix tolerant
//	PCRE_CASELESS equivalent to Perl’s /i or within a pattern (?i)
//	PCRE_MULTILINE ˆ and $ match internal newlines, equivalent to Perl’s /m option, or (?m)

// le tableau pour les resultats de matching ( 1+N = complete match + substrings )
// les valeurs sont par paire {offset start, offset end} (-1,-1 si substring sautee)
// nombre d'ints = 3*(1+N) soit (1+N) paires de resultats et (1+N) reserves
#define QSUBS 3
#define OVECCOUNT (3*(1+QSUBS))    /* should be a multiple of 3 */
int ovector[OVECCOUNT];
// nombre de matches (complete match + substrings)
int match_cnt;
// offset de depart dans le texte (0 sauf cas de match multiple)
int start;
// texte cible et sa longueur en bytes
const char * letexte;
int lalen;
// constructeurs
pcreux(const char * pattern) : options(PCRE_NEWLINE_ANY|PCRE_MULTILINE), start(0),
  letexte(NULL), lalen(0) {
  // compilons donc
  compiled_re = pcre_compile( pattern, options, &error, &erroffset, NULL );
  }
pcreux(const char * pattern, int opt) : options(opt), start(0),
  letexte(NULL), lalen(0) {
  // compilons donc
  compiled_re = pcre_compile( pattern, options, &error, &erroffset, NULL );
  }
// destructeur
~pcreux() {
  if ( compiled_re )
     pcre_free( compiled_re );       /* Release memory used for the compiled pattern */
  }
// methodes
int matchav() {					// find next
   if ( letexte == NULL )
      return 0;
   // matchons une fois
   match_cnt = pcre_exec( compiled_re, NULL, letexte, lalen, start, PCRE_NOTEMPTY,
			  ovector, OVECCOUNT );
   if ( match_cnt == PCRE_ERROR_NOMATCH )
      return -99;
   if ( match_cnt < 0 )
      return match_cnt;
   // preparer prochain
   start = ovector[1];	// pas de recouvrement
   return match_cnt;
   }
int matchav( const char * texte, int tlen ) {	// find first
   letexte = texte; lalen = tlen; start = 0;
   return matchav();
   }
};
