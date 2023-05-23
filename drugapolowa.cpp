#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include <time.h>
#include "defines_structs_functions.h"

//ustaw podstawowe dane zmiennym
void setDefValues(gameData* game) {
	if (game->menuPos != NULL) {
		for (int i = 0; i < game->actMenuPos; i++)
			delete[] game->menuPos[i];
		delete[] game->menuPos;
		game->menuPos = NULL;
	}
	if (game->menuPos != NULL) {
		for (int i = 0; i < game->actMenuPos; i++)
			delete[] game->scoreBoard[i];
		delete[] game->scoreBoard;
		game->scoreBoard = NULL;
	}
	game->actMenuPos = NULL;
	game->canAddScore = 1;
	game->startMenu = 0;
	game->disToNewRoad = roadChangeDis;
	game->roadChangePosY = 1;
	game->roadWidth = defRoadWidth;
	game->roadNewWidth = defRoadWidth;
	game->actPickups = 0;
	game->pickedMenuPos = 1;
	game->actMenuPos = 0;
	game->loadMenu = 0;
	game->scoreMenu = 0;
	game->revivedProtection = 0;
	game->actBullets = 0;
	game->bullets = NULL;
	game->lockPts = 0;
	game->actNPCs = 0;
	game->actDecs = 0;
	game->actEnemies = 0;
	game->paused = 0;
	game->revives = defRevives;
	game->col = 0;
	game->frames = 0;
	game->fps = 0;
	game->quit = 0;
	game->worldTime = 0;
	game->lastRevive = 0;
	game->distance = 0;
	game->pts = 0;
	game->prevDistance = 0;
	game->waitForNewGame = 0;
	game->speed = startSpeed;
	game->timers.killColorTimer = 0;
	game->timers.deathScreenTimer = 0;
	game->timers.fpsTimer = 0;

	if (game->twoPlayers) {
		game->car[0].pos.x = SCREEN_WIDTH / 2 - twoPlrsOff;
		game->car[1].pos.x = SCREEN_WIDTH / 2 + twoPlrsOff;
	}
	else
		game->car[0].pos.x = startX;

	for (int i = 0; i < 2; i++) {
		game->car[i].bulletOffTimer = 0;
		game->car[i].pos.y = startY;
		game->car[i].shooting = 0;
		game->car[i].destroyed = notDestroyed;
		game->car[i].shootingDis = basicShootingDis;
		game->car[i].shootingRate = basicShootingRate;
		game->car[i].ammoFasterLeft = 0;
		game->car[i].ammoLongerLeft = 0;
		game->car[i].lastBulletOnLeft = 0;
		game->car[i].dead = 0;
	}
}

//zwolnij pamiec zarezerwowana dla SDL_Surface
void freeMem(SDLdata* data) {
	SDL_FreeSurface(data->charset);
	SDL_FreeSurface(data->charsetGreen);
	SDL_FreeSurface(data->screen);
	SDL_FreeSurface(data->bullet);
	SDL_FreeSurface(data->car);
	SDL_FreeSurface(data->car2);
	SDL_FreeSurface(data->tree1);
	SDL_FreeSurface(data->tree2);
	SDL_FreeSurface(data->rock1);
	SDL_FreeSurface(data->moreDis);
	SDL_FreeSurface(data->carNPC);
	SDL_FreeSurface(data->fasterShoot);
	SDL_FreeSurface(data->newCar);
	SDL_FreeSurface(data->enemy1);
	SDL_FreeSurface(data->boomGreen);
	SDL_FreeSurface(data->boomGrey);
	SDL_FreeSurface(data->carNPCdesR_green);
	SDL_FreeSurface(data->carNPCdesL_green);
	SDL_FreeSurface(data->carNPCdesR);
	SDL_FreeSurface(data->carNPCdesL);
	SDL_DestroyTexture(data->scrtex);
	SDL_DestroyWindow(data->window);
	SDL_DestroyRenderer(data->renderer);
}

//zwraca losowego inta miedzy 0 a maxVal wlacznie
int random(int maxValue, int seedOff, gameData game) {

	//nie magiczne stale tylko losowe duze liczby - o ile nie beda zerem moga byc jakiekolwiek

	int result = -1;
	if (seedOff == 0)
		seedOff = 1;
	else
		seedOff *= random(maxValue * game.pts, seedOff - 1, game);
	int div = (int)(game.delta * 100000);
	if (div == 0)
		div = 1234;

	while (result < 0) {
		result = seedOff * (SDL_GetTicks() * (int)game.pts * 28293842 / div - (int)game.car[0].pos.x + (int)game.car[0].pos.y + 10293) % (maxValue + 1);
	}
	return result;
}

//losuje true lub false z szansa chance% na wylosowanie true, randOff pozwala przemieszac wyniki
bool precChance(double chance, int randOff, gameData game) {
	if (chance - (int)chance != 0) {
		double gamble = random(100 / chance, randOff, game);
		while (chance - (int)chance != 0) {
			chance *= 10;
		}
		if (chance >= gamble)
			return true;
		return false;
	}

	double gamble = random(100, 0, game);
	if (chance >= gamble)
		return true;
	return false;
}

//zwaraca true jezelei pos jest na drodze, false w przeciwnym razie
bool isOnRoad(position pos, gameData game, SDLdata data) {
	if (pos.y > game.roadChangePosY && fabs(pos.x - SCREEN_WIDTH / 2) < game.roadWidth / 2)
		return true;
	if (pos.y < game.roadChangePosY && fabs(pos.x - SCREEN_WIDTH / 2) < game.roadNewWidth / 2)
		return true;
	return false;
}

//ustawia niezbedne rzeczy do wyswietlania okna, true jezeli wszystko poprawnie ustawiono, przeciwnie false
bool SDLsetup(SDLdata* data) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &(data->window), &(data->renderer));

	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(data->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(data->window, "Jan Kaczerski 193237");


	data->screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	data->scrtex = SDL_CreateTexture(data->renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_ShowCursor(SDL_DISABLE);

	data->czarny = SDL_MapRGB(data->screen->format, 0x00, 0x00, 0x00);
	data->zielony = SDL_MapRGB(data->screen->format, 0x00, 0xFF, 0x00);
	data->czerwony = SDL_MapRGB(data->screen->format, 0xFF, 0x00, 0x00);
	data->niebieski = SDL_MapRGB(data->screen->format, 0x11, 0x11, 0xCC);
	data->szary = SDL_MapRGB(data->screen->format, 0x60, 0x60, 0x60);
	return 0;
}

//sprawdza czy poprawnia zaladowano bitmape
bool checkSurface(SDL_Surface* surf, SDLdata* data, char name[]) {
	if (surf == NULL) {
		printf("SDL_LoadBMP(");
		printf(name);
		printf(") error: %s\n", SDL_GetError());
		freeMem(data);
		SDL_Quit();
		return 1;
	}
	return 0;
}

//zaladowanie bitmap do struktury data, zwraca true jezeli nie udalo sie zaladowac jednej z grafik, przeciwnie false
bool loadImgs(SDLdata* data) {
	//charset
	data->charset = SDL_LoadBMP("./cs8x8.bmp");
	if (checkSurface(data->charset, data, "cs8x8.bmp"))
		return 1;

	//samochod
	data->car = SDL_LoadBMP("./car.bmp");
	if (checkSurface(data->car, data, "car.bmp"))
		return 1;

	//drzewo 1
	data->tree1 = SDL_LoadBMP("./tree1.bmp");
	if (checkSurface(data->tree1, data, "tree_one.bmp"))
		return 1;

	//drzewo 2
	data->tree2 = SDL_LoadBMP("./tree2.bmp");
	if (checkSurface(data->tree2, data, "tree2.bmp"))
		return 1;

	//skala 1
	data->rock1 = SDL_LoadBMP("./rock1.bmp");
	if (checkSurface(data->rock1, data, "rock1.bmp"))
		return 1;

	//samochod NPC
	data->carNPC = SDL_LoadBMP("./car_NPC.bmp");
	if (checkSurface(data->carNPC, data, "car_NPC.bmp"))
		return 1;

	//samochod enemy
	data->enemy1 = SDL_LoadBMP("./car_enemy1.bmp");
	if (checkSurface(data->enemy1, data, "car_enemy1.bmp"))
		return 1;

	//explosion green
	data->boomGreen = SDL_LoadBMP("./explosion_green.bmp");
	if (checkSurface(data->boomGreen, data, "explosion_green.bmp"))
		return 1;

	//explosion grey
	data->boomGrey = SDL_LoadBMP("./explosion_grey.bmp");
	if (checkSurface(data->boomGrey, data, "explosion_grey.bmp"))
		return 1;

	//anim NPC z prawej z traw¹
	data->carNPCdesR_green = SDL_LoadBMP("./car_NPC_des_fromRight_grass.bmp");
	if (checkSurface(data->carNPCdesR_green, data, "car_NPC_des_fromRight_grass.bmp"))
		return 1;

	//anim NPC z lewej z traw¹
	data->carNPCdesL_green = SDL_LoadBMP("./car_NPC_des_fromLeft_grass.bmp");
	if (checkSurface(data->carNPCdesL_green, data, "car_NPC_des_fromLeft_grass.bmp"))
		return 1;

	//anim NPC z prawej
	data->carNPCdesR = SDL_LoadBMP("./car_NPC_des_fromRight.bmp");
	if (checkSurface(data->carNPCdesR, data, "car_NPC_des_fromRight.bmp"))
		return 1;

	//anim NPC z lewej
	data->carNPCdesL = SDL_LoadBMP("./car_NPC_des_fromLeft.bmp");
	if (checkSurface(data->carNPCdesL, data, "car_NPC_des_fromLeft"))
		return 1;

	//charset red
	data->charsetRed = SDL_LoadBMP("./cs8x8_red.bmp");
	if (checkSurface(data->charsetRed, data, "cs8x8_red.bmp"))
		return 1;

	//charset green
	data->charsetGreen = SDL_LoadBMP("./cs8x8_green.bmp");
	if (checkSurface(data->charsetGreen, data, "cs8x8_green.bmp"))
		return 1;

	//bullet
	data->bullet = SDL_LoadBMP("./bullet.bmp");
	if (checkSurface(data->bullet, data, "bullet.bmp"))
		return 1;

	//dis powerup
	data->moreDis = SDL_LoadBMP("./pickup_longerShoot.bmp");
	if (checkSurface(data->moreDis, data, "pickup_longerShoot.bmp"))
		return 1;

	//speed powerup
	data->fasterShoot = SDL_LoadBMP("./pickup_fasterShoot.bmp");
	if (checkSurface(data->fasterShoot, data, "pickup_fasterShoot.bmp"))
		return 1;

	//new car powerup
	data->newCar = SDL_LoadBMP("./pickup_newCar.bmp");
	if (checkSurface(data->newCar, data, "pickup_newCar.bmp"))
		return 1;

	//new car powerup
	data->car2 = SDL_LoadBMP("./car_2nd.bmp");
	if (checkSurface(data->car2, data, "car_2nd.bmp"))
		return 1;

	return 0;
}


void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	if (x >= 0 && x <= SCREEN_WIDTH && y >= 0 && y <= SCREEN_HEIGHT) {
		int bpp = surface->format->BytesPerPixel;
		Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
		*(Uint32*)p = color;
	}
};

void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};

void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};