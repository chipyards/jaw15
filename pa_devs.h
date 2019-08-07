#define	PA_USE_ASIO	1

#ifdef  __cplusplus
extern "C" {
#endif
// appeler cette fonction apres Pa_Initialize();
// retourne le nombre de devices, ou < 0 si erreur (alors c'est un code d'erreur portaudio)
// options : 1 pour sample rates, 2 pour details ASIO, 3 pour tout
int print_pa_devices( int options );

#ifdef  __cplusplus
}
#endif
