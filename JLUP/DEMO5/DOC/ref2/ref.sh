# comparaison de fenetres, a la meme Fc que "Castro Fast" brut sans interpolation
# frequence basse ==> long FIR ==> reponse des fenetres proche de l'ideal
# le beta (-B) des versions analytiques a ete ajuste pour matcher Castro
DEMO5/demo5 -Z 32 -P 153.9375 -w 0 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 1 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 2 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 3 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 4 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 5 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 8 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 6 -4
DEMO5/demo5 -Z 32 -P 153.9375 -w 11 -B 10.083 -4
# mid qual, better -L 23
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 0 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 1 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 2 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 3 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 4 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 5 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 9 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 7 -2 
DEMO5/demo5 -Z 84 -P 534.21428571428567 -L 22 -w 11 -B 12.25 -2 
# test decentrage A0
DEMO5/demo5 -Z 32 -F 0.4 -A -0.7 -w 4
DEMO5/demo5 -Z 32 -F 0.4 -A 0.3 -w 4
DEMO5/demo5 -Z 32 -F 0.4 -A 0.5 -w 4
DEMO5/demo5 -Z 32 -F 0.4 -A 0.7 -w 4
# WAV
#DEMO5/demo5 -Z 32 -P 308 -w 3 -o DEMO5/st308clas.wav DEMO5/stmono39s.wav
#DEMO5/demo5 -Z 32 -r 44100 -f 71.590909090909091 -w 3 -o DEMO5/st308gen.wav DEMO5/stmono39s.wav
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
