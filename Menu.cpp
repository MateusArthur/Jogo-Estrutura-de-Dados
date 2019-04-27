#include <iostream>
#include <allegro.h>
#include <allegro5/allegro.h>
#include <time.h>
#include <math.h>

// Atributos da tela
#define LARGURA_TELA 500
#define ALTURA_TELA 1024
#define GRAUS_PARA_ALLEGRO(x) x/1.40625  //conversão

// Timers
volatile int TimerOuvirPassos;
volatile int TimerTiros;

// Variáveis Globais

const int NUM_BALAS = 30;
const int TILESIZE = 50;

// Estruturas

enum IDS {JOGADORES, PROJETIL, INIMIGOS};

struct TPersonagem 
{
	float x, y, z;
};

struct TProjeteis
{
	int ID;
	float x;
	float y;
	float z;
	int velocidade;
	bool recaregando = false;
	bool ativo = false;
};


// Mapas

enum
{
	SOLO = 0,
	GRAMA = 1,
	AGUA = 2,
	LARVA = 3,
	PEDRA = 4
};

// FUNÇÕES

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

// PEGAR O ANGULO DA MIRA 
TPersonagem Subtrair(float Mira_x, float Mira_y, float Mira_z, TPersonagem Jogador)
{
	TPersonagem diferenca;
	diferenca.x = Mira_x - Jogador.x;
	diferenca.y = Mira_y - Jogador.y;
	diferenca.z = Mira_z - Jogador.z;
	return diferenca;
}
float Magnitude(TPersonagem vec)
{
	return sqrtf(vec.x*vec.x + vec.y + vec.y + vec.z*vec.z);
}
float Distancia(float Mira_x, float Mira_y, float Mira_z, TPersonagem Jogador)
{
	TPersonagem diferenca = Subtrair(Mira_x, Mira_y, Mira_z, Jogador);
	return Magnitude(diferenca);
}
TPersonagem CalcularAngulo(float Mira_x, float Mira_y, float Mira_z, TPersonagem Jogador)
{
	TPersonagem diferenca = Subtrair(Mira_x, Mira_y, Mira_z, Jogador);
	TPersonagem Angulo;
	Angulo.z = (-(float)atan2(diferenca.x, diferenca.y)) / M_PI * 180.0f + 180.0f;
	return Angulo;
}

void InitBalas(TProjeteis balas[], int tamanho)
{
	for(int i = 0; i<tamanho; i++)
	{
		balas[i].ID = PROJETIL;
		balas[i].velocidade = 10;
		balas[i].ativo = false;
	}
}
void AtiraBalas(TProjeteis balas[], int tamanho, TPersonagem jogador)
{
	for(int i = 0; i<tamanho; i++)
	{
		if(!balas[i].ativo)
		{
			balas[i].x = jogador.x+17;
			balas[i].y = jogador.y;
			balas[i].ativo = true;
			break;
		}
	}
}
void AtualizarBalas(TProjeteis balas[], int tamanho)
{
	for(int i = 0; i<tamanho; i++)
	{
		if(balas[i].ativo)
		{
			balas[i].x += balas[i].velocidade;
			if(balas[i].x > LARGURA_TELA)
				balas[i].ativo = false;
		}
	}
}
void DesenharBalas(BITMAP *buffer, TProjeteis balas[], int tamanho)
{
	for(int i = 0; i<tamanho; i++)
	{
		if(balas[i].ativo)
		{
			circle(buffer, balas[i].x+35, balas[i].y+35, 1, makecol(255,255,255));
		}
	}
}

// Funções Timer;
void incrementa_TimerOuvirPassos()
{
	TimerOuvirPassos++;
}
END_OF_FUNCTION(incrementa_TimerOuvirPassos)

void incrementa_TimerTiros()
{
	TimerTiros++;
}
END_OF_FUNCTION(incrementa_TimerTiros)

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

	// INICIALIZAÇÃO DE OBJETOS
	TPersonagem Jogador;
	TProjeteis Balas[NUM_BALAS];
	InitBalas(Balas, NUM_BALAS);

	// Variaveis
	Jogador.x = 100; 
	Jogador.y = 100;
	int vel = 5;
	bool TocandoPassos = false;
	bool TocandoTiros = false;
	TimerOuvirPassos = 0;
	TimerTiros = 0;
	LOCK_FUNCTION(incrementa_TimerOuvirPassos);
	LOCK_FUNCTION(incrementa_TimerTiros);
	LOCK_VARIABLE(TimerOuvirPassos);
	LOCK_VARIABLE(TimerTiros);
	install_int_ex(incrementa_TimerOuvirPassos, MSEC_TO_TIMER(250));
	install_int_ex(incrementa_TimerTiros, MSEC_TO_TIMER(80));

	// Icone do Jogo

	// Carregar Imagens
	BITMAP *buffer = create_bitmap(ALTURA_TELA,LARGURA_TELA);
	BITMAP *Personagem = load_bitmap("Imagens/Jogadores/player_c.bmp", NULL);
	BITMAP *Mira = load_bitmap("Imagens/Outros/Mira.bmp", NULL);
	
	//Carregar a mira
	show_mouse(Mira);

	// MAPA

	int linhas, colunas;
	int **mapa = Carregar_Mapa("Mapas/mapa.txt", &linhas, &colunas);
	
	//Sons
	
	SAMPLE *Caminhar = load_sample("Sons/Jogadores/Correndo.wav");
	SAMPLE *STiro = load_sample("Sons/Jogadores/Tiro.wav");
	
	// Looping do Jogo

	while(!key[KEY_ESC])
	{
		if(key[KEY_D] && !key[KEY_LSHIFT]) // Direita
		{
			Jogador.x += 2;
			//			Var       Vol  Can  Vel Rep
			if(!TocandoPassos)
			{
				TocandoPassos = true;
			}
		}
		else if(key[KEY_D] && key[KEY_LSHIFT]) // Direita
		{
			Jogador.x += 1;
		}
		if(key[KEY_A] && !key[KEY_LSHIFT]) // Esquerda
		{ 
			Jogador.x -= 2;
			if(!TocandoPassos)
			{
				TocandoPassos = true;
			}
		}
		else if(key[KEY_A] && key[KEY_LSHIFT]) // Esquerda
		{ 
			Jogador.x -= 1;
		}
		if(key[KEY_W] && !key[KEY_LSHIFT]) // Cima
		{ 
			Jogador.y -= 2;
			if(!TocandoPassos)
			{
				TocandoPassos = true;
			}
		}
		else if(key[KEY_W] && key[KEY_LSHIFT]) // Cima
		{ 
			Jogador.y -= 1;
		}
		if(key[KEY_S] && !key[KEY_LSHIFT]) // Baixo
		{
			Jogador.y += 2;
			if(!TocandoPassos)
			{
				TocandoPassos = true;
			}
		}
		else if(key[KEY_S] && key[KEY_LSHIFT])
		{
			Jogador.y += 1;
		}
		if(TimerOuvirPassos > 1)
		{
			if(key[KEY_W] || key[KEY_A] || key[KEY_D] || key[KEY_S])
			{
				if(!key[KEY_LSHIFT])
				{
					play_sample(Caminhar, 255, 128, 1000, 0);
					play_sample(Caminhar, 255, 128, 1000, 0);
					TimerOuvirPassos = 0;
				}
				else
				{
					TocandoPassos = false;
					TimerOuvirPassos = 0;
				}
			}
		}
		if(TimerTiros > 1)
		{
			if(TocandoTiros)
			{
				TocandoTiros = false;
				TimerTiros = 0;
			}
		}
		if(key[KEY_SPACE])
		{
			if(!TocandoTiros)
			{	
				play_sample(STiro, 255, 128, 1000, 0);
				AtiraBalas(Balas, NUM_BALAS, Jogador);
				TocandoTiros =  true;
			}
		}
		AtualizarBalas(Balas, NUM_BALAS);
		Desenhar_Mapa(buffer, mapa, linhas, colunas);
		//draw_sprite(buffer, Personagem, 100+x, 100+y);
		//pivot_sprite(buffer, Personagem, Jogador.x, Jogador.y, 0, 0, itofix(GRAUS_PARA_ALLEGRO(Angulo.z)));
		TPersonagem Angulo = CalcularAngulo(mouse_x, mouse_y, mouse_z, Jogador);
		rotate_sprite(buffer, Personagem, Jogador.x, Jogador.y, itofix(GRAUS_PARA_ALLEGRO(Angulo.z)));
		draw_sprite(buffer, Mira, mouse_x, mouse_y);
		DesenharBalas(buffer, Balas, NUM_BALAS);
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