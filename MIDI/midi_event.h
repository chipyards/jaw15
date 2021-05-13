#ifndef MIDI_EVENT_H
#define MIDI_EVENT_H

// classe de base pour tout type d'event, wired ou meta
class midi_event {
public :
int ms_timestamp;	// millisecond timestamp
int us_timestamp;	// microsecond timestamp
int mf_timestamp;	// midifile timestamp (variable tempo)
int midistatus;	// le canal est zeroed, sauf pour F7 et FF
int channel;
int midinote;	// ou controller number ou type de meta-event
int vel;	// ou valeur de controle
int flags;
int iext;	// index d'extension : note off (si note on) ou string (si meta-event) ou -1 si rien

// methode
int dump( FILE * fil )
{
fprintf( fil, "%d ", mf_timestamp );
switch( midistatus )
	{
	case 0x80 : fprintf( fil, "Off ch=%d n=%d v=%d\n", channel+1, midinote, vel );
		break;
	case 0x90 : fprintf( fil, "On ch=%d n=%d v=%d\n", channel+1, midinote, vel );
		break;
	case 0xA0 : fprintf( fil, "PoPr ch=%d n=%d v=%d\n", channel+1, midinote, vel );	// poly pressure = note after touch
		break;
	case 0xB0 : fprintf( fil, "Par ch=%d c=%d v=%d\n", channel+1, midinote, vel  );	// controller
		break;
	case 0xC0 : fprintf( fil, "PrCh ch=%d p=%d\n", channel+1, midinote );	// program change
		break;
	case 0xD0 : fprintf( fil, "ChPr ch=%d v=%d\n", channel+1, midinote );	// channel pressure = channel after touch
		break;
	case 0xE0 : fprintf( fil, "Pb ch=%d v=%d\n", channel+1, vel );		// pitch bend
		break;
	case 0xFF :
	switch	( midinote )
		{
		case 0 :
			fprintf( fil, "SeqNr %d\n", vel );
			break;
		case 1 :					// text meta-events
			fprintf( fil, "Meta Text ");
			break;
		case 2 :
			fprintf( fil, "Meta Copyright ");
			break;
		case 3 :
			fprintf( fil, "Meta TrkName ");
			break;
		case 4 :
			fprintf( fil, "Meta InstrName ");
			break;
		case 5 :
			fprintf( fil, "Meta Lyric ");
			break;
		case 6 :
			fprintf( fil, "Meta Marker ");
			break;
		case 7 :
			fprintf( fil, "Meta Cue ");
			break;					// non-text short meta-events
		case 0x20 :	// midi channel prefix
			fprintf( fil, "ChanPrefix %d\n", vel );
			break;
		case 0x21 :	// midi port
			fprintf( fil, "MidiPort %d\n", vel );
			break;
		case 0x2f :	// End of Track
			fprintf( fil, "Meta TrkEnd\n");
			break;
		case 0x51 :	// Set tempo
			fprintf( fil, "Tempo %d\n", vel );
			break;
		case 0x58 : {	// time signature
			char nn = ((char *)(&vel))[3];
			char dd = ((char *)(&vel))[2];
			char cc = ((char *)(&vel))[1];
			char bb = ((char *)(&vel))[0];
			fprintf( fil, "TimeSig %d/%d %d %d\n", nn, 1<<dd, cc, bb );
			} break;
		case 0x59 : {	// key signature
			char sf = ((char *)(&vel))[1];
			char mi = ((char *)(&vel))[0];
			fprintf( fil, "KeySig %d %s\n", (sf>127?sf-256:sf), (mi?"minor":"major") );
			} break;					// non-text long meta-events
		case 0x54 :	// smpte
			fprintf( fil, "SMPTE ");
			break;
		case 0x7f :	// Sequencer Specific
			fprintf( fil, "SeqSpec ");
			break;
		default :
			fprintf( fil, "Meta 0x%02x ", midinote );
		}
	default : ;
	}
return iext;
};

};

// extension d'un event t.q. sysex ou meta-event avec plus de 4 bytes de data
class midi_event_ext {
public :
vector <unsigned char> bytes;		// data

// methode
void dump( FILE * fil, int type )	// type sert a decider si on dumpe en hexa ou en ascii
{
if	( type <= 0x0F )		// on connait des meta texte de 0 a 7, mais de 8 a F seraient reserves pour texte aussi
	{
	fprintf( fil, "\"" );
	for	( unsigned int i = 0; i < bytes.size(); ++i )
		fputc( bytes[i], fil );
	fprintf( fil, "\"" );
	}
else	{
	for	( unsigned int i = 0; i < bytes.size(); ++i )
		fprintf( fil, "%02x ", bytes[i] );
	}
fprintf( fil, "\n" );
};

};



#endif