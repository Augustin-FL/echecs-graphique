# Jeu d'echec en C
Jeu d'échec graphique  graphique codé en C, avec la SDL 1.0, SDL_gfx et SDL_Image

Il a été codé, testé et compilé sous windows 7 (64 bit - GCC). Il permet de jouer à 2 joueurs sur un meme pc.


#Fonctionnalités du programme

- jeu en C avec interface graphique
- chronos et mode "pause"
- affichage des pièces mangés
- affichage des possibilités de déplacement pour une pièce donnée

Vous avez trouvé un bug? n'hésitez pas a déposer une issue sur GitHub ;)


#Compilation 
Pour compiler le programme, il faut installer la SDL 1.2, la librairie SDL_GFX, SDL_TTF, et SDL_IMAGE

les parametres de compilation sont :```-O2 -Wall -D_GNU_SOURCE=1 -Dmain=SDL_main -DNO_STDIO_REDIRECT```
Et ceux du linker sont : ```-lmingw32 -lSDLmain -lSDL  -DNO_STDIO_REDIRECT -lm -lSDL_ttf -lSDL_gfx -lSDL_image```
 
