#include <iostream>
#include <allegro.h>
#include <time.h>

// Atributos da tela
#define LARGURA_TELA 500
#define ALTURA_TELA 1024

#define GRAUS_PARA_ALLEGRO(x) x/1.40625  //conversão

// Estruturas

/*struct obj {int wx, wy, x, y, w, h;};
struct obj
{
	playerA = {0,0,200,200,50,50},
	tiro = {0,0,200,200,50,50}
};*/

// Mapas

const int TILESIZE = 50;

enum
{
	SOLO = 0,
	GRAMA = 1,
	AGUA = 2,
	LARVA = 3,
	PEDRA = 4
};

int **Carregar_Mapa(const char *nome_arquivo, int *linhas, int *colunas)
{
	FILE *f = fopen(nome_arquivo, "r");
	int **matriz;

	if(f != NULL)
	{
		int i, j;
		fscanf(f, "%d %d", linhas, colunas);

		//Alocar o Mapa
		matriz = (int**)malloc((*linhas)*sizeof(int*));
		for(i = 0; i < *linhas; i++)
			matriz[i] = (int*)malloc((*colunas)*sizeof(int));

		//Carregar TILES
		for(i = 0;i < *linhas; i++)
		{
			for(j = 0; j < *linhas; j++)
				fscanf(f, "%d", &matriz[i][j]);
		}
		fclose(f);
	}
	return matriz;
}

void Desenhar_Mapa(BITMAP *buffer, int **mapa, int linhas, int colunas)
{
	int i, j;
	for(i = 0; i < linhas; i ++)
	{
		for(j = 0; j < colunas; j ++)
		{
			if(mapa[i][j] == SOLO)
				rectfill(buffer, j * TILESIZE, i * TILESIZE, (j * TILESIZE) + TILESIZE, (i * TILESIZE) + TILESIZE, makecol(0,230,0));
			else if(mapa[i][j] == GRAMA)
				rectfill(buffer, j * TILESIZE, i * TILESIZE, (j * TILESIZE) + TILESIZE, (i * TILESIZE) + TILESIZE, makecol(0,230,0));
			else if(mapa[i][j] == AGUA)
				rectfill(buffer, j * TILESIZE, i * TILESIZE, (j * TILESIZE) + TILESIZE, (i * TILESIZE) + TILESIZE, makecol(70,160,255));
			else if(mapa[i][j] == LARVA)
				rectfill(buffer, j * TILESIZE, i * TILESIZE, (j * TILESIZE) + TILESIZE, (i * TILESIZE) + TILESIZE, makecol(215,0,0));
			else if(mapa[i][j] == PEDRA)
				rectfill(buffer, j * TILESIZE, i * TILESIZE, (j * TILESIZE) + TILESIZE, (i * TILESIZE) + TILESIZE, makecol(133,133,133));
		}
	}
}

void Libera_Mapa(int **mapa, int linhas)
{
	int i;
	for(i = 0; i < linhas; i ++)
		free(mapa[i]);
	free(mapa);
}
// Variáveis Globais

volatile int timer;
int angulo;

void incrementa_timer()
{
	timer++;
}
END_OF_FUNCTION(incrementa_timer)

int main(void)
{
	// Inicializando e configurando o Jogo
	allegro_init();
	install_timer();
	install_keyboard();
	install_mouse();
	set_mouse_speed(20, 20);
	set_color_depth(32);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, ALTURA_TELA, LARGURA_TELA, 0, 0);
	set_window_title("Counter Strike 2D");
	install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL);
	// Variaveis
	int x = 100, y = 100, vel = 5;
	bool TocandoPassos = false;
	timer = 0;
	angulo=0;
	LOCK_FUNCTION(incrementa_timer);
	LOCK_VARIABLE(timer);
	install_int_ex(incrementa_timer, MSEC_TO_TIMER(250));
	// Icone do Jogo

	// Carregar Imagens
	BITMAP *buffer = create_bitmap(ALTURA_TELA,LARGURA_TELA);
	BITMAP *Armas[5];
	// Quatro lados do Personagem
	BITMAP *Personagem = load_bitmap("Imagens/Jogadores/player_c.bmp", NULL);
	BITMAP *Mira = load_bitmap("Imagens/Outros/Mira.bmp", NULL);
	show_mouse(Mira);
	// MAPA
	int linhas, colunas;
	int **mapa = Carregar_Mapa("Mapas/mapa.txt", &linhas, &colunas);
	//Sons
	SAMPLE *Caminhar = load_sample("Sons/Jogadores/Correndo.wav");
	// Looping do Jogo
	while(!key[KEY_ESC])
	{
		if(key[KEY_D] && !key[KEY_LSHIFT]) // Direita
		{
			x += 2;
			//			Var       Vol  Can  Vel Rep
			if(!TocandoPassos)
			{
				TocandoPassos = true;
			}
		}
		else if(key[KEY_D] && key[KEY_LSHIFT]) // Direita
		{
			x += 1;
		}
		if(key[KEY_A] && !key[KEY_LSHIFT]) // Esquerda
		{ 
			x -= 2;
			if(!TocandoPassos)
			{
				TocandoPassos = true;
			}
		}
		else if(key[KEY_A] && key[KEY_LSHIFT]) // Esquerda
		{ 
			x -= 1;
		}
		if(key[KEY_W] && !key[KEY_LSHIFT]) // Cima
		{ 
			y -= 2;
			if(!TocandoPassos)
			{
				TocandoPassos = true;
			}
		}
		else if(key[KEY_W] && key[KEY_LSHIFT]) // Cima
		{ 
			y -= 1;
		}
		if(key[KEY_S] && !key[KEY_LSHIFT]) // Baixo
		{
			y += 2;
			if(!TocandoPassos)
			{
				TocandoPassos = true;
			}
		}
		else if(key[KEY_S] && key[KEY_LSHIFT])
		{
			y += 1;
		}
		if(timer > 1)
		{
			if(key[KEY_W] || key[KEY_A] || key[KEY_D] || key[KEY_S])
			{
				if(!key[KEY_LSHIFT])
				{
					play_sample(Caminhar, 255, 128, 1000, 0);
					timer = 0;
				}
				else
				{
					TocandoPassos = false;
					timer = 0;
				}
			}
		}
		Desenhar_Mapa(buffer, mapa, linhas, colunas);
		//draw_sprite(buffer, Personagem, 100+x, 100+y);
		pivot_sprite(buffer, Personagem, 100+x, 100+y, 100, 100, itofix(GRAUS_PARA_ALLEGRO(mouse_y)));
		//rotate_sprite(buffer, Personagem, mouse_x, mouse_y, itofix(GRAUS_PARA_ALLEGRO(angulo)));
		draw_sprite(buffer, Mira, mouse_x, mouse_y);
		draw_sprite(screen, buffer, 0, 0);
		rest(1);
		clear(buffer);
	}
	destroy_bitmap(Personagem);
	destroy_bitmap(Mira);
	destroy_bitmap(buffer);
	destroy_sample(Caminhar);
	Libera_Mapa(mapa, linhas);
  	return 0;
}

END_OF_MAIN();