#include <iostream>
#include <allegro.h>
#include <time.h>
#include <math.h>

// Atributos da tela
#define LARGURA_TELA 600
#define ALTURA_TELA 800
#define GRAUS_PARA_ALLEGRO(x) x/1.40625  //conversão

// Timers
volatile int TimerOuvirPassos;
volatile int TimerTiros;
volatile int TimerRecarregar;
bool RodarTimerRecarregar = false;
bool RodarTimerTiro = false;

// Variáveis Globais

const int NUM_BALAS = 30;
const int TILESIZE = 50;

// Estruturas

enum IDS {JOGADORES, PROJETIL, INIMIGOS};

struct TPersonagem 
{
	float x, y, z;
	int quadrante;
	int qntBalas;
	bool recarregando = false;
	int vida;
	int vidamax;
};

struct TProjeteis
{
	int ID;
	float x;
	float y;
	float tiro_x;
	float tiro_y;
	int velocidade;
	int quadrante;
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
				rectfill(buffer, j * TILESIZE, i * TILESIZE, (j * TILESIZE) + TILESIZE, (i * TILESIZE) + TILESIZE, makecol(0,0,0));
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
	diferenca.x = Mira_x - (Jogador.x+26);
	diferenca.y = Mira_y - (Jogador.y+28);
	diferenca.z = Mira_z - Jogador.z;
	return diferenca;
}
TPersonagem CalcularAngulo(float Mira_x, float Mira_y, float Mira_z, TPersonagem Jogador)
{
	TPersonagem diferenca = Subtrair(Mira_x, Mira_y, Mira_z, Jogador);
	TPersonagem Angulo;
	Angulo.z = (-(float)atan2(diferenca.x, diferenca.y)) / M_PI * 180.0f + 180.0f;
	return Angulo;
}

// Destruir Tiros

TPersonagem SubtrairDist(float Mira_x, float Mira_y, float Mira_z, TProjeteis Tiro)
{
	TPersonagem diferenca;
	diferenca.x = Mira_x - Tiro.x;
	diferenca.y = Mira_y - Tiro.y;
	diferenca.z = Mira_z - 0;
	return diferenca;
}
float Magnitude(TPersonagem vec)
{
	return sqrtf(vec.x*vec.x + vec.y + vec.y + vec.z*vec.z);
}
float Distancia(TProjeteis Balas)
{
	TPersonagem diferenca = SubtrairDist(Balas.tiro_x, Balas.tiro_y, 0, Balas);
	return Magnitude(diferenca);
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
			balas[i].tiro_x = mouse_x-15;
			balas[i].tiro_y = mouse_y-25;
			balas[i].ativo = true;
			TPersonagem Angulo = CalcularAngulo(mouse_x, mouse_y, mouse_z, jogador);
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
			double bullet_direction = atan2((balas[i].tiro_y) - balas[i].y , (balas[i].tiro_x) - balas[i].x);
			if(balas[i].quadrante == 0) //atirar pra frente
			{
				balas[i].x += balas[i].velocidade*cos(bullet_direction);
				balas[i].y += balas[i].velocidade*sin(bullet_direction);
			}
			else if(balas[i].quadrante == 1) // atirar pra trás
			{
				balas[i].x -= balas[i].velocidade*cos(bullet_direction);
				balas[i].y -= balas[i].velocidade*sin(bullet_direction);
			}
			float DistanciaT = sqrtf(((balas[i].tiro_x - balas[i].x)*(balas[i].tiro_x - balas[i].x)) + ((balas[i].tiro_y - balas[i].y)*(balas[i].tiro_y - balas[i].y)));
			// Remover da Tela
			if(DistanciaT < 5)
				balas[i].ativo = false;
		}
	}
}
void DesenharBalas(BITMAP *buffer, TProjeteis balas[], int tamanho, TPersonagem Jogador)
{
	for(int i = 0; i<tamanho; i++)
	{
		if(balas[i].ativo)
		{
			TPersonagem Angulo = CalcularAngulo(mouse_x, mouse_y, mouse_z, Jogador);
			// itofix(GRAUS_PARA_ALLEGRO(Angulo.z)
			int somarx = 22;
			int somary = 35;
			circle(buffer, balas[i].x+somarx, balas[i].y+somary, 2, makecol(255,0,0));
		}
	}
}
//Funcoes de vida
void Barra_Vida(BITMAP *buffer, TPersonagem Jogador) 
{
	int n = (Jogador.vida*150) / Jogador.vidamax;
	rectfill(buffer, LARGURA_TELA-162, 10, LARGURA_TELA-8, 25, 0x003300);
	rectfill(buffer, LARGURA_TELA-160, 12, LARGURA_TELA-160+n, 23, 0x00ff00);
	rectfill(buffer, LARGURA_TELA-160, 12, LARGURA_TELA-160+n, 15, 0xbbffaa);
	rect(buffer, LARGURA_TELA-162, 10, LARGURA_TELA-8, 25, 0x000000); 
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

void incrementa_TimerRecarregar()
{
	if(RodarTimerRecarregar == true) TimerRecarregar++;
}
END_OF_FUNCTION(incrementa_TimerRecarregar)

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

	// Variaveis & Inicializacao de variaveis
	Jogador.x = 100; 
	Jogador.y = 100;
	Jogador.qntBalas = 30;
	Jogador.vida = 200;
	Jogador.vidamax = 200;
	int vel = 5;
	char str[10];
	bool sair = false;
	bool TocandoPassos = false;
	bool TocandoTiros = false;
	TimerOuvirPassos = 0;
	TimerTiros = 0;
	TimerRecarregar = 0;
	LOCK_FUNCTION(incrementa_TimerOuvirPassos);
	LOCK_FUNCTION(incrementa_TimerTiros);
	LOCK_VARIABLE(incrementa_TimerRecarregar);
	LOCK_VARIABLE(TimerOuvirPassos);
	LOCK_VARIABLE(TimerTiros);
	LOCK_VARIABLE(TimerRecarregar);
	install_int_ex(incrementa_TimerOuvirPassos, MSEC_TO_TIMER(250));
	install_int_ex(incrementa_TimerTiros, MSEC_TO_TIMER(80));
	install_int_ex(incrementa_TimerRecarregar, SECS_TO_TIMER(1));

	// Icone do Jogo

	// Carregar Imagens
	BITMAP *buffer = create_bitmap(ALTURA_TELA,LARGURA_TELA);
	BITMAP *Personagem = load_bitmap("Imagens/Jogadores/player_c.bmp", NULL);
	BITMAP *Mira = load_bitmap("Imagens/Outros/Mira.bmp", NULL);
	BITMAP *FogoTiro = load_bitmap("Imagens/Outros/Tiro.bmp", NULL);
	
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
		if(sair == true)
			break;
		int ax,ay;
		ax = Jogador.x;
		ay = Jogador.y;
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
		if(key[KEY_R] && Jogador.qntBalas < 30 && !Jogador.recarregando) {
			Jogador.recarregando = true;
			RodarTimerRecarregar = true;
		}
		if(TimerRecarregar > 3 && Jogador.recarregando) {
			Jogador.qntBalas = 30;
			TimerRecarregar = 0;
			Jogador.recarregando = false;
			RodarTimerRecarregar = false;
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
		if(TimerTiros > 1) // 6 Tiro por tiro 1 Spray
		{
			if(TocandoTiros)
			{
				TocandoTiros = false;
				TimerTiros = 0;
				RodarTimerTiro = false;
			}
		}
		if(mouse_b)
		{
			if(!TocandoTiros && Jogador.qntBalas != 0)
			{	
				play_sample(STiro, 255, 128, 1000, 0);
				AtiraBalas(Balas, NUM_BALAS, Jogador);
				Jogador.qntBalas--;
				TocandoTiros =  true;
				RodarTimerTiro = true;
			}
		}
		// Verificar Colisao
		bool colidiu = false;
		int px = Jogador.x-160;
		int py = Jogador.y-160+16;
		for(int ci=0; ci < 32; ci++) 
		{
			for(int cj = 0; cj < 16; cj++)
			{
				if(getpixel(Colisao, Jogador.x+ci, Jogador.y+cj)) 
				{
					colidiu = true;
					ci = 32;
					cj = 16;
				}
				if(getpixel(Colisao, Jogador.x+ci, Jogador.y+cj) == makecol(133,133,133)) 
					sair = true;
			}
		}
		if(colidiu) 
		{
			Jogador.x = ax;
			Jogador.y = ay;
		}
		// Fim Colisao
		Desenhar_Mapa(buffer, mapa, linhas, colunas);
		AtualizarBalas(Balas, NUM_BALAS);
		TPersonagem Angulo = CalcularAngulo(mouse_x, mouse_y, mouse_z, Jogador);
		Jogador.z = Angulo.z;
		rotate_sprite(buffer, Personagem, Jogador.x, Jogador.y, itofix(GRAUS_PARA_ALLEGRO(Angulo.z)));
		draw_sprite(buffer, Mira, mouse_x, mouse_y);
		DesenharBalas(buffer, Balas, NUM_BALAS, Jogador);
		//Textos
		sprintf(str, "Balas: %d", Jogador.qntBalas);
		if(Jogador.recarregando) 
		{ 
			textout_ex(buffer, font, "Recarregando, Aguarde..", Jogador.x-30, Jogador.y-10, makecol(255,0,0), -1);
		}
		textout_ex(buffer, font, str, 30, 10, makecol(255,0,0), 0);
		Barra_Vida(buffer, Jogador);
		//Fim
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