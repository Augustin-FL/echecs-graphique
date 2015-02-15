Jeu d'echec en C - by gusfl (Augustin Favin-Lévêque)
====================================================


Intro:
------
Ce programme est un jeu d'échec graphique codé en C
Il a été codé, testé et compilé sous windows 7 (64 bit), sans IDE(avec notepad++ et GCC seulement). Il permet de jouer à 2 joueur sur un meme pc.


Setup : 
-------
Pour compiler le programme, il faut installer une version modifié de la SDL 1.2 (accessible ici : https://mega.co.nz/#F!opF1EDIZ!NYp-4ziT41kpjpK9YTxz1Q ), la librairie SDL_GFX, SDL_TTF , et SDL_IMAGE

les parametrès de compilation sont : 
CFLAGS=-O2 -Wall -D_GNU_SOURCE=1 -Dmain=SDL_main -DNO_STDIO_REDIRECT
FLAGS_LINKER=-lmingw32 -lSDLmain -lSDL  -DNO_STDIO_REDIRECT -lm -lSDL_ttf -lSDL_gfx -lSDL_image
 

Features :
---------
-jeu en C avec interface graphique
-chronos et mode "pause"
-affichage des pièces mangés
-affichage des possibilités de déplacement pour une pièce donnée


Bug reporting:
-----------
email : gusfl@free.fr
twitter : @gusfl2