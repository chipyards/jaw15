# script a executer dans le dir TheKlub17/Save/JSON
# lui passer le num d'un .pes existant et celui du nouveau (possiblement le meme)
# avoir une icone RGBA 256*256 et une copie de ubc.exe 
./ubc -d ../PoseEdit$1.pes -o PoseEdit$1.json
/f/DEV/JAW/JAW15/kawo $3.wav PoseEdit$1.json PoseEdit$2.json
./ubc -e PoseEdit$2.json -o ../PoseEdit$2.pes
cp PoseEdit.icon.png ../PoseEdit$2.icon.png
ls -l ../PoseEdit$1*
ls -l ../PoseEdit$2*
