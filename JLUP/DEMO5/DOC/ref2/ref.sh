# test interpolation de la RI au fudge factor (forte decimation de la RI)
DEMO5/demo5 -Z 32 -P 1.203 -w 6 -4
DEMO5/demo5 -Z 32 -P 1.203 -w 6 -4 -A 0.3
DEMO5/demo5 -Z 32 -P 1.203 -w 6 -4 -A 0.5
DEMO5/demo5 -Z 32 -P 1.203 -w 11 -B 10.083 -4
# test interpolation de la RI avec forte decimation
DEMO5/demo5 -Z 32 -P 3 -w 6 -4
DEMO5/demo5 -Z 32 -P 3 -w 6 -4 -A 0.5
DEMO5/demo5 -Z 84 -P 3 -L 22 -w 7 -2 
#DEMO5/demo5 -Z 32 -P 3 -w 11 -B 10.083 -4
#DEMO5/demo5 -Z 32 -P 3 -w 11 -B 10.083 -4 -A 0.5
DEMO5/demo5 -Z 32 -P 33 -w 6 -4
DEMO5/demo5 -Z 32 -P 33 -w 6 -4 -A 0.5
DEMO5/demo5 -Z 32 -P 33 -w 11 -B 10.083 -4
# test interpolation de la RI avec seulement translation : pas de defaut visible
DEMO5/demo5 -Z 32 -P 153.9375 -w 6 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 6 -4 -A 0.5
DEMO5/demo5 -Z 32 -P 153.9375 -w 11 -B 10.083 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 11 -B 10.083 -4 -A 0.5
# comparaison solutions lo-fi (objectif 40dB)
DEMO5/demo5 -Z 10 -P 1.5 -w 1 -6
DEMO5/demo5 -Z 10 -P 1.5 -w 2 -6
DEMO5/demo5 -Z 10 -P 1.5 -w 5 -6

# comparaison de fenetres, a la meme Fc que "Castro Fast" brut sans interpolation
# frequence basse ==> long FIR ==> reponse des fenetres proche de l'ideal
# le beta (-B) des versions analytiques a ete ajuste pour matcher Castro
DEMO5/demo5 -Z 32 -P 153.9375 -w 0 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 1 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 2 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 3 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 4 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 5 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 6 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 11 -B 10.083 -4
# mid qual, better use -L 22
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 0 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 1 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 2 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 3 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 4 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 5 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 7 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 11 -B 12.25 -2 
# demo decentrage A0
DEMO5/demo5 -Z 32 -F 0.4 -A -0.7 -w 4
DEMO5/demo5 -Z 32 -F 0.4 -A 0.3 -w 4
DEMO5/demo5 -Z 32 -F 0.4 -A 0.5 -w 4
DEMO5/demo5 -Z 32 -F 0.4 -A 0.7 -w 4
# recherche fudge factor de base
DEMO5/demo5 -Z 32 -P 1.04 -w 0 -6
DEMO5/demo5 -Z 32 -P 1.11 -w 1 -6
DEMO5/demo5 -Z 32 -P 1.109 -w 2 -6
DEMO5/demo5 -Z 32 -P 1.185 -w 3 -6
DEMO5/demo5 -Z 32 -P 1.25 -w 4 -6
DEMO5/demo5 -Z 32 -P 1.083 -w 5 -6
DEMO5/demo5 -Z 32 -P 1.2031 -w 6 -6
DEMO5/demo5 -Z 32 -P 1.2031 -w 11 -B 10.083 -6

# WAV filtrage
../wavgen -t I -b 0 -e 6000 -a 0.8 -d 10 DEMO5/lin6000.wav
DEMO5/demo5 -Z 32 -r 44100 -f 2000 -g 4000 -w 5
DEMO5/demo5 -Z 32 -r 44100 -f 2000 -g 4000 -w 5 -o DEMO5/filt-2000-4000.wav DEMO5/lin6000.wav
# WAV resample
../wavgen -t I -b 0 -e 22000 -a 0.8 -d 10 -F DEMO5/lin22k.wav
DEMO5/demo5 -Z 32 -P 1.25 -w 11 -B 10.083 -4
DEMO5/demo5 -Z 32 -P 1.25 -w 11 -B 10.083 -4 -K 1.2 -o DEMO5/resamp1.2.wav DEMO5/lin22k.wav
DEMO5/demo5 -Z 32 -P 1.25 -w 11 -B 10.083 -4 -K 0.8 -o DEMO5/resamp0.8.wav DEMO5/lin22k.wav
DEMO5/demo5 -Z 32 -P 1.25 -w 11 -B 10.083 -4 -K 9.9 -o DEMO5/resamp9.9.wav DEMO5/lin22k.wav
DEMO5/demo5 -Z 32 -P 1.25 -w 11 -B 10.083 -4 -K 0.11 -o DEMO5/resamp0.11.wav DEMO5/lin22k.wav
DEMO5/demo5 -Z 32 -P 1.25 -w 6 -4 -K 1.2 -o DEMO5/resamp1.2i.wav DEMO5/lin22k.wav
DEMO5/demo5 -Z 32 -P 1.25 -w 4 -4 -K 1.2 -o DEMO5/resamp1.2b.wav DEMO5/lin22k.wav
rubberband -f 1.2 -T 1.2 DEMO5/lin22k.wav DEMO5/resamp1.2r.wav
# Lo-Fi pour image
DEMO5/demo5 -Z 8 -P 1.3 -w 5 -6
DEMO5/demo5 -Z 12 -P 1.22 -w 5 -6
DEMO5/demo5 -Z 16 -P 1.164 -w 5 -6
DEMO5/demo5 -Z 16 -P 1.164 -w 5 -6 -K 1.2 -o DEMO5/resimg1.2.wav DEMO5/carr9.wav
DEMO5/demo5 -Z 16 -P 1.164 -w 5 -6 -K 0.8 -o DEMO5/resimg0.8.wav DEMO5/carr9.wav
DEMO5/demo5 -Z 16 -P 1.164 -w 5 -6 -K 0.101 -o DEMO5/resimg0.101.wav DEMO5/carr9.wav
DEMO5/demo5 -Z 16 -P 1.164 -w 5 -6 -K 9.901 -o DEMO5/resimg9.901.wav DEMO5/resimg0.101.wav
DEMO5/demo5 -Z 16 -P 1.164 -w 5 -6 -K 0.101 -o DEMO5/resimg0.101bis.wav DEMO5/resimg9.901.wav
DEMO5/demo5 -Z 16 -P 1.164 -w 5 -6 -K 9.901 -o DEMO5/resimg9.901bis.wav DEMO5/resimg0.101bis.wav
DEMO5/demo5 -Z 12 -P 1.22 -w 5 -6 -K 0.101 -o DEMO5/ressimg0.101.wav DEMO5/carr9.wav
DEMO5/demo5 -Z 12 -P 1.22 -w 5 -6 -K 9.901 -o DEMO5/ressimg9.901.wav DEMO5/ressimg0.101.wav
DEMO5/demo5 -Z 12 -P 1.22 -w 5 -6 -K 0.101 -o DEMO5/ressimg0.101bis.wav DEMO5/ressimg9.901.wav
DEMO5/demo5 -Z 12 -P 1.22 -w 5 -6 -K 9.901 -o DEMO5/ressimg9.901bis.wav DEMO5/ressimg0.101bis.wav
DEMO5/demo5 -Z 8 -P 1.3 -w 5 -6 -K 0.101 -o DEMO5/resssimg0.101.wav DEMO5/carr9.wav
DEMO5/demo5 -Z 8 -P 1.3 -w 5 -6 -K 9.901 -o DEMO5/resssimg9.901.wav DEMO5/resssimg0.101.wav
DEMO5/demo5 -Z 8 -P 1.3 -w 5 -6 -K 0.101 -o DEMO5/resssimg0.101bis.wav DEMO5/resssimg9.901.wav
DEMO5/demo5 -Z 8 -P 1.3 -w 5 -6 -K 9.901 -o DEMO5/resssimg9.901bis.wav DEMO5/resssimg0.101bis.wav

# passe-bande
DEMO5/demo5 -Z 32 -r 44100 -f 4410 -w 4
DEMO5/demo5 -Z 32 -r 44100 -f 4410 -b 2 -w 4
DEMO5/demo5 -Z 32 -r 44100 -f 4410 -g 13230 -w 4
DEMO5/demo5 -Z 32 -r 44100 -f 4410 -g 13230 -w 6
DEMO5/demo5 -Z 32 -r 44100 -F 0.2 -b 2 -w 4
DEMO5/demo5 -Z 32 -r 44100 -F 0.2 -G 0.6 -w 4
# passe bande qui ressemble a passe-bas, et son frere vrai jumeau (magie de la trigo)
DEMO5/demo5 -Z 32 -r 44100 -f 0 -g 4410 -w 4
DEMO5/demo5 -Z 64 -r 44100 -f 4410 -w 4
# passe bande qui ressemble a passe-haut, et son frere vrai jumeau (a 6dB pres)
DEMO5/demo5 -Z 32 -r 44100 -F 0.1 -b 9 -w 4
DEMO5/demo5 -Z 64 -r 44100 -F 0.2 -b 5 -w 4
