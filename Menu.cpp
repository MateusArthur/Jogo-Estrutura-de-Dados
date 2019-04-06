#include <iostream>
#include <allegro.h>

 
// Atributos da tela
#define LARGURA_TELA 640
#define ALTURA_TELA 480

// Objetos
struct tPersonagem {
	int wx;
	int wy;
	int x;
	int y;
	int w;
	int h;
};

int main(void)
{
	// Inicializando e configurando o Jogo
	allegro_init();
	install_timer();
	install_keyboard();
	set_color_depth(32);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, 800, 600, 0, 0);
	set_window_title("Counter Strike 2D");
	install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL);
	// Variaveis
	int x = 100, y = 100, vel = 5;
	// Icone do Jogo

	// Carregar Imagens
	BITMAP *buffer = create_bitmap(800,600);
	BITMAP *Personagem[5];
	Personagem[0] = load_bitmap("Imagens/Jogadores/ctc.bmp", NULL);
	Personagem[1] = load_bitmap("Imagens/Jogadores/ctb.bmp", NULL);
	Personagem[2] = load_bitmap("Imagens/Jogadores/ctd.bmp", NULL);
	Personagem[3] = load_bitmap("Imagens/Jogadores/cte.bmp", NULL);
	Personagem[4] =  load_bitmap("Imagens/Jogadores/ctc.bmp", NULL);
	BITMAP *background  = load_bitmap("Imagens/background_1.bmp", NULL);

	//Sons
	SAMPLE *Caminhar = load_sample("Sons/Jogadores/Correndo.wav");
	// Looping do Jogo
	while(!key[KEY_ESC])
	{
		if(key[KEY_D] && !key[KEY_LSHIFT]) // Direita
		{
			x += 2;
			Personagem[0] = Personagem[2];
			//			Var       Vol  Can  Vel Rep
			play_sample(Caminhar, 255, 128, 1000, 0);
		}
		else if(key[KEY_D] && key[KEY_LSHIFT]) // Direita
		{
			x += 1;
			Personagem[0] = Personagem[2];
		}
		if(key[KEY_A] && !key[KEY_LSHIFT]) // Esquerda
		{ 
			x -= 2;
			Personagem[0] = Personagem[3];
			play_sample(Caminhar, 255, 128, 1000, 0);
		}
		else if(key[KEY_A] && key[KEY_LSHIFT]) // Esquerda
		{ 
			x -= 1;
			Personagem[0] = Personagem[3];
		}
		if(key[KEY_W] && !key[KEY_LSHIFT]) // Cima
		{ 
			y -= 2;
			Personagem[0] = Personagem[4];
			play_sample(Caminhar, 255, 128, 1000, 0);
		}
		else if(key[KEY_W] && key[KEY_LSHIFT]) // Cima
		{ 
			y -= 1;
			Personagem[0] = Personagem[4];
		}
		if(key[KEY_S] && !key[KEY_LSHIFT]) // Baixo
		{
			y += 2;
			Personagem[0] = Personagem[1];
			play_sample(Caminhar, 255, 128, 1000, 0);
		}
		else if(key[KEY_S] && key[KEY_LSHIFT])
		{
			y += 1;
			Personagem[0] = Personagem[1];
		}
		masked_blit(background, buffer, 0, 0, 0, 0, 900, 600);
		//masked_blit(Personagem, buffer, 0, 0, 0, 0, 900+x, 600+y);
		draw_sprite(buffer, Personagem[0], 100+x,100+y);
		draw_sprite(screen, buffer, 0, 0);

		rest(5);
		clear(buffer);
	}
	for(int x = 0; x<5; x++)
		destroy_bitmap(Personagem[x]);
	destroy_bitmap(buffer);
	destroy_bitmap(background);
	destroy_sample(Caminhar);
  	return 0;
}
END_OF_MAIN();