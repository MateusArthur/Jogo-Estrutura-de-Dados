#include <iostream>
#include <allegro.h>
#include <time.h>
#include <math.h>

// Atributos da tela

#define LARGURA_TELA 600
#define ALTURA_TELA 800
#define GRAUS_PARA_ALLEGRO(x) x/1.40625  //conversão
#define MAX_PLAYERS 2
#define playerid 0
#define NPC 1
#define MAX_COLISOES 5

// Timers

volatile int TimerMenu;
volatile int TimerOuvirPassos;
volatile int TimerTiros[MAX_PLAYERS];
volatile int TimerRecarregar[MAX_PLAYERS];
bool RodarTimerRecarregar[MAX_PLAYERS];
bool RodarTimerTiro[MAX_PLAYERS];

// Variáveis Globais

const int NUM_BALAS = 30;
const int TILESIZE = 50;
int Colisoes[MAX_COLISOES][4];
int SalvarVida[MAX_PLAYERS];

// Estruturas

enum IDS {JOGADORES, PROJETIL, INIMIGOS};

struct TPersonagem 
{
	int x, y;
	float z;
	int qntBalas;
	bool recarregando = false;
	int vida;
	int vidamax;
};

struct TInimigos 
{
	int x, y;
	float visao_x, visao_y;
	float z = -90;
	int qntBalas;
	bool recarregando = false;
	int vida;
	int vidamax;
	int escutou_x, escutou_y;
	bool enxergando = false;
};

struct TProjeteis
{
	int ID;
	float x;
	float y;
	float tiro_x;
	float tiro_y;
	float x_anterior;
	float y_anterior;
	int velocidade;
	int atirador = -1;
	bool ativo = false;
};

struct TMenu
{
	int op;
	int max_op;
	int local;
	int resolucao;
	int volume;
	int sensi;
	int load;
	bool sair = false;
	bool click = false;
	bool ativo = true;
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

TInimigos SubtrairInimigo(float Jogador_x, float Jogador_y, float Jogador_z, TInimigos Inimigo)
{
	TInimigos diferenca;
	diferenca.x = Jogador_x - (Inimigo.x);
	diferenca.y = Jogador_y - (Inimigo.y);
	diferenca.z = Jogador_z - Inimigo.z;
	return diferenca;
}

TInimigos CalcularAnguloInimigo(float Jogador_x, float Jogador_y, float Jogador_z, TInimigos Inimigo)
{
	TInimigos diferenca = SubtrairInimigo(Jogador_x, Jogador_y, Jogador_z, Inimigo);
	TInimigos Angulo;
	Angulo.z = (-(float)atan2(diferenca.x, diferenca.y)) / M_PI * 180.0f + 180.0f;
	return Angulo;
}

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
			balas[i].atirador = playerid;
			break;
		}
	}
}

void AtiraBalasInimigos(TProjeteis balas[], int tamanho, TInimigos Inimigo, TPersonagem Jogador)
{
	for(int i = 0; i<tamanho; i++)
	{
		if(!balas[i].ativo)
		{
			balas[i].x = Inimigo.x+17;
			balas[i].y = Inimigo.y;
			balas[i].tiro_x = Jogador.x;
			balas[i].tiro_y = Jogador.y;
			balas[i].ativo = true;
			balas[i].atirador = NPC;
			break;
		}
	}
}

void AtualizarBalas(TProjeteis balas[], int tamanho, TPersonagem Jogador, TInimigos Inimigo)
{
	for(int i = 0; i<tamanho; i++)
	{
		if(balas[i].ativo)
		{
			double bullet_direction = atan2((balas[i].tiro_y) - balas[i].y , (balas[i].tiro_x) - balas[i].x);
			balas[i].x += balas[i].velocidade*cos(bullet_direction);
			balas[i].y += balas[i].velocidade*sin(bullet_direction);
			for(int j = 0; j < MAX_COLISOES; j++)
			{
				if(balas[i].x >= Colisoes[j][0]-20 && balas[i].y <= Colisoes[j][3]-29 && balas[i].x <= Colisoes[j][2]-25 && balas[i].y >= Colisoes[j][1]-35)
				{
					balas[i].ativo = false;
				}
			}
			float Distancia_Alvo;
			if(balas[i].atirador == playerid)
			{
				Distancia_Alvo = sqrtf(((Inimigo.x - balas[i].x)*(Inimigo.x - balas[i].x)) + ((Inimigo.y - balas[i].y)*(Inimigo.y - balas[i].y)));
			}
			else 
			{
				Distancia_Alvo = sqrtf(((Jogador.x - balas[i].x)*(Jogador.x - balas[i].x)) + ((Jogador.y - balas[i].y)*(Jogador.y - balas[i].y)));
			}
			float DistanciaT = sqrtf(((balas[i].tiro_x - balas[i].x)*(balas[i].tiro_x - balas[i].x)) + ((balas[i].tiro_y - balas[i].y)*(balas[i].tiro_y - balas[i].y)));
			// Remover da Tela
			if(Distancia_Alvo < 15)
			{
				if(balas[i].atirador == playerid)
					SalvarVida[NPC] = Inimigo.vida -= 10;
				else
					 SalvarVida[playerid] = Jogador.vida -= 10;
				balas[i].ativo = false;
			}
			else if(DistanciaT < 5)
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
			int somarx = 22;
			int somary = 35;
			circle(buffer, balas[i].x+somarx, balas[i].y+somary, 2, makecol(255,0,0));
		}
	}
}

//Funcoes de vida
void Barra_Vida_Inimigo(BITMAP *buffer, TInimigos Inimigo) 
{
	int nm = (Inimigo.vida*93)/Inimigo.vidamax;
	rectfill(buffer, Inimigo.x-5+1, Inimigo.y-10, Inimigo.x+nm, Inimigo.y, 0xFF0000);
	rect(buffer, Inimigo.x-5, Inimigo.y-10, Inimigo.x+93, Inimigo.y, 0x000000);
}

void Barra_Vida(BITMAP *buffer, TPersonagem Jogador) 
{
	int n = (Jogador.vida*150) / Jogador.vidamax;
	rectfill(buffer, LARGURA_TELA-162, 10, LARGURA_TELA-8, 25, 0x003300);
	rectfill(buffer, LARGURA_TELA-160, 12, LARGURA_TELA-160+n, 23, 0x00ff00);
	rectfill(buffer, LARGURA_TELA-160, 12, LARGURA_TELA-160+n, 15, 0xbbffaa);
	rect(buffer, LARGURA_TELA-162, 10, LARGURA_TELA-8, 25, 0x000000); 
}

void Init_Colisoes()
{
	// Quadrado 1
	Colisoes[0][0] = 150;
	Colisoes[0][1] = 100;
	Colisoes[0][2] = Colisoes[0][0]+99;
	Colisoes[0][3] = Colisoes[0][1]+99;

	// Quadrado 2
	Colisoes[1][0] = Colisoes[0][0];
	Colisoes[1][1] = 400;
	Colisoes[1][2] = Colisoes[1][0]+99;
	Colisoes[1][3] = Colisoes[1][1]+99;

	// Quadrado 3
	Colisoes[2][0] = 350;
	Colisoes[2][1] = 250;
	Colisoes[2][2] = Colisoes[2][0]+99;
	Colisoes[2][3] = Colisoes[2][1]+99;

	// Quadrado 4
	Colisoes[3][0] = 550;
	Colisoes[3][1] = Colisoes[0][1];
	Colisoes[3][2] = Colisoes[3][0]+99;
	Colisoes[3][3] = Colisoes[3][1]+99;
	
	// Quadrado 5
	Colisoes[4][0] = Colisoes[3][0];
	Colisoes[4][1] = Colisoes[1][1];
	Colisoes[4][2] = Colisoes[4][0]+99;
	Colisoes[4][3] = Colisoes[4][1]+99;
}

//Definir Colisoes
void Desenhar_Colisoes(BITMAP *buffer)
{
	for(int x = 0; x < MAX_COLISOES; x++)
	{
		hline(buffer, Colisoes[x][0], Colisoes[x][1], Colisoes[x][2], makecol(0,0,0));
		vline(buffer, Colisoes[x][0], Colisoes[x][1], Colisoes[x][3], makecol(0,0,0));
		hline(buffer, Colisoes[x][0], Colisoes[x][3], Colisoes[x][2], makecol(0,0,0));
		vline(buffer, Colisoes[x][2], Colisoes[x][1], Colisoes[x][3], makecol(0,0,0));
	}
}

bool Checar_Colisao(int x, int y) 
{
	for(int j = 0; j < MAX_COLISOES; j++)
	{
		if(x >= Colisoes[j][0]-74 && y <= Colisoes[j][3] && x <= Colisoes[j][2]-5 && y >= Colisoes[j][1]-70)
			return true;
	}
	return false;
}

bool Checar_VisaoInimigo(TInimigos Inimigo, int Jogador_x, int Jogador_y)
{
	float DistanciaT = 100;
	Inimigo.visao_x = Inimigo.x+17;
	Inimigo.visao_y = Inimigo.y;
	while(DistanciaT > 5)
	{
		double bullet_direction = atan2((Jogador_y) - Inimigo.visao_y , (Jogador_x) - Inimigo.visao_x);
		Inimigo.visao_x += 5*cos(bullet_direction);
		Inimigo.visao_y += 5*sin(bullet_direction);
		for(int j = 0; j < MAX_COLISOES; j++)
		{
			if(Inimigo.visao_x >= Colisoes[j][0]-20 && Inimigo.visao_y <= Colisoes[j][3]-29 && Inimigo.visao_x <= Colisoes[j][2]-25 && Inimigo.visao_y >= Colisoes[j][1]-35)
				return false;
		}
		DistanciaT = sqrtf(((Jogador_x - Inimigo.visao_x)*(Jogador_x - Inimigo.visao_x)) + ((Jogador_y - Inimigo.visao_y)*(Jogador_y - Inimigo.visao_y)));
		if(DistanciaT < 5)
			return true;
	}
	return false;
}

int SetarPos_x(TInimigos Inimigo)
{
	if(Inimigo.x < Inimigo.escutou_x)
		return Inimigo.x += 2;
	else if(Inimigo.x >= Inimigo.escutou_x)
		return Inimigo.x -= 2;
	return Inimigo.x;
}

int SetarPos_y(TInimigos Inimigo)
{
	if(Inimigo.y < Inimigo.escutou_y)
		return Inimigo.y += 2;
	else if(Inimigo.y >= Inimigo.escutou_y)
		return Inimigo.y -= 2;
	return Inimigo.y;
}

//Desenhar Loading 

void Desenhar_Loading(BITMAP *buffer, FONT *Fonte, TMenu Menu) 
{
	if(Menu.load == 1)
		textout_centre_ex(buffer, Fonte, "LOADING.", ALTURA_TELA/2, 250, makecol(255, 255, 255), makecol(-1, -1, -1));
	else if(Menu.load == 2)
		textout_centre_ex(buffer, Fonte, "LOADING..", ALTURA_TELA/2, 250, makecol(255, 255, 255), makecol(-1, -1, -1));
	else if(Menu.load == 3)
		textout_centre_ex(buffer, Fonte, "LOADING...", ALTURA_TELA/2, 250, makecol(255, 255, 255), makecol(-1, -1, -1));

}

//Desenhar Menu
void Desenhar_Menu(BITMAP *buffer, FONT *Fonte, TMenu Menu)
{
	if(Menu.local == 0)
	{
		if(Menu.op == 0) textout_centre_ex(buffer, Fonte, "INICIAR", ALTURA_TELA/2, 250, makecol(255, 255, 255), makecol(0,255,127));
		else textout_centre_ex(buffer, Fonte, "INICIAR", ALTURA_TELA/2, 250, makecol(255, 255, 255), makecol(-1, -1, -1));
		
		if(Menu.op == 1) textout_centre_ex(buffer, Fonte, "CONFIGURACOES", ALTURA_TELA / 2, 290, makecol(255, 255, 255), makecol(0,255,127));
		else textout_centre_ex(buffer, Fonte, "CONFIGURACOES", ALTURA_TELA / 2, 290, makecol(255, 255, 255), makecol(-1, -1, -1));
		
		if(Menu.op == 2) textout_centre_ex(buffer, Fonte, "SAIR", ALTURA_TELA / 2, 330, makecol(255, 255, 255), makecol(0,255,127));
		else textout_centre_ex(buffer, Fonte, "SAIR", ALTURA_TELA / 2, 330, makecol(255, 255, 255), makecol(-1, -1, -1));
	}
	else if(Menu.local == 1)
	{
		if(Menu.op == 0) textout_centre_ex(buffer, Fonte, "VOLUME", ALTURA_TELA/2, 250, makecol(255, 255, 255), makecol(0,255,127));
		else textout_centre_ex(buffer, Fonte, "VOLUME", ALTURA_TELA/2, 250, makecol(255, 255, 255), makecol(-1, -1, -1));
		
		textprintf_right_ex(buffer, Fonte, ALTURA_TELA-20, 250, makecol(255, 255, 255), -1, "< %d >", Menu.volume);

		if(Menu.op == 1) textout_centre_ex(buffer, Fonte, "RESOLUCAO", ALTURA_TELA/2, 290, makecol(255, 255, 255), makecol(0,255,127));
		else textout_centre_ex(buffer, Fonte, "RESOLUCAO", ALTURA_TELA/2, 290, makecol(255, 255, 255), makecol(-1, -1, -1));
		
		char str[20];
		if(Menu.resolucao == 0)
			sprintf(str, "< Modo Janela >");
		else
			sprintf(str, "< Tela Cheia >");

		textout_right_ex(buffer, Fonte, str, ALTURA_TELA-20, 290, makecol(255, 255, 255), -1);

		if(Menu.op == 2) textout_centre_ex(buffer, Fonte, "SENSIBILIDADE", ALTURA_TELA/2, 330, makecol(255, 255, 255), makecol(0,255,127));
		else textout_centre_ex(buffer, Fonte, "SENSIBILIDADE", ALTURA_TELA/2, 330, makecol(255, 255, 255), makecol(-1, -1, -1));

		textprintf_right_ex(buffer, Fonte, ALTURA_TELA-20, 330, makecol(255, 255, 255), -1, "< %d >", Menu.sensi);

		if(Menu.op == 3) textout_centre_ex(buffer, Fonte, "VOLTAR", ALTURA_TELA/2, 370, makecol(255, 255, 255), makecol(0,255,127));
		else textout_centre_ex(buffer, Fonte, "VOLTAR", ALTURA_TELA/2, 370, makecol(255, 255, 255), makecol(-1, -1, -1));
	}
	else if(Menu.local == 2)
	{
		if(Menu.op == 0) textout_centre_ex(buffer, Fonte, "RETOMAR JOGO", ALTURA_TELA/2, 250, makecol(255, 255, 255), makecol(0,255,127));
		else textout_centre_ex(buffer, Fonte, "RETOMAR JOGO", ALTURA_TELA/2, 250, makecol(255, 255, 255), makecol(-1, -1, -1));
		
		if(Menu.op == 1) textout_centre_ex(buffer, Fonte, "SAIR PARA O MENU", ALTURA_TELA / 2, 290, makecol(255, 255, 255), makecol(0,255,127));
		else textout_centre_ex(buffer, Fonte, "SAIR PARA O MENU", ALTURA_TELA / 2, 290, makecol(255, 255, 255), makecol(-1, -1, -1));
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
	if(RodarTimerTiro[playerid]) TimerTiros[playerid]++;
	if(RodarTimerTiro[NPC]) TimerTiros[NPC]++;
}
END_OF_FUNCTION(incrementa_TimerTiros)

void incrementa_TimerRecarregar()
{
	if(RodarTimerRecarregar[playerid] == true) TimerRecarregar[playerid]++;
	if(RodarTimerRecarregar[NPC] == true) TimerRecarregar[NPC]++;
}

void incrementa_TimerMenu()
{
	TimerMenu++;
}
END_OF_FUNCTION(incrementa_TimerRecarregar)

void SalvarConf(TMenu Menu)
{
	FILE * pFile;
	pFile = fopen("conf.dat","w");
	char buffer[16] = {0};
	if (pFile!=NULL)
	{
		sprintf(buffer, "%d\n", Menu.volume);
		fputs(buffer, pFile);
		sprintf(buffer, "%d\n", Menu.resolucao);
  		fputs(buffer, pFile);
  		sprintf(buffer, "%d\n", Menu.sensi);
  		fputs(buffer, pFile);
		fclose(pFile);
	}
}

int main(void)
{
	// Inicializando Allegro
	allegro_init();
	install_timer();
	install_keyboard();
	install_mouse();
	set_color_depth(32);
	set_window_title("Counter Strike 2D");
	install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL);

	//Carregar configuracoes salvas
	FILE * pFile;
	pFile = fopen("conf.dat","r");
	int arm[3];
	int i = 0;
	if(pFile == NULL)
	{
		printf("[COUNTER STRIKE ALLEGRO]: Erro ao ler as configuracoes de usuario!\n");
		return 0;
	}
	else
	{
		while((fscanf(pFile, "%d\n", &arm[i])) != EOF)
			i++;
	}
	fclose (pFile);

	// Carregar estrutura do menu
	TMenu Menu;
	Menu.click = false;
	Menu.op = 0;
	Menu.local = 0;
	TimerMenu = 0;
	Menu.volume = arm[0];
	Menu.resolucao = arm[1];
	Menu.sensi = arm[2];
	
	// Configuracoes do usuario salva
	set_volume(Menu.volume, Menu.volume);
	set_mouse_speed(Menu.sensi, Menu.sensi);
	if(Menu.resolucao == 0)
		set_gfx_mode(GFX_AUTODETECT_WINDOWED, ALTURA_TELA, LARGURA_TELA, 0, 0);
	else
		set_gfx_mode(GFX_AUTODETECT_FULLSCREEN, ALTURA_TELA, LARGURA_TELA, 0, 0);
	
	// Imagens no Menu
	BITMAP *tela_menu = create_bitmap(ALTURA_TELA,LARGURA_TELA);
	BITMAP *background = load_bitmap("Imagens/Outros/menu.bmp", NULL);

	// Fontes Menu
	FONT *myfont;
	PALETTE palette;
	myfont = load_font("Fontes/Elephant.pcx", palette, NULL);
	if (!myfont)
		printf("Couldn't load font!\n");

	LOCK_FUNCTION(incrementa_TimerMenu);
	install_int_ex(incrementa_TimerMenu, MSEC_TO_TIMER(80));
	// Loop Menu
	while(!Menu.sair)
	{
		// Desenhar fundo e opcoes
		masked_blit(background, tela_menu, 0, 0, 0, 0, ALTURA_TELA, LARGURA_TELA);
		Desenhar_Menu(tela_menu, myfont, Menu);
		// Keys no Menu
		if(TimerMenu > 1)
		{
			TimerMenu = 0;
			if(key[KEY_DOWN] && !Menu.click) Menu.op++;
			if(key[KEY_UP] && !Menu.click) Menu.op--;
			if(key[KEY_RIGHT] && !Menu.click && Menu.local == 1)
			{
				// Volume
				if(Menu.op == 0)
				{
					Menu.volume++;
					if(Menu.volume > 255)
						Menu.volume = 255;
					set_volume(Menu.volume, Menu.volume);
				} // RESOLUCAO
				else if(Menu.op == 1)
				{
					if(Menu.resolucao == 1)
					{
						Menu.resolucao = 0;
						set_gfx_mode(GFX_AUTODETECT_WINDOWED, ALTURA_TELA, LARGURA_TELA, 0, 0);
					}
					else
					{
						Menu.resolucao = 1;
						set_gfx_mode(GFX_AUTODETECT_FULLSCREEN, ALTURA_TELA, LARGURA_TELA, 0, 0);
					}
				} // sensibilidade
				else if(Menu.op == 2) 
				{
					Menu.sensi++;
					set_mouse_speed(Menu.sensi, Menu.sensi);
				}
			}
			if(key[KEY_LEFT] && !Menu.click && Menu.local == 1)
			{
				// Volume
				if(Menu.op == 0)
				{
					Menu.volume--;
					if(Menu.volume < 0)
						Menu.volume = 0;
					set_volume(Menu.volume, Menu.volume);
				} // RESOLUCAO
				else if(Menu.op == 1)
				{
					if(Menu.resolucao == 1)
					{
						Menu.resolucao = 0;
						set_gfx_mode(GFX_AUTODETECT_WINDOWED, ALTURA_TELA, LARGURA_TELA, 0, 0);
					}
					else
					{
						Menu.resolucao = 1;
						set_gfx_mode(GFX_AUTODETECT_FULLSCREEN, ALTURA_TELA, LARGURA_TELA, 0, 0);
					}
				} // sensibilidade
				else if(Menu.op == 2) 
				{
					Menu.sensi--;
					set_mouse_speed(Menu.sensi, Menu.sensi);
				}
			}
			TimerMenu = 0;
		}
		if(key[KEY_ENTER]) Menu.click = true;
		// Botao Voltar
		if(Menu.local == 1 && Menu.op == 3 && Menu.click == true)
		{
			Menu.click = false;
			Menu.op = 1;
			Menu.local--;
		}
		// Parte Principal Menu
		if(Menu.op < 0)
		{
			if(Menu.local == 0) Menu.op = 2;
			else if(Menu.local == 1) Menu.op = 3;
		}
		if(Menu.op > 2 && Menu.local == 0)
		{
			Menu.op = 0;
		}
		else if(Menu.op > 3 && Menu.local == 1)
		{
			Menu.op = 0;
		}
		// Opcoes do Menu
		if(Menu.op == 2 && Menu.click && Menu.local == 0)
		{
			Menu.sair = true;
		}
		else if(Menu.op == 1 && Menu.click && Menu.local == 0)
		{
			Menu.click = false;
			Menu.local++;
			Menu.op = 0;
		}
		else if(Menu.op == 0 && Menu.click && Menu.local == 0)
		{
			Menu.click = false;
			Menu.op = 0;
			// Inicializando os configuracoes do Jogo
			Init_Colisoes();
			// INICIALIZAÇÃO DE OBJETOS
			TPersonagem Jogador;
			TProjeteis Balas[NUM_BALAS];
			InitBalas(Balas, NUM_BALAS);
			TInimigos Inimigo;

			// Variaveis & Inicializacao de variaveis
			int vel = 5;
			char str[10];
			bool sair = false;
			int pressEsc = false;
			bool TocandoPassos = false;
			bool TocandoTiros[MAX_PLAYERS];
			bool InimigoMovimentando = false;
			//Inicializar Variaveis Players
			Jogador.x = 80; 
			Jogador.y = 280;
			Jogador.qntBalas = 30;
			Jogador.vida = 200;
			Jogador.vidamax = 200;
			TocandoTiros[playerid] = false;
			TimerOuvirPassos = 0;
			TimerTiros[playerid] = 0;
			TimerRecarregar[playerid] = 0;
			RodarTimerRecarregar[playerid] = false;
			RodarTimerTiro[playerid] = false;
			SalvarVida[playerid] = 200;
			// Inicializar Variaveis Bot's
			Inimigo.x = 670;
			Inimigo.y = 300;
			Inimigo.qntBalas = 30;
			Inimigo.vida = 200;
			Inimigo.vidamax = 200;
			TocandoTiros[NPC] = false;
			TimerTiros[NPC] = 0;
			TimerRecarregar[NPC] = 0;
			RodarTimerRecarregar[NPC] = false;
			RodarTimerTiro[NPC] = false;
			Inimigo.visao_x = 0;
			Inimigo.visao_y = 0;
			Inimigo.escutou_x = -1;
			Inimigo.escutou_y = -1;
			SalvarVida[NPC] = 200;
			// Fim Inicialização de variáveis
			LOCK_FUNCTION(incrementa_TimerOuvirPassos);
			LOCK_FUNCTION(incrementa_TimerTiros);
			LOCK_VARIABLE(incrementa_TimerRecarregar);
			LOCK_VARIABLE(TimerOuvirPassos);
			for(int x; x < MAX_PLAYERS; x++)
			{
				LOCK_VARIABLE(TimerTiros[x]);
				LOCK_VARIABLE(TimerRecarregar[x]);
			}
			install_int_ex(incrementa_TimerOuvirPassos, MSEC_TO_TIMER(250));
			install_int_ex(incrementa_TimerTiros, MSEC_TO_TIMER(80));
			install_int_ex(incrementa_TimerRecarregar, SECS_TO_TIMER(1));

			// Carregar Imagens
			BITMAP *buffer = create_bitmap(ALTURA_TELA,LARGURA_TELA);
			BITMAP *Personagem = load_bitmap("Imagens/Jogadores/player.bmp", NULL);
			BITMAP *InimigoBMP = load_bitmap("Imagens/Jogadores/player.bmp", NULL);
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
			while(!sair)
			{
				if(key[KEY_ESC])
				{
					pressEsc = true;
					Menu.local = 2;
					Menu.op = 0;
					Menu.click = false;
					TimerMenu = 0;
				}
				if(pressEsc)
				{
					masked_blit(background, buffer, 0, 0, 0, 0, ALTURA_TELA, LARGURA_TELA);
					Desenhar_Menu(buffer, myfont, Menu);
					if(TimerMenu > 1)
					{
						if(key[KEY_DOWN] && !Menu.click) Menu.op++;
						if(key[KEY_UP] && !Menu.click) Menu.op--;
						TimerMenu = 0;
					}
					if(Menu.op > 1)
						Menu.op = 0;
					if(Menu.op < 0)
						Menu.op = 1;
					if(key[KEY_ENTER]) Menu.click = true;
					// Resume Game
					if(Menu.local == 2 && Menu.op == 0 && Menu.click)
					{
						Menu.local = 0;
						Menu.click = false;
						pressEsc = false;
					} // Sair Menu
					else if(Menu.local == 2 && Menu.op == 1 && Menu.click)
					{
						Menu.local = 0;
						Menu.op = 0;
						Menu.click = false;
						sair = true;
					}
					draw_sprite(screen, buffer, 0, 0);
					rest(1);
					clear(buffer);
				}
				else
				{
					int ax,ay;
					ax = Jogador.x;
					ay = Jogador.y;
					if(key[KEY_D] && !key[KEY_LSHIFT]) // Direita
					{
						Jogador.x += 2;
						Inimigo.escutou_x = Jogador.x;
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
						Inimigo.escutou_x = Jogador.x;
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
						Inimigo.escutou_y = Jogador.y;
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
						Inimigo.escutou_y = Jogador.y;
						if(!TocandoPassos)
						{
							TocandoPassos = true;
						}
					}
					else if(key[KEY_S] && key[KEY_LSHIFT])
					{
						Jogador.y += 1;
					}
					if(key[KEY_R] && Jogador.qntBalas < 30 && !Jogador.recarregando) 
					{
						Jogador.recarregando = true;
						RodarTimerRecarregar[playerid] = true;
					}
					if(TimerRecarregar[playerid] > 3 && Jogador.recarregando) 
					{
						Jogador.qntBalas = 30;
						TimerRecarregar[playerid] = 0;
						Jogador.recarregando = false;
						RodarTimerRecarregar[playerid] = false;
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
					if(TimerTiros[playerid] > 1) // 6 Tiro por tiro 1 Spray
					{
						if(TocandoTiros[playerid])
						{
							TocandoTiros[playerid] = false;
							TimerTiros[playerid] = 0;
							RodarTimerTiro[playerid] = false;
						}
					}
					if(mouse_b)
					{
						if(!TocandoTiros[playerid] && Jogador.qntBalas != 0)
						{	
							play_sample(STiro, 255, 128, 1000, 0);
							AtiraBalas(Balas, NUM_BALAS, Jogador);
							Jogador.qntBalas--;
							TocandoTiros[playerid] =  true;
							RodarTimerTiro[playerid] = true;
						}
					}
					if(Inimigo.qntBalas == 0 && !Inimigo.recarregando) 
					{
						Inimigo.recarregando = true;
						RodarTimerRecarregar[NPC] = true;
					}
					if(TimerRecarregar[NPC] > 3 && Inimigo.recarregando) 
					{
						Inimigo.qntBalas = 30;
						TimerRecarregar[NPC] = 0;
						Inimigo.recarregando = false;
						RodarTimerRecarregar[NPC] = false;
					} 
					if(TimerTiros[NPC] > 1) 
					{
						if(TocandoTiros[NPC])
						{
							TocandoTiros[NPC] = false;
							TimerTiros[NPC] = 0;
							RodarTimerTiro[NPC] = false;
						}
					}
					int px = Jogador.x-15;
					int py = Jogador.y-25;
					Inimigo.enxergando = Checar_VisaoInimigo(Inimigo, px, py);
					if(!TocandoTiros[NPC] && Inimigo.qntBalas != 0 && Inimigo.enxergando == true)
					{
						play_sample(STiro, 255, 128, 1000, 0);
						AtiraBalasInimigos(Balas, NUM_BALAS, Inimigo, Jogador);
						Inimigo.qntBalas--;
						TocandoTiros[NPC] = true;
						RodarTimerTiro[NPC] = true;
					}
					// Verificar Colisao
					if(Checar_Colisao(Jogador.x, Jogador.y) || Jogador.y < 0 || Jogador.x < 0 || Jogador.y >= 550 || Jogador.x > 740) 
					{
						Jogador.x = ax;
						Jogador.y = ay;
					}
					if(Inimigo.enxergando)
					{
						Inimigo.escutou_x = -1;
						Inimigo.escutou_y = -1;
					}
					else
					{
						int arm_x = 0;
						int arm_y = 0;
						if(Inimigo.escutou_x != -1 && Inimigo.x != Inimigo.escutou_x)
						{
							arm_x = SetarPos_x(Inimigo);
							if(Checar_Colisao(arm_x, Inimigo.y))
							{
								Inimigo.y += 2;
							}
							else Inimigo.x = arm_x;
						}
						else if(Inimigo.x == Inimigo.escutou_x)
							Inimigo.escutou_x = -1;
						if(Inimigo.escutou_y != -1 && Inimigo.y != Inimigo.escutou_y)
						{
							arm_y = SetarPos_y(Inimigo);
							if(Checar_Colisao(Inimigo.x, arm_y))
							{
								Inimigo.x += 2;
							}
							else Inimigo.y = arm_y;
						}
						else if(Inimigo.escutou_y == Inimigo.y)
							Inimigo.escutou_y = -1;
					}
					// Fim Colisao
					Desenhar_Mapa(buffer, mapa, linhas, colunas);
					// Desenhar Colisões
					Desenhar_Colisoes(buffer);
					AtualizarBalas(Balas, NUM_BALAS, Jogador, Inimigo);
					Jogador.vida = SalvarVida[playerid];
					Inimigo.vida = SalvarVida[NPC];
					TPersonagem Angulo = CalcularAngulo(mouse_x, mouse_y, mouse_z, Jogador);
					Jogador.z = Angulo.z;
					rotate_sprite(buffer, Personagem, Jogador.x, Jogador.y, itofix(GRAUS_PARA_ALLEGRO(Angulo.z)));
					TInimigos AnguloInimigo = CalcularAnguloInimigo(Jogador.x, Jogador.y, Jogador.z, Inimigo);
					if(Inimigo.enxergando) 
					{
						Inimigo.z = AnguloInimigo.z;
						rotate_sprite(buffer, InimigoBMP, Inimigo.x, Inimigo.y, itofix(GRAUS_PARA_ALLEGRO(Inimigo.z)));
					}
					else rotate_sprite(buffer, InimigoBMP, Inimigo.x, Inimigo.y, itofix(GRAUS_PARA_ALLEGRO(Inimigo.z)));
					draw_sprite(buffer, Mira, mouse_x, mouse_y);
					DesenharBalas(buffer, Balas, NUM_BALAS);
					//Textos
					sprintf(str, "Balas: %d", Jogador.qntBalas);
					textout_ex(buffer, font, str, 30, 10, makecol(255,0,0), -1);
					if(Jogador.recarregando) 
						textout_ex(buffer, font, "Recarregando, Aguarde..", Jogador.x-30, Jogador.y-10, makecol(255,0,0), -1);
					Barra_Vida(buffer, Jogador);
					Barra_Vida_Inimigo(buffer, Inimigo);
					// Morto
					if(Jogador.vida <= 0 || Inimigo.vida <= 0)
						sair = true;
					//Fim
					draw_sprite(screen, buffer, 0, 0);
					rest(1);
					clear(buffer);
				}
			}
			destroy_bitmap(Personagem);
			destroy_bitmap(Mira);
			destroy_bitmap(buffer);
			destroy_bitmap(InimigoBMP);
			destroy_sample(Caminhar);
			Libera_Mapa(mapa, linhas);
		}
		draw_sprite(screen, tela_menu, 0, 0);
		clear(tela_menu);
	}
	SalvarConf(Menu);
	destroy_font(myfont);
	destroy_bitmap(background);
	destroy_bitmap(tela_menu);
  	return 0;
}

END_OF_MAIN();