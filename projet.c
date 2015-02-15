#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_rotozoom.h>

/*
compilé avec GCC pour windows(via mingw32)

linké avec :
Librairie SDL 1.2 (-lmingw32 -lSDLmain -lSDL -lm) 
Librairie SDL gfx (-lSDL_gfx)
Librairie SDL image(-lSDL_image)
Librairie SDL ttf (-lSDL_ttf)

*/

typedef struct cases {int x,y, joueur, piece, couleur,a_bouge,a_bouge_au_tour_prececent,position_precedente_x,position_precedente_y;} CASES;

SDL_Surface *ecran, *image;
SDL_TimerID timer;

CASES cases[8][8];
int adoubement[2][4];

int continuer=1;
int jeu_en_pause=0;
int tour=0;
int largeur_fenetre=1400;
int hauteur_fenetre=820;

int chrono_lance=0;
int chronos[2];

#define FPS 20

#define changer_pixel(x,y,couleur) if(dans_ecran(x,y))  *((Uint32 *)ecran->pixels + y*largeur_fenetre + x) = couleur
//macro ->remplace en préprocesseur changer_pixel(x,y,couleur). On fais une directive préprocesseur car + rapide, une fonction "classique" serai trop lente

#define TOUR 135
#define FOU 196
#define CAVALIER 260
#define PION 320
#define REINE 73
#define ROI 10

#define JOUEUR_BLANC 10
#define JOUEUR_NOIR 70

#define gris       0x808080
#define noir       0x000000
#define blanc      0xFFFFFF
#define jaune_vert 0x9ACD32
#define bleu       0x0000FF
#define vert       0x00FF00
#define rouge      0xFF0000
#define violet     0x800080

int main(int argc, char *argv[]);


// ########################
// 1. FONCTIONS "D'ATTENTE"
// ########################

int attendre_choix_piece(int *retour_x, int *retour_y,int joueur);
int attendre_choix_deplacement_piece(int x,int y,int *position_finale_x,int *position_finale_y);
int attendre_choix_adoubement_piece(int x,int y);
int attendre_clic_debut_partie();
int attendre_clic_fin_partie(int message);


// ##########################
// 2. FONCTIONS "D'AFFICHAGE"
// ##########################

void init_plateau();//affiche le plateau la 1ere fois
void afficher_sprite(int x, int y);//affiche le sprite(l'image corresponant à la pièce) qui se trouve sur la case (x,y)
void dessiner_case_damier(int x,int y);//dessine la case (x,y) du damier
void dessiner_rectangle(Uint32 couleur, int x_min,int y_min,int x_max,int y_max);//dessine un rectangle (pixel par pixel)
void dessiner_cercle_case(Uint32 couleur, int x, int y);//dessine un cercle centré dans la case (x,y) du damier
void dessiner_cercle(Uint32 couleur, float rayon, int x, int y);//dessine un cercle(pixel par pixel)
void afficher_sprite_choix_adoubement(int joueur);//affichage des sprites sur le panneau blanc sur le coté(pour l'adoubement)
void afficher_message(int message,int afficher);//affiche/cache un message texte
void afficher_pieces_manges();//calcule et affiche les pieces mangés
void afficher_chronos();//affiche les chronos
void resizer_ecran(int x,int y);//resize l'ecran a la taille (x,y)
void pause(int focus_de_la_fenetre);//met le jeu en pause

// ##############################
// 3. FONCTIONS DE VERIFICATIONS
// ##############################

int verifier_est_en_echec(int joueur,int x,int y,int i,int j);//verifie si echec pour un joueur. si i,j,x,y non nul, simule avec déplacement piece (x,y)->(i,j)
int dans_ecran(int x,int y);//graphique ->fonction de debug, verifie si tout les pixels sont bien dans l'ecran
int est_menace(int x,int y,int joueur);//indique si la piece en (x,y) est menacé par le joueur adverse
int verifier_mat_nul(int joueur);//verifie si 

// ##########################################
// 4. FONCTIONS DE GESTION/ALGORITHME DU JEU
// ##########################################

void obtenir_possibilite_deplacement(int piece,int *possibilite_deplacement,int x,int y,int empecher_roi_recursif);
void obtenir_pieces_en_jeu(int *liste_piece);//donne la liste des pieces encore en jeu
void deplacer_piece(int x,int y,int x_final,int y_final);//déplace une piece

// ########################################
// 5.  FONCTIONS DE GESTION DU CHRONO/TEMPS
// ########################################
Uint32 fonction_callback_chrono(Uint32 intervalle, void *parametre);

int main(int argc, char *argv[])
{
	int decision_joueur_x,decision_joueur_y, decision_joueur_finale_x, decision_joueur_finale_y,joueur, a_joue=0,mat_nul=0;
	
	init_plateau();
	attendre_clic_debut_partie();
	
	while(continuer && mat_nul==0)
	{
		if(tour%2==0) joueur=JOUEUR_BLANC;//les joueurs jouent chacun leur tour : un coup sur 2.
		else joueur=JOUEUR_NOIR;
		a_joue=-1;
		chrono_lance=tour%2+1;
		
		while(a_joue==-1 && continuer)
		{
			attendre_choix_piece(&decision_joueur_x,&decision_joueur_y,joueur);
		
			if(continuer) a_joue=attendre_choix_deplacement_piece(decision_joueur_x,decision_joueur_y,&decision_joueur_finale_x,&decision_joueur_finale_y);
		}

		if(continuer)deplacer_piece(decision_joueur_x,decision_joueur_y,decision_joueur_finale_x,decision_joueur_finale_y);
		
		mat_nul=verifier_mat_nul(joueur);
		
		tour++;
	}
	
	tour--;
	if(continuer) attendre_clic_fin_partie(mat_nul);
	
	return 0;
}

//------------------------
//fonctions d'attente
//-----------------------

int attendre_choix_piece(int *retour_x, int *retour_y,int joueur)
{
    int x=0,y=0, clic = 0,temps_precedent=SDL_GetTicks();
	SDL_Event evenement;
   
	while(continuer)
    {
		SDL_PollEvent(&evenement);
		
		if(evenement.type == SDL_ACTIVEEVENT && evenement.active.state & SDL_APPINPUTFOCUS) pause(evenement.active.gain);
		else if (evenement.type == SDL_MOUSEMOTION)  clic=0;  // Si l'utilisateur a juste bougé la souris
		else if ((evenement.type == SDL_MOUSEBUTTONDOWN) && (evenement.button.button == SDL_BUTTON_LEFT)) clic = 1; // Sinon si l'utilisateur a bougé la souris et cliqué avec le bouton gauche.
		else if(evenement.type == SDL_VIDEORESIZE)resizer_ecran(evenement.resize.w,evenement.resize.h);
		else if(evenement.type == SDL_QUIT)continuer=0;// Sinon si l'utilisateur veut quitter.
		
		if(jeu_en_pause==0)
		{
			for(x=0;x<8;x++)
			{
				for(y=0;y<8;y++)
				{
					
					if(cases[x][y].joueur==joueur)
					{//si l'utilisateur a un pion sur cette case
					
						dessiner_case_damier(x,y);//on redessine la case(on efface la potentielle piece dessus)
						
						if(evenement.button.x >= cases[x][y].x-50 && evenement.button.x <= cases[x][y].x+50
						&&(evenement.button.y >= cases[x][y].y-50 && evenement.button.y <= cases[x][y].y+50)
						)
						{//si la souris de l'utilisateur se trouve sur la case (x,y), et il y a un pion appartenant au joueur sur cette case
						 //ET qu'il peut le déplacer (cela signifie que les pions non deplacables seront quand meme souligné en vert, mais non cliquable

							if (clic)
							{
								*retour_x=x;
								*retour_y=y;
								if(cases[x][y].piece!=0) afficher_sprite(x,y);
								return 1;
							}
							else // Sinon dessine un cercle jaune par dessus la case
							{	
								dessiner_cercle_case(jaune_vert,x,y);	
							}				
						}
						
						if(cases[x][y].piece!=0) afficher_sprite(x,y);
					}
				}
			}
			SDL_Flip(ecran);
		}
		if((SDL_GetTicks()-temps_precedent)<(1000/FPS)) SDL_Delay((1000/FPS)-(SDL_GetTicks()-temps_precedent));
		temps_precedent=SDL_GetTicks();
    }
	return 0;
}

int attendre_choix_deplacement_piece(int x,int y,int *position_finale_x,int *position_finale_y)
{
	
	int possibilite_deplacement[8][8], i,j,k,l,clic=0, piece_indeplacable=1,temps_precedent=SDL_GetTicks();
	SDL_Event evenement;
	
	for(i=0;i<8;i++)
	{
		for(j=0;j<8;j++)
		{
			possibilite_deplacement[i][j]=0; 
		}
	}
	obtenir_possibilite_deplacement(cases[x][y].piece,(int*)possibilite_deplacement,x,y,0);
	
	
	for(i=0;i<8;i++)
	{
		for(j=0;j<8;j++)
		{
			if(possibilite_deplacement[i][j]!=0)
			{
				dessiner_case_damier(i,j);
				if(possibilite_deplacement[i][j]==1) dessiner_cercle_case(bleu,i,j);
				else if(possibilite_deplacement[i][j]==2) dessiner_cercle_case(rouge,i,j);
				else if(possibilite_deplacement[i][j]==3) dessiner_cercle_case(violet,i,j);
				if(cases[i][j].piece!=0) afficher_sprite(i,j);	
				
				if(possibilite_deplacement[i][j]!=0)piece_indeplacable=0;
			}	
		}
	}
	SDL_Flip(ecran);
	
	
	while(continuer)
    {
		SDL_PollEvent(&evenement);
		
		if(evenement.type == SDL_ACTIVEEVENT && evenement.active.state & SDL_APPINPUTFOCUS) pause(evenement.active.gain);
		if (evenement.type == SDL_MOUSEMOTION)  clic=0;  // Si l'utilisateur a juste bougé la souris
		else if ((evenement.type == SDL_MOUSEBUTTONDOWN) && (evenement.button.button == SDL_BUTTON_LEFT)) clic = 1; // Sinon si l'utilisateur a bougé la souris et cliqué avec le bouton gauche.
		else if(evenement.type == SDL_QUIT)continuer = 0;
		else if(evenement.type == SDL_VIDEORESIZE)resizer_ecran(evenement.resize.w,evenement.resize.h);
		else if(evenement.type == SDL_KEYDOWN && evenement.key.keysym.sym == SDLK_ESCAPE || piece_indeplacable)
		{
			if(jeu_en_pause==0)
			{
				for(i=0;i<8;i++)
				{
					for(j=0;j<8;j++)
					{
						dessiner_case_damier(i,j);
						if(cases[i][j].piece!=0) afficher_sprite(i,j);
					}
				}
				return -1;
			}
		}
		
		if(jeu_en_pause==0)
		{
			for(i=0;i<8;i++)
			{
				for(j=0;j<8;j++)
				{
					dessiner_case_damier(i,j);
					if(possibilite_deplacement[i][j]==1) dessiner_cercle_case(bleu,i,j);
					else if(possibilite_deplacement[i][j]==2) dessiner_cercle_case(rouge,i,j);
					else if(possibilite_deplacement[i][j]==3) dessiner_cercle_case(violet,i,j);
					
					if(possibilite_deplacement[i][j]!=0
					&& evenement.button.x >= cases[i][j].x-50 && evenement.button.x <= cases[i][j].x+50
					&& evenement.button.y >= cases[i][j].y-50 && evenement.button.y <= cases[i][j].y+50)
					{
						
						if (clic)
						{
							*position_finale_x=i;
							*position_finale_y=j;
							
							return 1;
						}
						else // Sinon dessine un cercle jaune par dessus la case
						{	
							dessiner_cercle_case(jaune_vert,i,j);	
						}
						
					}
					if(cases[i][j].piece!=0) afficher_sprite(i,j);
				}
			}
			SDL_Flip(ecran);
		}
		if((SDL_GetTicks()-temps_precedent)<(1000/FPS)) SDL_Delay((1000/FPS)-(SDL_GetTicks()-temps_precedent));
		temps_precedent=SDL_GetTicks();
	}
	return 0;
}

int attendre_choix_adoubement_piece(int x,int y)
{
	int i,k=0,l=0,clic=0;
	SDL_Event evenement;
	
	chrono_lance=0;
	
	dessiner_rectangle(blanc,100*9+50,50,100*12+50,150);

	afficher_sprite_choix_adoubement(cases[x][y].joueur);
	afficher_message(8,1);

	while(continuer)
    {
		SDL_WaitEvent(&evenement);
		
		if(evenement.type == SDL_ACTIVEEVENT && evenement.active.state & SDL_APPINPUTFOCUS)
		{
			pause(evenement.active.gain);
			chrono_lance=0;
			if(evenement.active.gain==1) afficher_message(8,1);
			
		}
		else if (evenement.type == SDL_MOUSEMOTION)  clic=0;  // Si l'utilisateur a juste bougé la souris
		else if ((evenement.type == SDL_MOUSEBUTTONDOWN) && (evenement.button.button == SDL_BUTTON_LEFT)) clic = 1; // Sinon si l'utilisateur a bougé la souris et cliqué avec le bouton gauche.
		else if(evenement.type == SDL_VIDEORESIZE)
		{
			resizer_ecran(evenement.resize.w,evenement.resize.h);
			afficher_message(8,1);
		}
		else if(evenement.type == SDL_QUIT)continuer=0;// Sinon si l'utilisateur veut quitter.
		
		if(jeu_en_pause==0)
		{			
			dessiner_rectangle(blanc,100*9+50,50,100*12+50,150);
			
			for(i=0;i<4;i++)
			{
				
				if(evenement.button.x >= 962+i*75  && evenement.button.x <= 1012+i*75 
				&& evenement.button.y >=50 && evenement.button.y <= 150)
				{
					if(clic)
					{
						if(i==0) cases[x][y].piece=CAVALIER;
						else if(i==1) cases[x][y].piece=FOU;
						else if(i==2) cases[x][y].piece=TOUR;
						else if(i==3) cases[x][y].piece=REINE;
						if(cases[x][y].joueur==JOUEUR_NOIR)adoubement[0][i]++;
						else adoubement[1][i]++;
						
						dessiner_rectangle(noir,100*9+50,50,100*12+50,150);//on efface la zone de choix
						afficher_message(8,0);//et le texte
						chrono_lance=tour%2+1;
						return 1;
					}
					else  dessiner_cercle(jaune_vert,37.5,987.0+i*75.0,100);
				}
			}
			
			afficher_sprite_choix_adoubement(cases[x][y].joueur);
		}
		SDL_Flip(ecran);
	}
	return 0;
}

int attendre_clic_debut_partie()
{
	int ok=0;
	SDL_Event evenement;
	
	afficher_message(0,1);
	SDL_Flip(ecran);
	while(SDL_WaitEvent(&evenement) && !ok && continuer)
	{
	
		if ((evenement.type == SDL_MOUSEBUTTONDOWN) && (evenement.button.button == SDL_BUTTON_LEFT)) ok=1;
		else if(evenement.type == SDL_VIDEORESIZE)
		{
			resizer_ecran(evenement.resize.w,evenement.resize.h);
			afficher_message(0,1);
			SDL_Flip(ecran);
		}
		else if(evenement.type == SDL_QUIT)continuer=0;// Sinon si l'utilisateur veut quitter.
	}
	afficher_message(0,0);
	return continuer;
}

int attendre_clic_fin_partie(int message)
{
	int ok=0;
	SDL_Event evenement;
	 
	
	afficher_message(message,1);
	if(message==3 ||message==4) afficher_message(message+2,1);
	
	SDL_RemoveTimer(timer);
	
	SDL_Flip(ecran);
	while(SDL_WaitEvent(&evenement) && !ok && continuer)
	{
	
		if ((evenement.type == SDL_MOUSEBUTTONDOWN) && (evenement.button.button == SDL_BUTTON_LEFT)) ok=1;
		else if(evenement.type == SDL_VIDEORESIZE)
		{
			resizer_ecran(evenement.resize.w,evenement.resize.h);
			afficher_message(message,1);
			afficher_message(message,1);
			SDL_Flip(ecran);
		}
		else if(evenement.type == SDL_QUIT)continuer=0;// Sinon si l'utilisateur veut quitter.
	}
	return continuer;


}

//---------------------
//fonctions graphiques
//---------------------
void init_plateau()
{
	int x=0,y=0;
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);//lancement des moteurs SDL graphique+timer, creation de la fenêtre.
	ecran = SDL_SetVideoMode(largeur_fenetre, hauteur_fenetre, 32, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_RESIZABLE);
	SDL_WM_SetCaption("Jeu d'echec en C - Augustin F.L",NULL);
	
	dessiner_rectangle(noir,0,0,largeur_fenetre-1,hauteur_fenetre-1);
	
	image=SDL_LoadBMP("pieces.bmp");
	SDL_SetColorKey(image, SDL_SRCCOLORKEY, SDL_MapRGB(image->format, 255, 255, 255));

	SDL_SetAlpha(image, SDL_SRCALPHA, 200);
	//la transparance de l'image
	
	for(x=0;x<2;x++)
	{
		chronos[x]=0;
		for(y=0;y<4;y++)
		{
			adoubement[x][y]=0;
		}
	}
	
	timer=SDL_AddTimer(950,fonction_callback_chrono,NULL);//on met un peut moins que 1 sec: possible retard de l'os
	
	
	for(x=0;x<8;x++)
	{
		for(y=0;y<8;y++)
		{
			cases[x][y].x=45+x*100+50+x;
			cases[x][y].y=10+y*100+50+y;
			
			dessiner_case_damier(x,y);
			
			cases[x][y].piece=0;
			cases[x][y].joueur=0;
			cases[x][y].a_bouge=0;
			cases[x][y].a_bouge_au_tour_prececent=0;
			cases[x][y].position_precedente_x=0;
			cases[x][y].position_precedente_y=0;
			
			if(y==0 || y==7)
			{
				if(x==0 || x==7 ) cases[x][y].piece=TOUR; //tour
				else if(x==1 || x==6) cases[x][y].piece=CAVALIER; //cavalier
				else if(x==2 || x==5) cases[x][y].piece=FOU; //fou
				else if(x==3) cases[x][y].piece=REINE;//reine
				else if(x==4) cases[x][y].piece=ROI;//roi
			}
			else if(y==1 || y==6) cases[x][y].piece=PION;//pion
			
			
			dessiner_rectangle(blanc,100*9,310,100*13,510);
			
			if(cases[x][y].piece !=0)
			{
				if(y==6 || y==7) cases[x][y].joueur=JOUEUR_BLANC; //blanc
				else if(y==0 || y==1) cases[x][y].joueur=JOUEUR_NOIR;//noir
				afficher_sprite(x,y); 
			}
		}
	}
	afficher_chronos();
}

void afficher_sprite(int x, int y)
{
	if(!(x>=0 && x<8 && y>=0 && y<8))
	{
		printf("erreur ! dessiner_case_damier() : x et y invalides. x=¨%d, y=%d\n",x,y);
		return ;
	}
	
	SDL_Rect position_image,sprite;

	sprite.w = 50; 
	sprite.h = 50;
	sprite.x=cases[x][y].piece;
	sprite.y=cases[x][y].joueur;//
	
	position_image.x=cases[x][y].x-25;
	position_image.y=cases[x][y].y-25;//cooordonnées inversées par rapport a l'orrigine. (0,0)= en bas a gauche		

	SDL_BlitSurface(image, &sprite, ecran, &position_image);
}

void dessiner_case_damier(int x,int y)
{
	if(!(x>=0 && x<8 && y>=0 && y<8))
	{
		printf("erreur ! dessiner_case_damier() : x et y invalides. x=¨%d, y=%d\n",x,y);
		return ;
	}
	
	int xmin=cases[x][y].x-50, xmax=cases[x][y].x+50, ymin=cases[x][y].y-50, ymax=cases[x][y].y+50, i=0, j=0;
	Uint32 couleur;
	if((x+y)%2==0) couleur=gris;
	else couleur=blanc;
	dessiner_rectangle(couleur,xmin,ymin,xmax,ymax);
}

void dessiner_rectangle(Uint32 couleur, int x_min,int y_min,int x_max,int y_max)
{
	int x,y;

	for (x=x_min;x<=x_max;x++)
	{
		for (y=y_min;y<=y_max;y++)
		{
			changer_pixel(x,y,couleur);
		}
	}
}

void dessiner_cercle_case(Uint32 couleur, int x, int y)
{
	if(x>=0 && x<8 && y>=0 && y<8)  dessiner_cercle(couleur, 37.5,cases[x][y].x,cases[x][y].y);
	else printf("erreur ! dessiner_cercle_case() a recu des coordonnées invalides. x=%d, y=%d\n",x,y);
}


void dessiner_cercle(Uint32 couleur, float rayon, int x, int y)
{
	float dx=0.0, dy=0.0;
	int i,j;

	for(i=x-rayon; i<=x+rayon; i++)
	{
		dx = i - x;
		for (j=y-rayon; j<=y+rayon;j++)
		{
			dy = j - y;
			if(dx*dx + dy*dy <= rayon*rayon) changer_pixel(i,j,couleur);
		}
	}
}

void afficher_sprite_choix_adoubement(int joueur)
{
	SDL_Rect position_image,sprite;
	int i;
	sprite.w = 50; 
	sprite.h = 50;
	sprite.y=joueur;
	position_image.y=75;
	
	for(i=0;i<4;i++)
	{
		if(i==0) sprite.x=CAVALIER;
		else if(i==1) sprite.x=FOU;
		else if(i==2) sprite.x=TOUR;
		else if(i==3) sprite.x=REINE;
		position_image.x=962+i*75;
		SDL_BlitSurface(image, &sprite, ecran, &position_image);
	}
}


void afficher_message(int message,int afficher)
{
	int ok=0,taille_police=100, hauteur_max_texte=45, largeur_max_texte=350;
	char *texte;
	SDL_Surface *texte_sdl;
	SDL_Rect position_texte;
	TTF_Font *police;
	SDL_Color couleur;
		
	couleur.r=255;//couleur : blanc
	couleur.g=255;
	couleur.b=255;
	
	position_texte.x=940;
	position_texte.y=550;
	TTF_Init();
	if(message==0)       texte="Cliquez pour démarrer la partie";
	else if(message==1)  texte="Roi noir en échec !";
	else if(message==2)  texte="Roi blanc en échec !";
	else if(message==3)  texte="match nul !";
	else if(message==4)  texte="Roi noir en échec et mat !";
	else if(message==5)  texte="Roi blanc en échec et mat !";
	else if(message==6)  texte="le joueur blanc gagne";
	else if(message==7)  texte="le joueur noir gagne";
	else if(message==8)  texte="en quoi le pion dois il être adoubé?";
	
	
	if(message==0) position_texte.x-=10;
	else if(message==1 || message==2) position_texte.x-=5;
	else if(message==3)  position_texte.x+=60;
	else if(message==6 || message==7)
	{
		position_texte.x-=7;
		position_texte.y+=50;
	}
	else if(message==8)
	{
		largeur_max_texte=360;
		hauteur_max_texte=46;
		
		position_texte.x-=5;
		position_texte.y-=380;
	}
	
	dessiner_rectangle(noir,position_texte.x,position_texte.y,position_texte.x+largeur_max_texte,position_texte.y+hauteur_max_texte);

	if(afficher)
	{
		while(!ok && taille_police>2)
		{
			police = TTF_OpenFont("verdana.ttf",taille_police);	
			texte_sdl = TTF_RenderText_Blended(police,texte,couleur);
			
			taille_police--;
			if(texte_sdl->w<largeur_max_texte && texte_sdl->h<hauteur_max_texte) ok=1;
		}
		
		SDL_BlitSurface(texte_sdl, NULL, ecran, &position_texte);
	}
	
	SDL_Flip(ecran);
}


void afficher_pieces_manges()
{	
	SDL_Rect position_image,sprite;
	int x,y,i,j,k,liste_pieces_en_jeu[2][5],nb_pion_max,nb_adoubement=0,decalage=0;
	
	dessiner_rectangle(blanc,100*9,310,100*13,510);
	
	sprite.w = 50; 
	sprite.h = 50;
	
	obtenir_pieces_en_jeu((int*)liste_pieces_en_jeu);
	
	
	for(i=0;i<2;i++)//affichage
	{
		if(i==1)sprite.y=JOUEUR_BLANC;
		else sprite.y=JOUEUR_NOIR;
		nb_adoubement=0;
		for(j=0;j<4;j++)  nb_adoubement+=adoubement[i][j];

		for(j=0;j<5;j++)
		{
			if(j==0||j==1||j==2)nb_pion_max=2+adoubement[i][j];
			else if(j==3) nb_pion_max=8-nb_adoubement;
			else if(j==4) nb_pion_max=1+adoubement[i][j-1];
			
			for(k=0;k<nb_pion_max-liste_pieces_en_jeu[i][j];k++)
			{
				position_image.x=900+k*50;
				position_image.y=410-100*i;//si le joueur est blanc : on enleve 100
				
				if(j==0)sprite.x=CAVALIER;
				else if(j==1)sprite.x=FOU;
				else if(j==2)sprite.x=TOUR;
				else if(j==3)sprite.x=PION;
				else if(j==4)sprite.x=REINE;
				
				switch(j)
				{
					case 4://reine
						position_image.x+=100;
					case 2://tour
						position_image.x+=100;
					case 1://fou
						position_image.x+=100;
					case 0://cavalier
						break;
						
					case 3://pion
						position_image.y+=50;
						break;
				}
				if(j==4 && adoubement[i][j-1]!=0 && k>0)
				{
					position_image.x=900+(8-k)*50;//
					position_image.y=410-100*i+50;
				}
				else if(j!=4 && j!=3 && adoubement[i][j]!=0 && k>1)
				{
					decalage=adoubement[i][3];
					if(j==0)decalage+=adoubement[i][1];
					if(j==1||j==0)decalage+=adoubement[i][2];
					
					position_image.x=900+(8-k-decalage+1)*50;
					position_image.y=410-100*i+50;
				}
				
				SDL_BlitSurface(image, &sprite, ecran, &position_image);
			}
		}
		
	}
}

void afficher_chronos()
{
	char texte[18];
	int i,heures, minutes,secondes,ok=0,taille_police=50;
	
	SDL_Surface *texte_sdl;
	SDL_Rect position_texte;
	TTF_Font *police;
	SDL_Color couleur;
	
	couleur.r=255;//couleur : blanc
	couleur.g=255;
	couleur.b=255;
	
	TTF_Init();
		
	for(i=0;i<2;i++)
	{
		secondes=chronos[i];
		heures=secondes/3600;
		secondes %= 3600;
		minutes=secondes/60;
		secondes %= 60;
		
		if(i==0)sprintf(texte,"Blancs : %02d:%02d:%02d",heures,minutes,secondes);
		else if(i==1)sprintf(texte,"Noirs : %02d:%02d:%02d",heures,minutes,secondes);
		
		while(!ok && taille_police>2)
		{
			police = TTF_OpenFont("verdana.ttf",taille_police);
				
			texte_sdl = TTF_RenderText_Blended(police,texte,couleur);
			
			taille_police--;
			if(texte_sdl->w<350 && texte_sdl->h<45) ok=1;
		}
		texte_sdl = TTF_RenderText_Blended(police,texte,couleur);
			
		position_texte.x=930+i*10;
		position_texte.y=750-i*100;
		
		dessiner_rectangle(noir,930+i*10,750-i*100,930+i*10+350,750-i*100+45);
		SDL_BlitSurface(texte_sdl, NULL, ecran, &position_texte);
	}
}

void resizer_ecran(int x,int y)
{	
	int joueur;
	largeur_fenetre=x;
	hauteur_fenetre=y;
	ecran = SDL_SetVideoMode(x,y, 32, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_RESIZABLE);//etape 1 : on rezize la fenetre
	
	dessiner_rectangle(noir,0,0,largeur_fenetre-1,hauteur_fenetre-1);
	
	for(x=0;x<8;x++)//etape 2 : on redessine le damier
	{
		for(y=0;y<8;y++)
		{
			dessiner_case_damier(x,y);
			if(cases[x][y].piece!=0) afficher_sprite(x,y);
		}
	}
	
	if(tour%2==0) joueur=JOUEUR_BLANC;//les joueurs jouent chacun leur tour : un coup sur 2.
	else joueur=JOUEUR_NOIR;
	
	afficher_pieces_manges();
	if(verifier_est_en_echec(joueur,-1,-1,-1,-1))
	{
		if(joueur==JOUEUR_NOIR)afficher_message(1,1);
		else afficher_message(2,1);
	}
	afficher_chronos();
	
	
}

void pause(int focus_de_la_fenetre)
{
	int ok=0,taille_police=100;
	SDL_Surface *texte;
	SDL_Rect position_texte;
	TTF_Font *police;
	SDL_Color couleur;
			
	couleur.r=255;//couleur : blanc
	couleur.g=255;
	couleur.b=255;
	
	if(focus_de_la_fenetre==0)
	{
		jeu_en_pause=1;
		chrono_lance=0;
		SDL_WM_SetCaption("Jeu d'echec en C - Augustin F.L - En pause",NULL);
		dessiner_rectangle(gris,0,0,largeur_fenetre-1,hauteur_fenetre-1);
	
		TTF_Init();
		
		while(!ok && taille_police>2)
		{
			police = TTF_OpenFont("verdana.ttf",taille_police);
			
			texte = TTF_RenderText_Blended(police, "pause",couleur);
			
			taille_police--;
			if(texte->w<largeur_fenetre/3 && texte->h<hauteur_fenetre/3) ok=1;
		}
		
			position_texte.x=largeur_fenetre/2-(texte->w/2);
			position_texte.y=hauteur_fenetre/2-(texte->h/2);
			SDL_BlitSurface(texte, NULL, ecran, &position_texte);
			SDL_Flip(ecran);
	}
	else
	{
		jeu_en_pause=0;
		resizer_ecran(largeur_fenetre,hauteur_fenetre);
		SDL_WM_SetCaption("Jeu d'echec en C - Augustin F.L", NULL );
		chrono_lance=tour%2+1;
	}
}



int verifier_est_en_echec(int joueur,int x,int y,int i,int j)
{
	int k,l,joueur_deplace,piece_deplace,a_bouge_deplace,a_bouge_au_tour_prececent_deplace,est_en_echec=0,position_precedente_x,position_precedente_y;

	if(x!=-1 && y!=-1 && i!=-1 && j!=-1)
	{
		joueur_deplace=cases[i][j].joueur;
		piece_deplace=cases[i][j].piece;
		a_bouge_au_tour_prececent_deplace=cases[i][j].a_bouge_au_tour_prececent;
		a_bouge_deplace=cases[i][j].a_bouge;
		position_precedente_x=cases[i][j].position_precedente_x;
		position_precedente_y=cases[i][j].position_precedente_y;
		
		cases[i][j].piece=cases[x][y].piece;
		cases[i][j].joueur=cases[x][y].joueur;
		cases[i][j].a_bouge=cases[x][y].a_bouge;
		cases[i][j].a_bouge_au_tour_prececent=cases[x][y].a_bouge_au_tour_prececent;
		cases[i][j].position_precedente_x=cases[x][y].position_precedente_x;
		cases[i][j].position_precedente_y=cases[x][y].position_precedente_y;
		
		cases[x][y].joueur=0;
		cases[x][y].piece=0;
		cases[x][y].a_bouge=0;
		cases[x][y].a_bouge_au_tour_prececent=0;
		cases[x][y].position_precedente_x=0;
		cases[x][y].position_precedente_y=0;
	}
	
	for(k=0;k<8;k++)
	{
		for(l=0;l<8;l++)
		{
			if(cases[k][l].piece==ROI && cases[k][l].joueur==joueur)
			{
				if(est_menace(k,l,joueur)) est_en_echec=1;
				else est_en_echec=0;
			}
		}
	}
	
	if(x!=-1 && y!=-1 && i!=-1 && j!=-1)
	{
		cases[x][y].piece=cases[i][j].piece;
		cases[x][y].joueur=cases[i][j].joueur;
		cases[x][y].a_bouge_au_tour_prececent=cases[i][j].a_bouge_au_tour_prececent;
		cases[x][y].a_bouge=cases[i][j].a_bouge;
		cases[x][y].position_precedente_x=cases[i][j].position_precedente_x;
		cases[x][y].position_precedente_y=cases[i][j].position_precedente_y;
		
		cases[i][j].piece=piece_deplace;
		cases[i][j].joueur=joueur_deplace;
		cases[i][j].a_bouge_au_tour_prececent=a_bouge_au_tour_prececent_deplace;
		cases[i][j].a_bouge=a_bouge_deplace;
		cases[i][j].position_precedente_x=position_precedente_x;
		cases[i][j].position_precedente_y=position_precedente_y;
	}
	
	return est_en_echec;
	
}

int dans_ecran(int x, int y)
{
	if(x<0 || x>=largeur_fenetre) printf("attention ! demande d'affichage d'un pixel hors ecran : x=%d\n",x);
	if(y<0 || y>=hauteur_fenetre)  printf("attention ! demande d'affichage d'un pixel hors ecran : y=%d\n",y);
	if(x<0 || x>=largeur_fenetre || y<0 || y>=hauteur_fenetre)return 0;
	return 1;
}

int est_menace(int x,int y,int joueur)
{
	int i,j, est_menace[8][8],joueur_adverse=0, debug=0, 
	v,w,est_menace_debug[8][8];//pour le debug uniquement
	
	if(joueur==JOUEUR_BLANC) joueur_adverse=JOUEUR_NOIR;
	else joueur_adverse=JOUEUR_BLANC;
	
	for(i=0;i<8;i++) 
	{
		for(j=0;j<8;j++)					
		{
			if(debug)
			{
				dessiner_case_damier(i,j);
				est_menace_debug[i][j]=0;
				if(cases[i][j].piece !=0)  afficher_sprite(i,j); 
			}
			est_menace[i][j]=0;
		}
	}
	
	for(i=0;i<8;i++)
	{	
		for(j=0;j<8;j++) 
		{
			if(cases[i][j].joueur==joueur_adverse)//joueur= le joueur adverse
			{
				if(debug)for(w=0;w<8;w++)for(v=0;v<8;v++)est_menace_debug[w][v]=0;
				
				obtenir_possibilite_deplacement(cases[i][j].piece,(int*)est_menace,i,j,1);

				if(debug)
				{
					obtenir_possibilite_deplacement(cases[i][j].piece,(int*)est_menace_debug,i,j,1);
					dessiner_cercle_case(noir,i,j);
							
					for(w=0;w<8;w++)
					{
						for(v=0;v<8;v++)					
						{	
							dessiner_case_damier(w,v);
							if(est_menace_debug[w][v]==2) dessiner_cercle_case(rouge,w,v);
							if(est_menace_debug[w][v]==1) dessiner_cercle_case(bleu,w,v);
							if(cases[w][v].piece !=0)  afficher_sprite(w,v); 
						}
					}
					dessiner_cercle_case(vert,i,j);
					SDL_Flip(ecran);
					//wait_clic();
				}
			}
		}
	}
	
	if(est_menace[x][y]==2) return 1;
	else return 0;
}

int verifier_mat_nul(int joueur)
{	
	int x,y,i,j, nul=0,echec=0,pat=0, possibilite_deplacement[8][8],joueur_adverse,nb_pieces_en_jeu[2],nb_cavalier[2],nb_fou[2];
	
	if(joueur==JOUEUR_BLANC) joueur_adverse=JOUEUR_NOIR;
	else joueur_adverse=JOUEUR_BLANC;
	
	for(i=0;i<2;i++)
	{
		nb_pieces_en_jeu[i]=0;
		nb_cavalier[i]=0;
		nb_fou[i]=0;
	}
	
	
	for(x=0;x<8;x++)//si il reste en jeu une piece autre que les rois
	{
		for(y=0;y<8;y++)
		{
			possibilite_deplacement[x][y]=0;
			if(cases[x][y].joueur==JOUEUR_BLANC)i=0;
			else i=1;
			
			if(cases[x][y].piece!=0)nb_pieces_en_jeu[i]++;
			if(cases[x][y].piece==CAVALIER)nb_cavalier[i]++;
			else if(cases[x][y].piece==FOU)nb_fou[i]++;
		}
	}
	
	if( (nb_pieces_en_jeu[0]==1 && nb_pieces_en_jeu[1]==1) || // si les deux joueurs n'ont que un roi 
		(nb_pieces_en_jeu[0]==2 && nb_cavalier[0]==1 && nb_pieces_en_jeu[1]==2 && nb_cavalier[1]==1 ) ||// OU un roi et un cavalier
		(nb_pieces_en_jeu[0]==3 && nb_cavalier[0]==2 && nb_pieces_en_jeu[1]==3 && nb_cavalier[1]==2 ) ||//OU un roi et deux cavaliers
		(nb_pieces_en_jeu[0]==2 && nb_fou[0]==1 && nb_pieces_en_jeu[1]==2 && nb_fou[1]==1) ) //OU un roi et un fou 
	 nul=1;//match nul
	
	
	echec=verifier_est_en_echec(joueur_adverse,-1,-1,-1,-1);

	pat=1;
	for(x=0;x<8 && pat==1;x++)
	{
		for(y=0;y<8 && pat==1;y++)
		{
			if(cases[x][y].joueur==joueur_adverse)
			{
				obtenir_possibilite_deplacement(cases[x][y].piece,(int*)possibilite_deplacement,x,y,0);
					
				for(i=0;i<8 && pat==1;i++)
				{
					for(j=0;j<8 && pat==1;j++)
					{
						if(possibilite_deplacement[i][j]!=0)  pat=0;
					}
				}
			}
			
		}
	}
	
	if(nul || pat && !echec)return 3;
	else if(pat && echec)
	{
		if(joueur==JOUEUR_BLANC) return 4;
		else return 3;
	}
	
	if(joueur_adverse==JOUEUR_NOIR)afficher_message(1,echec);
	else afficher_message(2,echec);
	return 0;
}



void obtenir_possibilite_deplacement(int piece,int *possibilite_deplacement,int x,int y,int empecher_roi_recursif)
{
	int i,j=0,k=0,l=0,
	piece_autorise_a_continuer_a_avancer=1, piece_mange_un_pion_adverse=0,
	joueur=cases[x][y].joueur,joueur_adverse;
	
	if(joueur==JOUEUR_BLANC) joueur_adverse=JOUEUR_NOIR;
	else joueur_adverse=JOUEUR_BLANC;
	
	//empecher_roi_recursif est destiné au roi. enfait, un roi ne peut aller que la ou il n'est pas menacé
	//il faut donc véréfier toutes les pieces adverses pour savoir si une d'elle metterai le roi en échec
	
	//mais, cela fait planter le script...car le roi adverse a lui aussi besoin de savoir si il est menacé pour se déplacer :p etc..on rentre dans une boucle infinie.
	//il faut donc stopper la récursivité du programme
	
	
	switch(piece)
	{
		case REINE://une reine=un fou+une tour
			obtenir_possibilite_deplacement(FOU,possibilite_deplacement,x,y,empecher_roi_recursif);
			obtenir_possibilite_deplacement(TOUR,possibilite_deplacement,x,y,empecher_roi_recursif);
			break;
			
		case TOUR:
		case FOU:
			for(i=0;i<4;i++)//un fou/tour peut se déplacer dans les 4 directions
			{			
				piece_mange_un_pion_adverse=0;
				piece_autorise_a_continuer_a_avancer=1;
				k=x;
				l=y;
				while(piece_autorise_a_continuer_a_avancer)
				{			
					if(i==0) k++;//verif sur l'axe - vers ->
					else if(i==1) k--;//verif sur l'axe - vers <-
					else if(i==2)l++;//verif axe | vers le haut
					else if(i==3)l--;//verif axe | vers le bas
					
					if(piece==FOU)
					{
						if(i==0)l++;//verif sur l'axe \ vers le bas
						else if(i==1)l--;//verif sur l'axe / vers le haut
						else if(i==2)k--;//verif sur l'axe / vers le bas
						else if(i==3)k++;//verif sur l'axe \ vers le haut
					}
					
					if(cases[k][l].joueur==joueur||piece_mange_un_pion_adverse||!(k<8 && k>=0 && l<8 && l>=0)) piece_autorise_a_continuer_a_avancer=0;//si la piece a mangé un pion a la case précédente OU elle sort du damier ou elle a mangé un pion adverse
					else if(cases[k][l].joueur==0 && possibilite_deplacement[8*k+l]<1) possibilite_deplacement[8*k+l]=1;//si la case est vide
					else if(cases[k][l].joueur==joueur_adverse)//si il y a un pion adverse dessus
					{
						possibilite_deplacement[8*k+l]=2;//impossible d'acceder a un tableau a 2 dimentions via [k][l]
						piece_mange_un_pion_adverse=1;//(probleme de portée de variable :le c ne se "souvient" pas que le tableau est a 2 dimentions)
					}
					 //si il y a un pion appartenenant au joueur sur la case
				}
			}
			break;
			
		case PION:
			if(joueur==JOUEUR_NOIR && y+1<8)
			{
				
				if(cases[x][y+1].joueur==0) 
				{
					if(possibilite_deplacement[8*x+y+1]<1) possibilite_deplacement[8*x+y+1]=1;
					if(y==6 && possibilite_deplacement[8*x+y+1]==1)possibilite_deplacement[8*x+y+1]=3;//adoubement
					if(y==1 && cases[x][y+2].joueur==0 && possibilite_deplacement[8*x+y+2]<1)possibilite_deplacement[8*x+y+2]=1;
				}
				if(x-1>=0)
				{
					if(cases[x-1][y+1].joueur==joueur_adverse) possibilite_deplacement[8*(x-1)+y+1]=2;
					if(cases[x-1][y].joueur==joueur_adverse && cases[x-1][y].piece==PION && cases[x-1][y].a_bouge_au_tour_prececent==1 && cases[x-1][y].position_precedente_y==6)  possibilite_deplacement[8*(x-1)+y+1]=3;
				}
				if(x+1<8)
				{
					
					if(cases[x+1][y+1].joueur==joueur_adverse) possibilite_deplacement[8*(x+1)+y+1]=2;
					if(cases[x+1][y].joueur==joueur_adverse && cases[x+1][y].piece==PION && cases[x+1][y].a_bouge_au_tour_prececent==1 && cases[x+1][y].position_precedente_y==6)  possibilite_deplacement[8*(x+1)+y+1]=3;
				}
			}
			else if(joueur==JOUEUR_BLANC && y-1>=0)
			{
				if(cases[x][y-1].joueur==0)	
				{
					if(possibilite_deplacement[8*x+y-1]<1) possibilite_deplacement[8*x+y-1]=1;
					if(y==1 && possibilite_deplacement[8*x+y-1]==1)possibilite_deplacement[8*x+y-1]=3;
					if(y==6 && cases[x][y-2].joueur==0 && possibilite_deplacement[8*x+y-2]<1)  possibilite_deplacement[8*x+y-2]=1;
				}
				if(x-1>=0)
				{
					if(cases[x-1][y-1].joueur==joueur_adverse) possibilite_deplacement[8*(x-1)+y-1]=2;
					if(cases[x-1][y].joueur==joueur_adverse && cases[x-1][y].piece==PION && cases[x-1][y].position_precedente_y==1 && cases[x-1][y].a_bouge_au_tour_prececent==1)  possibilite_deplacement[8*(x-1)+y-1]=3;
				}
				if(x+1<8)
				{
					if(cases[x+1][y-1].joueur==joueur_adverse)  possibilite_deplacement[8*(x+1)+y-1]=2;
					if(cases[x+1][y].joueur==joueur_adverse && cases[x+1][y].piece==PION && cases[x+1][y].position_precedente_y==1 && cases[x+1][y].a_bouge_au_tour_prececent==1)  possibilite_deplacement[8*(x+1)+y-1]=3;
				}
			}
			break;
			
		case CAVALIER:
			if(x+2<8)
			{
				if(y+1<8)
				{
					if(cases[x+2][y+1].joueur==joueur_adverse) possibilite_deplacement[8*(x+2)+y+1]=2;
					else if(cases[x+2][y+1].joueur==0 && possibilite_deplacement[8*(x+2)+y+1]<1) possibilite_deplacement[8*(x+2)+y+1]=1;
				}
				if(y-1>=0)
				{
					if(cases[x+2][y-1].joueur==joueur_adverse) possibilite_deplacement[8*(x+2)+y-1]=2;
					else if(cases[x+2][y-1].joueur==0 && possibilite_deplacement[8*(x+2)+y-1]<1) possibilite_deplacement[8*(x+2)+y-1]=1;
				}
			}
			if(x+1<8)
			{
				if(y+2<8)
				{
					if(cases[x+1][y+2].joueur==joueur_adverse) possibilite_deplacement[8*(x+1)+y+2]=2;
					else if(cases[x+1][y+2].joueur==0  && possibilite_deplacement[8*(x+1)+y+2]<1) possibilite_deplacement[8*(x+1)+y+2]=1;
				}
				if(y-2>=0)
				{
					if(cases[x+1][y-2].joueur==joueur_adverse) possibilite_deplacement[8*(x+1)+y-2]=2;
					else if(cases[x+1][y-2].joueur==0 && possibilite_deplacement[8*(x+1)+y-2]<1) possibilite_deplacement[8*(x+1)+y-2]=1;
				}
			}
			if(x-1>=0)
			{
				
				if(y+2<8)
				{
					if(cases[x-1][y+2].joueur==joueur_adverse) possibilite_deplacement[8*(x-1)+y+2]=2;
					else if(cases[x-1][y+2].joueur==0 && possibilite_deplacement[8*(x-1)+y+2]<1) possibilite_deplacement[8*(x-1)+y+2]=1;
				}
				if(y-2>=0)
				{
					if(cases[x-1][y-2].joueur==joueur_adverse) possibilite_deplacement[8*(x-1)+y-2]=2;
					else if(cases[x-1][y-2].joueur==0 && possibilite_deplacement[8*(x-1)+y-2]<1) possibilite_deplacement[8*(x-1)+y-2]=1;
				}
			}
			if(x-2>=0)
			{
				if(y+1<8)
				{
					if(cases[x-2][y+1].joueur==joueur_adverse) possibilite_deplacement[8*(x-2)+y+1]=2;
					else if(cases[x-2][y+1].joueur==0 && possibilite_deplacement[8*(x-2)+y+1]<1) possibilite_deplacement[8*(x-2)+y+1]=1;
				}
				if(y-1>=0)
				{
					if(cases[x-2][y-1].joueur==joueur_adverse) possibilite_deplacement[8*(x-2)+y-1]=2;
					else if(cases[x-2][y-1].joueur==0 && possibilite_deplacement[8*(x-2)+y-1]<1) possibilite_deplacement[8*(x-2)+y-1]=1;
				}
			}
			break;
			
		case ROI:
			if(joueur==JOUEUR_BLANC) i=7;
			else i=0;
			
				if(x==4 && y==i && cases[4][i].a_bouge==0//si le roi a pas bougé
				&& cases[0][i].piece==TOUR && cases[0][i].a_bouge==0 //et si la tour en (0,i) a pas bougé
				&& cases[1][i].piece==0 && cases[2][i].piece==0 && cases[3][i].piece==0// et si aucune piece ne fait obstacle
				)  possibilite_deplacement[8*1+i]=3; //cas de grand roque
			
				if(x==4 && y==i && cases[4][i].a_bouge==0
				&& cases[7][i].piece==TOUR && cases[7][i].a_bouge==0 
				&& cases[5][i].piece==0 && cases[6][i].piece==0//petit roque
				)  possibilite_deplacement[8*6+i]=3; //
				
			
			for(i=x-1;i<=x+1;i++)
			{
				for(j=y-1;j<=y+1;j++)
				{
					if((i!=x || j!=y) && !(i<0 || i>7 || j<0 || j>7))//si on sors pas du damier+on est pas sur la case (x;y)
					{
						if(cases[i][j].joueur==joueur_adverse && possibilite_deplacement[8*2+i]<2) possibilite_deplacement[8*i+j]=2;
						else if(cases[i][j].joueur==0 && possibilite_deplacement[8*i+j]<1) possibilite_deplacement[8*i+j]=1;
					}
				}
			}
			break;
	}
	
	for(i=0;i<8;i++)//si le déplacement met en echec le roi : on empeche le deplacement
	{
		for(j=0;j<8;j++)
		{
			if(!empecher_roi_recursif && possibilite_deplacement[i*8+j]>0)
			{
				if(verifier_est_en_echec(joueur,x,y,i,j)) possibilite_deplacement[i*8+j]=0;
			}
		}
	}
}

void obtenir_pieces_en_jeu(int *liste)
{
	int i,x,y;
	for(x=0;x<2;x++)
	{
		for(y=0;y<5;y++)
		{
			liste[x*5+y]=0;
		}
	}
	
	for(x=0;x<8;x++)
	{
		for(y=0;y<8;y++)
		{
			if(cases[x][y].joueur==JOUEUR_NOIR) i=0;
			else if(cases[x][y].joueur==JOUEUR_BLANC) i=5;
			else i=-1;
				
			if(i!=-1)
			{
				if(cases[x][y].piece==CAVALIER) liste[i]++;
				else if(cases[x][y].piece==FOU) liste[i+1]++;
				else if(cases[x][y].piece==TOUR) liste[i+2]++;
				else if(cases[x][y].piece==PION) liste[i+3]++;
				else if(cases[x][y].piece==REINE) liste[i+4]++;
			}
		}
	}
}

void deplacer_piece(int x,int y,int x_final,int y_final)
{
	int i,j;
	
	if(cases[x][y].joueur==JOUEUR_BLANC)i=7;
	else i=0;
	
	
	if(x==4 && y==i && cases[4][i].piece==ROI && x_final==1 && y_final==i && cases[4][i].a_bouge==0//si le roi a pas bougé
	&& cases[0][i].piece==TOUR && cases[0][i].a_bouge==0 //et si la tour en (0,i) a pas bougé
	&& cases[1][i].piece==0 && cases[2][i].piece==0 && cases[3][i].piece==0 // et si aucune piece ne fait obstacle
	)
		deplacer_piece(0,i,2,i);
	
	else if(x==4 && y==i && cases[4][i].piece==ROI && x_final==6 && y_final==i && cases[4][i].a_bouge==0
	&& cases[7][i].piece==TOUR && cases[7][i].a_bouge==0 
	&& cases[5][i].piece==0 && cases[6][i].piece==0
	) 
		deplacer_piece(7,i,5,i);
	
	if((y==3 ||y==4) && (cases[x_final][y].joueur==JOUEUR_NOIR && cases[x][y].joueur==JOUEUR_BLANC || cases[x_final][y].joueur==JOUEUR_BLANC && cases[x][y].joueur==JOUEUR_NOIR) && cases[x][y].piece==PION && (y_final==y+1 || y_final==y-1)&& (x_final==x-1 || x_final==x+1))
	{
		cases[x_final][y].joueur=0;
		cases[x_final][y].piece=0;
		cases[x_final][y].a_bouge=0;
		cases[x_final][y].a_bouge_au_tour_prececent=0;
		cases[x_final][y].position_precedente_x=0;
		cases[x_final][y].position_precedente_y=0;
	}
	
	
	cases[x][y].a_bouge=1;
		
	for(i=0;i<8;i++)
	{
		for(j=0;j<8;j++)
		{
			if(cases[i][j].joueur==cases[x][y].joueur) cases[i][j].a_bouge_au_tour_prececent=0;
		}
	}
	cases[x][y].a_bouge_au_tour_prececent=1;
	cases[x_final][y_final].position_precedente_x=x;
	cases[x_final][y_final].position_precedente_y=y;
	
	cases[x_final][y_final].joueur=cases[x][y].joueur;
	cases[x_final][y_final].piece=cases[x][y].piece;
	cases[x_final][y_final].a_bouge=cases[x][y].a_bouge;
	cases[x_final][y_final].a_bouge_au_tour_prececent=cases[x][y].a_bouge_au_tour_prececent;
	
	cases[x][y].joueur=0;
	cases[x][y].piece=0;
	cases[x][y].a_bouge=0;
	cases[x][y].a_bouge_au_tour_prececent=0;
	cases[x][y].position_precedente_x=0;
	cases[x][y].position_precedente_y=0;
	
	afficher_pieces_manges();
	
	for(i=0;i<8;i++)
	{
		for(j=0;j<8;j++)
		{
			dessiner_case_damier(i,j);
			if(cases[i][j].piece!=0) afficher_sprite(i,j);
		}
	}
	
	if(cases[x_final][y_final].piece==PION && 
	(cases[x_final][y_final].joueur==JOUEUR_BLANC && y_final==0 ||
	cases[x_final][y_final].joueur==JOUEUR_NOIR && y_final==7)
	)
	{
		attendre_choix_adoubement_piece(x_final,y_final);
		dessiner_case_damier(x_final,y_final);
		afficher_sprite(x_final,y_final);
		
		if(cases[x_final][y_final].joueur==JOUEUR_BLANC && verifier_est_en_echec(JOUEUR_NOIR,-1,-1,-1,-1)) afficher_message(1,1);
		if(cases[x_final][y_final].joueur==JOUEUR_NOIR && verifier_est_en_echec(JOUEUR_BLANC,-1,-1,-1,-1)) afficher_message(2,1);
	}
	SDL_Flip(ecran);
}



Uint32 fonction_callback_chrono(Uint32 intervalle, void *parametre)
{
	if(chrono_lance!=0) 
	{
		chronos[chrono_lance-1]++;
		afficher_chronos();
	}
	return intervalle;
}
