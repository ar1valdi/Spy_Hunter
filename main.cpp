#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<time.h>

#include"defines_structs_functions.h"
const char* zrobionePunkty = "abcdefgghijklmnoqr";

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#ifdef __cplusplus
extern "C"
#endif

//zmniejsza timer zablokowanych punktów
void handlePtsLock(gameData* game, SDLdata *data) {
	if (game->lockPts > 0) {
		if(!game->revivedProtection && !(game->car[0].dead || game->car[1].dead))
			game->lockPts -= game->delta;
	}
	else if (game->lockPts < 0) {
		game->lockPts = 0;
	}
}

//zapisuje czas miedzy klatkami do game->delta
void updateTimerPaused(gameData* game) {
	game->t2 = SDL_GetTicks();
	game->delta = (game->t2 - game->t1) * 0.001; //t2 i t1 są w milisekundach, *0.001 zamienia na sekundy
	game->t1 = game->t2;
}

//aktualizuje globalne timery
void updateTimer(gameData *game) {
	updateTimerPaused(game);
	if(!game->revivedProtection)
		game->worldTime += game->delta;

	if (game->timers.killColorTimer > 0)
		game->timers.killColorTimer -= game->delta;
	else
		game->timers.killColorTimer = 0;
}

//aktualizuje liczbe FPSow
void updateFPS(gameData *game) {
	game->timers.fpsTimer += game->delta;
	if (game->timers.fpsTimer > 0.5) {	//update co 0.5s
		game->fps = game->frames * 2;
		game->frames = 0;
		game->timers.fpsTimer -= 0.5;
	};
}

//aktualizuje przejechany dystans
void updateDistance(gameData* game) {
	game->prevDistance = game->distance;
	game->distance += game->speed * game->delta;
}

//dodaje punkty za dystans uwzgledniajac blokade na punkty
void updatePoints(gameData* game, SDLdata data) {
	if (game->lockPts == 0) {
		if(isOnRoad(game->car[0].pos, *game, data) && isOnRoad(game->car[1].pos, *game, data))
		game->pts += scoreSpeed * (game->distance - game->prevDistance);
	}
}

//wyrysowanie gornego HUD
void drawTopbar(gameData*game, SDLdata *data) {
	int players = game->twoPlayers ? 1 : 0;
	char text[bufSize];
	char text2[bufSize];
	const int firstVerse = topbarY + borderOffsizeY;
	const int nextVerse = letterSize + versesOffset;
	DrawRectangle(data->screen, topbarX, topbarY, SCREEN_WIDTH - topbarX*2, firstVerse +nextVerse*(4+players), data->czerwony, data->niebieski);

	SDL_Surface* txtColor = data->charset;

	//1 wers
	sprintf_s(text, "Jan Kaczerski 193237, czas trwania = %.1lf s  %.0lf klatek / s", game->worldTime, game->fps);
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, firstVerse, text, data->charset);

	//2 wers
	sprintf_s(text, "Punkty = %f    Rozbici przeciwnicy = %d", game->pts, game->col);
	if (game->lockPts != 0) txtColor = data->charsetRed;
	else if (game->timers.killColorTimer != 0) txtColor = data->charsetGreen;
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, firstVerse + nextVerse * ptsColsPos, text, txtColor);

	//3 wers - amunicja
	for (int i = 0; i <= players; i++) {

		if (game->twoPlayers) {
			sprintf_s(text, "Gracz %d: Bron: ", i+1);
		}
		else
			strcpy(text, "Bron: ");


		if (game->car[i].ammoLongerLeft > 0)
			sprintf_s(text2, "dluga (%d)", game->car[i].ammoLongerLeft);
		else
			sprintf_s(text2, "krotka (inf)");
		strcat(text, text2);

		if (game->car[i].ammoFasterLeft > 0)
			sprintf_s(text2, "szybka (%d)", game->car[i].ammoFasterLeft);
		else { sprintf_s(text2, "zwykla (inf)"); }
		strcat(text, "     Amunicja: ");
		strcat(text, text2);
		DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, firstVerse + nextVerse * (effectsPos+i), text, data->charset);
	}

	//4 wers - nowe pojazdy
	if (infRevivesTime - game->worldTime > 0) {
		sprintf_s(text, "Nielimitowane pojazdy: %f", infRevivesTime - game->worldTime); 
		txtColor = data->charsetGreen;
	}
	else {
		sprintf_s(text, "Dostepne pojazdy: %d     Nastepny za: %f", game->revives, ptsReqToRev - (game->pts - game->lastRevive));
		txtColor = data->charset;
	}
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, firstVerse + nextVerse * (revsPos+players), text, txtColor);
}

//wyrysuj ekran smierci
void handleDeathScreen(gameData* game, SDLdata* data) {
	game->timers.deathScreenTimer += game->delta;
		game->waitForNewGame = 1;

	char text[bufSize];
	const short rowsToShow = 5;
	DrawRectangle(data->screen, (SCREEN_WIDTH - deathScreenWidth)/2, (SCREEN_HEIGHT - (2 * 8 * rowsToShow))/2 - 10, deathScreenWidth, (2 * 8 * rowsToShow) + 10, data->czarny, data->czerwony);

	const int firstVerse = (SCREEN_HEIGHT - (versesOffset + letterSize) * rowsToShow)/2;
	const int nextVerse = versesOffset + letterSize;

	sprintf_s(text, "PRZEGRALES");
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, firstVerse, text, data->charset);
	sprintf_s(text, "Punkty = %f", game->pts);
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, firstVerse + nextVerse, text, data->charset);
	sprintf_s(text, "Rozbici przeciwnicy = %d", game->col);
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, firstVerse + nextVerse * 2, text, data->charset);
	sprintf_s(text, "Czas gry = %f", game->worldTime);
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, firstVerse + nextVerse * 3, text, data->charset);
	sprintf_s(text, "N - nowa gra     F - najlepsze wyniki     ESC - wyjdz");
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, firstVerse + nextVerse * 4, text, data->charset);
}

//wyrysuj protoskat z zaimplementowanymi elementami
void drawDownbar(SDLdata* data) {
	char text[bufSize];
	sprintf_s(text, zrobionePunkty);
	DrawRectangle(data->screen, SCREEN_WIDTH - strlen(text)*8 - 16, SCREEN_HEIGHT - 24, strlen(text)*8 + 8, 20, data->czerwony, data->niebieski);
	DrawString(data->screen, downbarX - strlen(text) * letterSize, downbarY, text, data->charset);
}

//obsłuż klawisze podczas deathscreena
void waitForNewGame(gameData* game, SDLdata *data) {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_n) { 
				game->car[0].dead = 0;
				game->car[1].dead = 0;
				game->waitForNewGame = 0;
				newGame(game);
				game->startMenu = 1;
				initGameMenu(game, data, chPlayers);
			}
			if (event.key.keysym.sym == SDLK_f) {
				game->scoreMenu = 1;
				initGameMenu(game, data, scores);
			}
			if (event.key.keysym.sym == SDLK_ESCAPE) game->quit = 1;
			break;
			break;
		};
	};
}

//obsłuż klawisze podczas pauzy
void checkUnpause(gameData* game, SDLdata *data) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_p) game->paused = 0;
			if (event.key.keysym.sym == SDLK_ESCAPE) game->quit = 1;
			if (event.key.keysym.sym == SDLK_l) {
				game->loadMenu = 1;
				initGameMenu(game, data, load);
			}
			if (event.key.keysym.sym == SDLK_n) { 
				newGame(game);
				game->startMenu = 1;
				initGameMenu(game, data, chPlayers);
			}
			if (event.key.keysym.sym == SDLK_f) {
				game->paused = 0;
				game->scoreMenu = 1;
				game->canAddScore = 1;
				initGameMenu(game, data, scores);
			}
			if (event.key.keysym.sym == SDLK_s) saveGame(game);
			break;
		};
	};
}

//obługa klawiszy podczas gry
void handleEvent(gameData *game , SDLdata *data) {
	game->car[0].prevPos = game->car[0].pos;
	game->car[1].prevPos = game->car[1].pos;

	SDL_PumpEvents();
	const Uint8* keystate = SDL_GetKeyboardState(NULL);

	int firstPlaShooting = SDL_SCANCODE_SPACE;		//1 gracz strzela spacja jezeli gra sam, jezeli w 2 osoby to strzela KP_0
	if(game->twoPlayers)
		firstPlaShooting = SDL_SCANCODE_KP_0;


	//==================== 1 Gracz ====================
	if (keystate[SDL_SCANCODE_LEFT] && !game->revivedProtection) game->car[0].pos.x -= moveSpeedH * game->delta;
	else if (keystate[SDL_SCANCODE_RIGHT] && !game->revivedProtection) game->car[0].pos.x += moveSpeedH * game->delta;
	if (keystate[SDL_SCANCODE_UP]) { 
		if (game->speed < maxSpeed)
			game->speed += speedingSpeed * game->delta;
		else
			game->speed = maxSpeed;
	}
	else if (keystate[SDL_SCANCODE_DOWN]) { 
		if (game->speed > 0)
			game->speed -= slowingSpeed * game->delta;
		else
			game->speed = 0;
	}
	if (keystate[firstPlaShooting] && !game->revivedProtection)
		game->car[0].shooting = 1;
	else
		game->car[0].shooting = 0;
	//=================================================
	


	//==================== 2 Gracz ====================
	if (game->twoPlayers) {
		if (keystate[SDL_SCANCODE_H] && !game->revivedProtection) game->car[1].pos.x -= moveSpeedH * game->delta;
		else if (keystate[SDL_SCANCODE_K] && !game->revivedProtection) game->car[1].pos.x += moveSpeedH * game->delta;
		if (keystate[SDL_SCANCODE_U]) {
			if (game->speed < maxSpeed)
				game->speed += speedingSpeed * game->delta;
			else
				game->speed = maxSpeed;
		}
		else if (keystate[SDL_SCANCODE_J]) {
			if (game->speed > 0)
				game->speed -= slowingSpeed * game->delta;
			else
				game->speed = 0;
		}

		if (keystate[SDL_SCANCODE_SPACE] && !game->revivedProtection)
			game->car[1].shooting = 1;
		else
			game->car[1].shooting = 0;
	}

	if (game->revivedProtection && game->speed != 0) {
		game->speed = startSpeedAfterDeath;
		game->revivedProtection = 0;
	}
	//=================================================


	//game control
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_p) game->paused = 1;
			if (event.key.keysym.sym == SDLK_ESCAPE) game->quit = 1;
			if (event.key.keysym.sym == SDLK_n) { 
				initGameMenu(game, data, chPlayers);
				game->startMenu = 1;
			}
			if (event.key.keysym.sym == SDLK_s) saveGame(game);
			if (event.key.keysym.sym == SDLK_l) { 
				game->loadMenu = 1;
				initGameMenu(game, data, load);
			}
			if (event.key.keysym.sym == SDLK_f) {
				game->scoreMenu = 1;
				game->canAddScore = 1;
				initGameMenu(game, data, scores);
			}
			break;
		};
	};
}

//przesun gracza gdy nie ma kontroli nad pojazdem
void movePlayer(gameData* game) {
	int players = game->twoPlayers ? 1 : 0;
	for (int i = 0; i <= players; i++) {

		if (game->car[0].dead || game->car[1].dead) {
			game->car[i].pos.y += game->speed * game->delta;
			continue;
		}

		if (game->car[i].destroyed == fromLeft) {
			game->car[i].pos.x += pushSpeed * game->delta;
		}
		else if (game->car[i].destroyed == fromRight) {
			game->car[i].pos.x -= pushSpeed * game->delta;
		}
	}
}

//stwórz dekoracje
void generateDecs(gameData* game, SDLdata *data) {
	if (precChance(decsSpawnrate, 0, *game) && game->actDecs < maxDecsNumber) {
		{
			double newDecX;
			bool end = false;
			int licznik = 0;
			do {
				end = true;
				newDecX = random(SCREEN_WIDTH - (decsOffsetBorder * 2), 0, *game) + decsOffsetBorder;

				for (int i = 0; i < maxDecsNumber; i++) {
					if ((fabs(newDecX - game->decs[i].pos.x) <= decsOffset && game->decs[i].pos.y < decsOffset) || 
						fabs(newDecX - SCREEN_WIDTH/2) < (double)decsFromRoadOffset + (double)game->roadNewWidth / 2)
					{
						end = false;
						break;
					}
				}

				licznik++;
				if (licznik >= 20)
				{
					end = false;
					break;
				}
			} while (!end);

			if (end) {
				game->decs[game->actDecs] = { {newDecX, 0}, random(3, 0, *game) + 1 };
				game->actDecs++;
			}

		}
	}
}

//przesun dekoracje w dol
void moveDecs(gameData* game){
	for (int i = 0; i < game->actDecs; i++) {
		game->decs[i].pos.y += game->speed*game->delta;
	}
	char txt[bufSize];
}

//zniszcz dekoracje poza ekranem
void destroyDecs(gameData* game) {
	int deleted = 0;
	for (int i = 0; i < game->actDecs; i++) {
		if (game->decs[i].pos.y >= SCREEN_HEIGHT+100) {
			game->decs[i].pos.x = -1;
			game->decs[i].pos.y = -1;
			game->decs[i].type = -1;
			game->decs[i].destroyed = notDestroyed;
			deleted++;

			char text[bufSize];
		}
	}

	if (deleted != 0) {		//reindekswoanie tablicy
		for (int i = 0; i < game->actDecs; i++) {
			char text[bufSize];
			if (game->decs[i].pos.x == -1) {
				for (int j = i+1; j < game->actDecs; j++) {
					if (game->decs[j].pos.x != -1) {
						game->decs[i].pos.x = game->decs[j].pos.x;
						game->decs[i].pos.y = game->decs[j].pos.y;
						game->decs[i].type = game->decs[j].type;
						game->decs[i].destroyed = game->decs[j].destroyed;
						game->decs[j].pos.x = -1;
						game->decs[j].pos.y = -1;
						game->decs[j].type = -1;
						game->decs[j].destroyed = notDestroyed;
						char text[bufSize];
						break;
					}
				}
			}
		}
	}

	game->actDecs -= deleted;
}

//obsłuż dekoracje
void handleDecs(gameData *game, SDLdata *data) {
	if (game->worldTime > 0.5) {
		generateDecs(game, data);
		moveDecs(game);
		destroyDecs(game);
	}
}

//wyryusuj bitmapy w grze
void drawSprites(gameData* game, SDLdata* data) {
	for (int i = 0; i < game->actDecs; i++) {		//dekoracje
		SDL_Surface* surf;
		if (game->decs[i].destroyed)
			surf = data->boomGreen;
		else {
			switch (game->decs[i].type) {
			case drzewo1:
				surf = data->tree1;
				break;
			case drzewo2:
				surf = data->tree2;
				break;
			case skala1:
				surf = data->rock1;
				break;
			default:
				continue;
				break;
			}
		}
		DrawSurface(data->screen, surf, game->decs[i].pos.x, game->decs[i].pos.y);
	}

	DrawRectangle(data->screen, SCREEN_WIDTH / 2 - game->roadNewWidth / 2, 0, game->roadNewWidth, game->roadChangePosY, data->szary, data->szary);
	DrawRectangle(data->screen, SCREEN_WIDTH / 2 - game->roadWidth / 2, game->roadChangePosY, game->roadWidth, SCREEN_HEIGHT - game->roadChangePosY, data->szary, data->szary);

	for (int i = 0; i < game->actPickups; i++) {		//pickupy
		SDL_Surface* surf;
		switch (game->pickups[i].mode) {
		case ammoFaster:
			surf = data->fasterShoot;
			break;
		case ammoLonger:
			surf = data->moreDis;
			break;
		case addCar:
			surf = data->newCar;
			break;
		default:
			continue;
			break;
		}
		DrawSurface(data->screen, surf, game->pickups[i].pos.x, game->pickups[i].pos.y);
	}


	for (int i = 0; i < game->actNPCs; i++) {		//npc
		SDL_Surface* surf = NULL;
		if (game->npcCars[i].destroyed == explode || game->npcCars[i].destroyed == fromUp || game->npcCars[i].destroyed == fromDown) {
			if (isOnRoad(game->npcCars[i].pos, *game, *data))
				surf = data->boomGrey;
			else
				surf = data->boomGreen;
		}
		else if (game->npcCars[i].destroyed) {
			if (game->npcCars[i].destroyed == fromRight) {
				if (!isOnRoad(game->npcCars[i].pos, *game, *data)) {
					surf = data->carNPCdesR_green;
				}
				else {
					surf = data->carNPCdesR;
				}
			}
			if (game->npcCars[i].destroyed == fromLeft) {
				if (!isOnRoad(game->npcCars[i].pos, *game, *data)) {
					surf = data->carNPCdesL_green;
				}
				else {
					surf = data->carNPCdesL;
				}
			}
		}
		else
			surf = data->carNPC;

		DrawSurface(data->screen, surf, game->npcCars[i].pos.x, game->npcCars[i].pos.y);
	}

	for (int i = 0; i < game->actEnemies; i++) {		//przeciwnicy
		SDL_Surface* surf = NULL;
		if (game->enemies[i].destroyed == explode || game->enemies[i].destroyed == fromUp || game->enemies[i].destroyed == fromDown) {
			if (isOnRoad(game->enemies[i].pos, *game, *data))
				surf = data->boomGrey;
			else
				surf = data->boomGreen;
		}
		else
			surf = data->enemy1;

		DrawSurface(data->screen, surf, game->enemies[i].pos.x, game->enemies[i].pos.y);
	}

	int players = game->twoPlayers ? 1 : 0;		//gracze
	for (int i = 0; i <= players; i++) {
		if (game->car[i].dead) {
			if (isOnRoad(game->car[i].pos, *game, *data))
				DrawSurface(data->screen, data->boomGrey, game->car[i].pos.x, game->car[i].pos.y);
			else
				DrawSurface(data->screen, data->boomGreen, game->car[i].pos.x, game->car[i].pos.y);
		}
		else {
			SDL_Surface* surf = data->car;
			if (i == 1)
				surf = data->car2;
			DrawSurface(data->screen, surf, game->car[i].pos.x, game->car[i].pos.y);
		}
	}


	for (int i = 0; i < game->actBullets; i++)		//strzaly
		DrawSurface(data->screen, data->bullet, game->bullets[i].pos.x, game->bullets[i].pos.y);

}

//obsłuż zmiany drogi
void handleRoad(gameData* game) {
	static bool roadFullRendered = 1;
	if (game->disToNewRoad <= 0) {
		roadFullRendered = 0;
		game->roadChangePosY = 1;
		double deltaWidth = random(maxRoadDelta * 2, 0, *game) - maxRoadDelta;
		if (game->roadWidth + deltaWidth > maxRoadWidth) deltaWidth = maxRoadWidth - game->roadWidth;
		else if (game->roadWidth + deltaWidth < minRoadWidth) deltaWidth = -(game->roadWidth - minRoadWidth);
		
		game->roadNewWidth = game->roadWidth + deltaWidth;
		game->disToNewRoad = roadChangeDis;
	}
	else {
		roadFullRendered = 0;
		if (game->roadChangePosY < SCREEN_HEIGHT)
			game->roadChangePosY += game->speed * game->delta;
		if(!roadFullRendered && !(game->roadChangePosY < SCREEN_HEIGHT)) {
			roadFullRendered = 1;
			game->roadChangePosY = SCREEN_HEIGHT;
			game->roadWidth = game->roadNewWidth;
		}
		game->disToNewRoad -= game->distance - game->prevDistance;
	}
}

//stworz NPC
void createNPCs(gameData* game, SDLdata *data) {
	bool chance = precChance(NPCspawnChance, 0, *game);
	if (chance && (game->actNPCs == 0 || (game->npcCars[game->actNPCs - 1].pos.y > data->carNPC->h + carEachOffset) && game->npcCars[game->actNPCs-1].pos.y < SCREEN_HEIGHT - data->carNPC->h - carEachOffset) &&
		game->actNPCs < NPCmaxCars) {
		double newX = random((game->roadNewWidth - carRoadOffset) * 100, 0, *game) / 100;
		double newRoadOff = newX - (game->roadNewWidth - carRoadOffset) / 2;
		short licznik = 0;
		for (int i = 0; i < game->actEnemies; i++) {
			if (game->enemies[i].pos.y < data->enemy1->h + carEachOffset && fabs(game->enemies[i].roadOffsetX - newRoadOff) < data->enemy1->w + carEachOffset) {
				licznik++;
				i = -1;
				double newX = random((game->roadNewWidth - carRoadOffset) * 100, licznik, *game) / 100 ;
				newRoadOff = newX - (game->roadNewWidth - carRoadOffset) / 2;
			}
			if (licznik == 4)
				return;
		}
		game->npcCars[game->actNPCs].roadOffsetX = newRoadOff;
		if (game->speed > NPCspeed) {
			game->npcCars[game->actNPCs].pos.y = -(data->carNPC->h + 1);
			game->npcCars[game->actNPCs].pos.x = SCREEN_WIDTH/2 + newRoadOff;
		}
		else {
			game->npcCars[game->actNPCs].pos.y = SCREEN_HEIGHT + data->carNPC->h + 1;
			game->npcCars[game->actNPCs].pos.x = SCREEN_WIDTH / 2 + newRoadOff;
		}
		game->actNPCs++;
	}
}

//przesun NPC w dol i sprawdz czy nie maja kolizji, jezeli nie sa na drodze to przywroc na droge, jezeli wybuchną to zatrzymaj je
void moveNPCs(gameData* game, SDLdata data) {
	for (int i = 0; i < game->actNPCs; i++) {
		if(game->npcCars[i].destroyed != explode)
			game->npcCars[i].pos.y += (game->speed - NPCspeed) * game->delta;
		else
			game->npcCars[i].pos.y += game->speed * game->delta;

		switch (game->npcCars[i].destroyed) {
		case fromLeft:
			game->npcCars[i].pos.x += pushSpeed * game->delta;
			break;
		case fromRight:
			game->npcCars[i].pos.x -= pushSpeed * game->delta;
			break;
		case notDestroyed:
			position toCheck1, toCheck2;
			toCheck1.x = game->npcCars[i].pos.x + data.carNPC->w/2;
			toCheck2.x = game->npcCars[i].pos.x - data.carNPC->w / 2;
			toCheck1.y = toCheck2.y = game->npcCars[i].pos.y;

			bool canTurn = 1;
			if (!isOnRoad(toCheck1, *game, data) || !isOnRoad(toCheck2, *game, data)) {
				if (game->npcCars[i].pos.x < SCREEN_WIDTH / 2)
					game->npcCars[i].roadOffsetX = - game->roadNewWidth / 2 + data.carNPC->w / 2 + 1;
				else
					game->npcCars[i].roadOffsetX = game->roadNewWidth / 2 - data.carNPC->w / 2 - 1;

				for (int j = 0; j < game->actEnemies; j++) {
					if (fabs(game->enemies[j].pos.y - game->npcCars[i].pos.y) < data.carNPC->h / 2 + data.enemy1->h / 2 &&
						fabs(game->enemies[j].pos.x - SCREEN_WIDTH / 2 + game->npcCars[i].roadOffsetX) < data.carNPC->w / 2 + data.enemy1->w / 2)
						canTurn = 0;
				}
			}
			
			if(canTurn)
				game->npcCars[i].pos.x = SCREEN_WIDTH / 2 + game->npcCars[i].roadOffsetX;
			break;
		}


		if (game->npcCars[i].destroyed != explode)
		for (int j = 0; j < game->actDecs; j++) {
			bool onX = fabs(game->npcCars[i].pos.x - game->decs[j].pos.x) < data.tree1->w / 2 + data.car->w / 2;
			bool onY = fabs(game->npcCars[i].pos.y - game->decs[j].pos.y) < data.tree1->h / 2 + data.tree1->h / 2;
			if (onX && onY) {
				game->decs[j].destroyed = explode;
				game->npcCars[i].destroyed = explode;
			}
		}

		if (game->npcCars[i].pos.x > SCREEN_WIDTH || game->npcCars[i].pos.x < 0)
			game->npcCars[i].destroyed = explode;
	}
}

//zniszcz NPC które są poza ekranem
void destroyNPCs(gameData* game) {
	int deleted = 0;

	for (int i = 0; i < game->actNPCs; i++) {
		if (game->npcCars[i].pos.y >= SCREEN_HEIGHT + renderDis || game->npcCars[i].pos.y < -renderDis) {
			game->npcCars[i].pos.x = -1;
			game->npcCars[i].pos.y = -1;
			game->npcCars[i].changedRoad = 0;
			game->npcCars[i].roadOffsetX = -1;
			game->npcCars[i].destroyed = notDestroyed;
			deleted++;
		}
	}

	if (deleted != 0) {		//reindekswoanie tablicy
		for (int i = 0; i < game->actNPCs; i++) {
			if (game->npcCars[i].pos.x == -1) {
				for (int j = i + 1; j < game->actNPCs; j++) {
					if (game->npcCars[j].pos.x != -1) {
						game->npcCars[i].pos.x = game->npcCars[j].pos.x;
						game->npcCars[i].pos.y = game->npcCars[j].pos.y;
						game->npcCars[i].roadOffsetX = game->npcCars[j].roadOffsetX;
						game->npcCars[i].destroyed = game->npcCars[j].destroyed;
						game->npcCars[i].changedRoad = game->npcCars[j].changedRoad;
						game->npcCars[j].pos.x = -1;
						game->npcCars[j].changedRoad = 0;
						game->npcCars[j].pos.y = -1;
						game->npcCars[j].roadOffsetX = -1;
						game->npcCars[j].destroyed = 0;
						break;
					}
				}
			}
		}
	}

	game->actNPCs -= deleted;
}

//obsłuż NPC
void handleNPCs(gameData* game, SDLdata* data) {
	createNPCs(game, data);
	moveNPCs(game, *data);
	destroyNPCs(game);
}

//obsłuż śmierć przeciwnika - dodaj punkty i ustaw wybuch przeciwnika
void enemyKilled(gameData* game, int id) {
	game->enemies[id].destroyed = explode;
	if (game->lockPts <= 0) {
		game->pts += killPoints;
		game->timers.killColorTimer = greenAfterKillTime;
	}
}

//zabij gracza
void killPlayer(gameData* game, player *car) {
	car->dead = 1;
	game->revivedProtection = 1;
}

//sprawdz czy gracz koliduje z innymi rzeczami na ekranie
void detectPlayerCollisions(gameData* game, SDLdata data) {
	int players = game->twoPlayers ? 1 : 0;
	for (int j = 0; j <= players; j++) {
		if (j == 0 && devModeRed)
			continue;
		if (j == 1 && devModeYel)
			continue;

		for (int i = 0; i < game->actNPCs; i++) {	//npc collisions
			double distanceX = fabs(game->car[j].pos.x - game->npcCars[i].pos.x);
			double distanceY = fabs(game->car[j].pos.y - game->npcCars[i].pos.y);
			double colDisX = data.carNPC->w / 2 + data.car->w / 2;
			double colDisY = data.carNPC->h / 2 + data.car->h / 2;

			bool onX = distanceX <= colDisX;
			bool onY = distanceY <= colDisY;

			if (onX && onY && game->npcCars[i].destroyed == notDestroyed) {
				double oldDisX = fabs(game->car[j].prevPos.x - game->npcCars[i].pos.x);
				if (oldDisX > colDisX) {
					if (game->car[j].pos.x > game->npcCars[i].pos.x)
						game->npcCars[i].destroyed = fromRight;
					else
						game->npcCars[i].destroyed = fromLeft;

					game->lockPts = npcKillLockTime;
				}
				else {
					game->npcCars[i].destroyed = explode;
					killPlayer(game, &game->car[j]);
				}
				break;
			}
		}


		for (int i = 0; i < game->actEnemies; i++) {	//enemy collsions
			double distanceX = fabs(game->car[j].pos.x - game->enemies[i].pos.x);
			double distanceY = fabs(game->car[j].pos.y - game->enemies[i].pos.y);
			double colDisX = data.enemy1->w / 2 + data.car->w / 2;
			double colDisY = data.enemy1->h / 2 + data.car->h / 2;

			bool onX = distanceX <= colDisX;
			bool onY = distanceY <= colDisY;

			if (onX && onY && game->enemies[i].destroyed == notDestroyed) {
				if ((game->car[j].prevPos.x < game->car[j].pos.x && game->car[j].pos.x - game->enemies[i].pos.x < 0) ||
					(game->car[j].prevPos.x > game->car[j].pos.x && game->car[j].pos.x - game->enemies[i].pos.x > 0)) {
					game->col++;
					if (game->car[j].pos.x > game->enemies[i].pos.x)
						game->enemies[i].destroyed = fromRight;
					else
						game->enemies[i].destroyed = fromLeft;
				}
				else if (fabs(game->car[j].pos.x - game->enemies[i].oldPos.x) > colDisX) {
					if (game->car[j].pos.x > game->enemies[i].pos.x)
						game->car[j].destroyed = fromLeft;
					else
						game->car[j].destroyed = fromRight;
				}
				else {
					game->enemies[i].destroyed = explode;
					killPlayer(game, &game->car[j]);
				}
				break;
			}
		}

		for (int i = 0; i < game->actDecs; i++) {	//decs collision
			bool onX = fabs(game->car[j].pos.x - game->decs[i].pos.x) < data.tree1->w / 2 + data.car->w / 2;
			bool onY = fabs(game->car[j].pos.y - game->decs[i].pos.y) < data.tree1->h / 2 + data.tree1->h / 2;
			if (onX && onY) {
				killPlayer(game, &game->car[j]);
				game->decs[i].destroyed = explode;
			}
		}

		if (game->car[j].pos.x > SCREEN_WIDTH || game->car[j].pos.x < 0)	//end of screen collision
			killPlayer(game, &game->car[j]);
	}
}

//sprawdz czy tworzony obiekt nie koliduje z przeciwnikiem
bool setSpawnByEnemies(gameData* game, SDLdata data, double *newRoadOff, short from) {
	char text[bufSize];
	short licznik = 0;
	for (int i = 0; i < game->actEnemies; i++) {
		if ((game->enemies[i].pos.y <= data.enemy1->h + carEachOffset || game->enemies[i].pos.y >= SCREEN_HEIGHT - carRoadOffset) && 
			fabs(game->enemies[i].roadOffsetX - *newRoadOff) < data.enemy1->w + carEachOffset) {
			licznik++;
			i = -1;
			double rWidth;
			if (from == mUp)
				rWidth = game->roadNewWidth;
			else
				rWidth = game->roadWidth;

			double newX = random((rWidth - 2 * carRoadOffset) * 100, licznik, *game) / 100;
			*newRoadOff = newX - (rWidth / 2) + carRoadOffset;
		}
		if (licznik == 4)
			return 0;
	}
	return 1;
}

//sprawdz czy tworzony obiekt nie koliduje z NPC
bool setSpawnByNPCs(gameData* game, SDLdata data, double* newRoadOff, short from) {
	short licznik = 0;
	for (int i = 0; i < game->actNPCs; i++) {
		if ((game->npcCars[i].pos.y <= data.carNPC->h + carEachOffset || game->npcCars[i].pos.y >= SCREEN_HEIGHT - carRoadOffset - data.carNPC->h) && 
			fabs(game->npcCars[i].roadOffsetX - *newRoadOff) < data.carNPC->w + carEachOffset) {
			licznik++;
			i = -1;
		}
		if (licznik == 4)
			return 0;
	}
	if (!setSpawnByEnemies(game, data, newRoadOff, from))
		return 0;
	return 1;
}

//ustaw pozycje x nowego samochodu (zeby nie kolidowal z innymi samochodami)
double setSpawnPos(gameData* game, SDLdata data, short from) {

	double rWidth;
	if (from == mUp)
		rWidth = game->roadNewWidth;
	else
		rWidth = game->roadWidth;

	double newX = random((rWidth - 2 * carRoadOffset) * 100, 0, *game) / 100;
	double x = newX - (rWidth / 2) + carRoadOffset;

	if (!setSpawnByEnemies(game, data, &x, from))
		return -1;
	if (!setSpawnByNPCs(game, data, &x, from))
		return -1;
return x;
}

//stworz przeciwnikow
void createEnemies(gameData* game, SDLdata data) {
	bool chance = precChance(NPCspawnChance, 5, *game);
	if (chance && game->actEnemies < maxEnemies) {

		double newRoadOff; 

		if (game->speed > enemyDefSpeed) {
			newRoadOff = setSpawnPos(game, data, mUp);
			if (newRoadOff == -1)
				return;
			game->enemies[game->actEnemies].pos.y = -(data.carNPC->h + 1);
		}
		else {
			newRoadOff = setSpawnPos(game, data, mDown);
			if (newRoadOff == -1)
				return;
			game->enemies[game->actEnemies].pos.y = SCREEN_HEIGHT + data.carNPC->h + 1;
		}
		game->enemies[game->actEnemies].pos.x = SCREEN_WIDTH / 2 + newRoadOff;
		game->enemies[game->actEnemies].roadOffsetX = newRoadOff;
		game->actEnemies++;
	}
}

//rusz przeciwnikow (atak i zwykla jazda), jezeli nie jest na drodze to powraca na droge
void moveEnemies(gameData* game, SDLdata data) {
	for (int i = 0; i < game->actEnemies; i++) {
	game->enemies[i].oldPos = game->enemies[i].pos;
		if (game->enemies[i].destroyed == notDestroyed) {
			bool canR = 1;
			bool canL = 1;
			short topDownMult = 1;
			switch (game->enemies[i].action) {
			case attack:	//atak
				int cId;

				if (fabs(game->enemies[i].pos.x - game->car[0].pos.x) > fabs(game->enemies[i].pos.x - game->car[1].pos.x) && game->twoPlayers)
					cId = 1;
				else
					cId = 0;

				for (int j = 0; j < game->actNPCs; j++) {
					bool zLewej = game->enemies[i].pos.x < game->npcCars[j].pos.x;
					if (fabs(game->enemies[i].pos.y - game->npcCars[j].pos.y) < data.carNPC->h) {
						if (game->npcCars[j].pos.x - (game->enemies[i].pos.x + enemySpeedV * game->delta) < data.carNPC->w +carEachOffset && zLewej)
							canR = 0;
						if ((game->enemies[i].pos.x - enemySpeedV * game->delta) - game->npcCars[j].pos.x < data.carNPC->w + carEachOffset && !zLewej)
							canL = 0;
					}
				}

				if (game->enemies[i].pos.x < game->car[cId].pos.x) {
					if (canR) {
						game->enemies[i].roadOffsetX += enemySpeedV * game->delta;
						game->enemies[i].pos.x += enemySpeedV * game->delta;
					}
					else {
						topDownMult = game->enemies[i].direction == mDown ? 2 : -2;
					}
				}
				else {
					if (canL) {
						game->enemies[i].roadOffsetX -= enemySpeedV * game->delta;
						game->enemies[i].pos.x -= enemySpeedV * game->delta;
					}
					else {
						topDownMult = game->enemies[i].direction == mDown ? 2 : -2;
					}
					
				}

				if (game->enemies[i].direction == mDown)
					game->enemies[i].pos.y += (game->speed - game->enemies[i].speed) * game->delta * topDownMult;
				else 
					game->enemies[i].pos.y += (game->speed - game->enemies[i].speed - enemySpeedup) * game->delta * topDownMult;
				


				if (!isOnRoad(game->enemies[i].pos, *game, data)) {
					game->enemies[i].goBackToRoad = 1;
				}
				break;
			case justRide:			//po prostu jechanie i powrot na droge
				position toCheck1, toCheck2;
				toCheck1.x = game->enemies[i].pos.x + data.carNPC->w / 2;
				toCheck2.x = game->enemies[i].pos.x - data.carNPC->w / 2;
				toCheck1.y = toCheck2.y = game->enemies[i].pos.y;
				double prevOff = game->enemies[i].roadOffsetX;
				bool canTurn = 1;

				if (game->enemies[i].goBackToRoad) {	//w ataku byl poza droga, wiec teraz wraca smooth
					double xShift = enemySpeedV * game->delta;
					if (game->enemies[i].pos.x > SCREEN_WIDTH / 2)
						xShift *= -1;
					game->enemies[i].roadOffsetX += xShift;
				}
				else if (!isOnRoad(toCheck1, *game, data) || !isOnRoad(toCheck2, *game, data)) {		//jedzie droga i zaraz ma zwezenie
					if (game->enemies[i].pos.x < SCREEN_WIDTH / 2)
						game->enemies[i].roadOffsetX = -game->roadNewWidth / 2 + data.carNPC->w / 2 + 1;
					else
						game->enemies[i].roadOffsetX = game->roadNewWidth / 2 - data.carNPC->w / 2 - 1;
				}
				
				if (isOnRoad(game->enemies[i].pos, *game, data)) {	//jest na drodze - ustawia zeby przestal wracac
					game->enemies[i].goBackToRoad = 0;
				}

				for (int j = 0; j < game->actEnemies; j++) {	//sprawdza czy moze wjechac bo oponenci
					if(j != i)
						if (fabs(game->enemies[j].pos.y - game->enemies[i].pos.y) < data.enemy1->h &&
							fabs(game->enemies[j].pos.x - SCREEN_WIDTH / 2 + game->enemies[i].roadOffsetX) < data.enemy1->w) {
							canTurn = 0;
					}
				}

				for (int j = 0; j < game->actNPCs; j++) {	//sprawdza czy moze wjechac bo oponenci
					if (fabs(game->npcCars[j].pos.y - game->enemies[i].pos.y) < data.enemy1->h / 2 + data.carNPC->h / 2 &&
						fabs(game->npcCars[j].pos.x - SCREEN_WIDTH / 2 - game->enemies[i].roadOffsetX) < data.carNPC->w / 2 + data.enemy1->w) {
						canTurn = 0;
					}
				}
				if (canTurn)
					game->enemies[i].pos.x = SCREEN_WIDTH / 2 + game->enemies[i].roadOffsetX;
				else
					game->enemies[i].roadOffsetX = prevOff;

				game->enemies[i].pos.y += (game->speed - game->enemies[i].speed) * game->delta;
				break;
			}
		}
			
		switch (game->enemies[i].destroyed) {		//zniszony
		case fromLeft:
			game->enemies[i].pos.x += pushSpeed * game->delta;
			game->enemies[i].pos.y += (game->speed - game->enemies[i].speed) * game->delta;
			break;
		case fromRight:
			game->enemies[i].pos.x -= pushSpeed * game->delta;
			game->enemies[i].pos.y += (game->speed - game->enemies[i].speed) * game->delta;
			break;
		case explode:
			game->enemies[i].pos.y += game->speed * game->delta;
			break;
		}

		if (game->enemies[i].destroyed != explode)		//jak wjedzie w drzewo
			for (int j = 0; j < game->actDecs; j++) {
				bool onX = fabs(game->enemies[i].pos.x - game->decs[j].pos.x) < data.tree1->w / 2 + data.car->w / 2;
				bool onY = fabs(game->enemies[i].pos.y - game->decs[j].pos.y) < data.tree1->h / 2 + data.tree1->h / 2;
				if (onX && onY) {
					game->decs[j].destroyed = explode;
					enemyKilled(game, i);
				}
			}

		if (game->enemies[i].pos.x > SCREEN_WIDTH || game->enemies[i].pos.x < 0)
			enemyKilled(game, i);

	}
}

//zniszcz przeciwnikow
void destroyEnemies(gameData* game) {
	int deleted = 0;

	for (int i = 0; i < game->actEnemies; i++) {
		if (game->enemies[i].pos.y >= SCREEN_HEIGHT + renderDis || game->enemies[i].pos.y < -renderDis) {
			game->enemies[i].pos.x = -1;
			game->enemies[i].pos.y = -1;
			game->enemies[i].oldPos.x = -1;
			game->enemies[i].oldPos.y = -1;
			game->enemies[i].roadOffsetX = -1;
			game->enemies[i].changedRoad = 0;
			game->enemies[i].goBackToRoad = 0;
			game->enemies[i].action = justRide;
			game->enemies[i].destroyed = notDestroyed;
			game->enemies[i].speed = enemyDefSpeed;
			deleted++;
		}
	}

	if (deleted != 0) {		//reindekswoanie tablicy
		for (int i = 0; i < game->actEnemies; i++) {
			if (game->enemies[i].pos.x == -1) {
				for (int j = i + 1; j < game->actEnemies; j++) {
					if (game->enemies[j].pos.x != -1) {
						game->enemies[i].pos.x = game->enemies[j].pos.x;
						game->enemies[i].pos.y = game->enemies[j].pos.y;
						game->enemies[i].roadOffsetX = game->enemies[j].roadOffsetX;
						game->enemies[i].destroyed = game->enemies[j].destroyed;
						game->enemies[i].action = game->enemies[j].action;
						game->enemies[i].oldPos = game->enemies[j].oldPos;
						game->enemies[i].changedRoad = game->enemies[j].changedRoad;
						game->enemies[i].goBackToRoad = game->enemies[j].goBackToRoad;
						game->enemies[i].speed = game->enemies[j].speed;
						game->enemies[j].pos.x = -1;
						game->enemies[j].goBackToRoad = 0;
						game->enemies[j].changedRoad = 0;
						game->enemies[j].pos.y = -1;
						game->enemies[j].oldPos.x = -1;
						game->enemies[j].oldPos.y = -1;
						game->enemies[j].roadOffsetX = -1;
						game->enemies[j].destroyed = notDestroyed;
						game->enemies[j].action = justRide;
						game->enemies[j].speed = enemyDefSpeed;
						break;
					}
				}
			}
		}
	}

	game->actEnemies -= deleted;
}

//ustaw akcję przeciwnika w zależności od odległości do gracza
void setEnemiesAction(gameData* game) {

	for (int i = 0; i < game->actEnemies; i++) {

		if (fabs(game->car[0].pos.y - game->enemies[i].pos.y) > enemyAttTrigDis && game->enemies[i].action || game->revivedProtection)
			game->enemies[i].action = justRide;
		else if (game->enemies[i].action != attack) {
			game->enemies[i].action = attack;

			if (game->enemies[i].pos.y < game->car[0].pos.y)
				game->enemies[i].direction = mDown;
			else
				game->enemies[i].direction = mUp;
			break;
		}
	}
}

//obsłuż przeciwnika
void handleEnemies(gameData *game, SDLdata data) {
	createEnemies(game, data);
	setEnemiesAction(game);
	moveEnemies(game, data);
	destroyEnemies(game);
}

//obsłuż mechanizm nowych samochodów
void handleRevive(gameData* game, SDLdata data) {
	if (game->speed > 0) {
		game->speed -= deathSlowdown * game->delta;
	}
	else {
		int players = game->twoPlayers ? 1 : 0;
		for (int i = 0; i <= players; i++) {
			game->speed = 0;
			game->car[i].destroyed = notDestroyed;
			game->car[i].pos.y = startY;
			double rWidth;
			if (game->car[i].pos.y > game->roadChangePosY)
				rWidth = game->roadWidth;
			else
				rWidth = game->roadNewWidth;

			game->car[i].pos.x = SCREEN_WIDTH / 2 + (rWidth / 2 + data.car->w / 2 + 1) * (i==0?1:-1);

			game->revivedProtection = 1;
			game->car[i].prevPos = game->car[i].pos;
			if (game->worldTime > infRevivesTime && game->car[i].dead)
				game->revives--;

			if(game->revives != -1)
				game->car[i].dead = false;
		}
	}

}

//stwórz pociski na pozycji strzelającego samochodu
void createBullets(gameData* game, SDLdata data, player *car) {
	if (car->bulletOffTimer <= 0) {
		bullet* temp = game->bullets;
		game->bullets = new bullet[game->actBullets+1];

		for (int i = 0; i < game->actBullets; i++)
			game->bullets[i] = temp[i];

		if (car->lastBulletOnLeft) {
			car->lastBulletOnLeft = 0;
			game->bullets[game->actBullets].pos.x = car->pos.x - data.car->w / 4;
		}
		else {
			car->lastBulletOnLeft = 1;
			game->bullets[game->actBullets].pos.x = car->pos.x + data.car->w / 4;
		}
		game->bullets[game->actBullets].pos.y = car->pos.y - data.car->h / 2 - data.bullet->h/2;
		game->bullets[game->actBullets].range = car->shootingDis;

		if (car->ammoFasterLeft > 0)
			car->ammoFasterLeft--;
		if (car->ammoLongerLeft > 0)
			car->ammoLongerLeft--;

		delete[] temp;
		game->actBullets++;

		car->bulletOffTimer = 1.0/car->shootingRate;
	}
}

//porusz pociskami
void moveBullets(gameData *game) {
	for (int i = 0; i < game->actBullets; i++) {
		game->bullets[i].pos.y -= bulletSpeed * game->delta;
		game->bullets[i].doneDistance += bulletSpeed * game->delta;
	}
}

//zniszcz naboje które przeleciały maxDistance
void destroyBullets(gameData* game) {
	if (game->actBullets == 0)
		return;

	bullet* temp = game->bullets;
	int deleted = 0;
	for (int i = 0; i < game->actBullets; i++) {
		if (game->bullets[i].pos.y <= game->car[0].pos.y - game->bullets[i].range) {
			temp[i].pos.x = -1;
			deleted++;
		}
	}

	game->bullets = new bullet[game->actBullets - deleted];
	int id = 0;

	for (int i = 0; i < game->actBullets; i++) {
		if (temp[i].pos.x != -1) {
			game->bullets[id] = temp[i];
			id++;
		}
	}

	game->actBullets -= deleted;
	delete[] temp;
}

//sprawdź czy naboje trafiły w cel
void bulletsCheckHit(gameData* game, SDLdata data) {
	for (int i = 0; i < game->actBullets; i++) {
		for (int j = 0; j < game->actEnemies; j++) {
			if (fabs(game->bullets[i].pos.y - game->enemies[j].pos.y) <= data.enemy1->h / 2 + data.bullet->h / 2 &&
				fabs(game->enemies[j].pos.x - game->bullets[i].pos.x) <= data.enemy1->w / 2 + data.bullet->w / 2 &&
				game->enemies[i].destroyed != explode)
			{
				game->col++;
				enemyKilled(game, j);
				game->bullets[i].pos.y = -1;
				game->bullets[i].pos.x = -1;
			}
		}

		for (int j = 0; j < game->actNPCs; j++) {
			if (fabs(game->bullets[i].pos.y - game->npcCars[j].pos.y) <= data.carNPC->h / 2 + data.bullet->h / 2 &&
				fabs(game->npcCars[j].pos.x - game->bullets[i].pos.x) <= data.carNPC->w / 2 + data.bullet->w / 2 &&
				game->npcCars[i].destroyed != explode)
			{
				game->npcCars[j].destroyed = explode;
				game->lockPts = npcKillLockTime;
				game->bullets[i].pos.y = -1;
				game->bullets[i].pos.x = -1;
			}
		}
	}
}

//obsłuż mechanikę strzelania
void shooting(gameData *game, SDLdata data) {
	if (game->car[0].shooting) {
		createBullets(game, data, &game->car[0]);
	}
	if (game->car[1].shooting) {
		createBullets(game, data, &game->car[1]);
	}
	moveBullets(game);
	game->car[0].bulletOffTimer -= game->delta;
	game->car[1].bulletOffTimer -= game->delta;
	bulletsCheckHit(game, data);
	destroyBullets(game);
}

//obsłuż część mechanik gracza (poza sterowaniem i kolizjami)
void handlePlayer(gameData* game, SDLdata data) {
	movePlayer(game);
	shooting(game, data);
}

//zaaplikuj pickup
void applyEffect(gameData* game, short effect, player* car) {
	if (effect == addCar)
		game->revives++;
	else if (effect == ammoLonger) {
		car->ammoLongerLeft += ammoLongerAmount;
		car->shootingDis = longShootingDis;
	}
	else if (effect == ammoFaster) {
		car->ammoFasterLeft += ammoFasterAmount;
		car->shootingRate = fasterShootingRate;
	}
}

//sprawdz czy jest jeszcze amunicja
void handleEffects(gameData* game) {
	for (int i = 0; i < 2; i++) {
		if (game->car[i].ammoLongerLeft <= 0)
			game->car[i].shootingDis = basicShootingDis;
		if (game->car[i].ammoFasterLeft <= 0)
			game->car[i].shootingRate = basicShootingRate;
	}
}

//stworz pickupy
void createPickups(gameData* game, SDLdata data) {
	bool chance = precChance(pickupSpawnChance,0,*game);
	if(chance)
	if (chance && (game->actPickups == 0 || game->pickups[game->actPickups].pos.y > data.moreDis->h) && game->actPickups < maxPickups) {
		game->pickups[game->actPickups].mode = random(3, 0, *game) + 1;
		game->pickups[game->actPickups].pos.y = -data.newCar->h;
		game->pickups[game->actPickups].speed = maxPickupSpeed;
		double newRoadOff = random((game->roadNewWidth - carRoadOffset) * 100, 0, *game) / 100 - (game->roadNewWidth - carRoadOffset) / 2;
		game->pickups[game->actPickups].roadOffsetX = newRoadOff;
		game->actPickups++;
	}
}

//przesun pickupy w dol
void movePickups(gameData *game) {
	for (int i = 0; i < game->actPickups; i++) {
		game->pickups[i].speed = game->speed;
		if (game->pickups[i].speed > maxPickupSpeed)
			game->pickups[i].speed = maxPickupSpeed;

		game->pickups[i].pos.y += game->delta * game->pickups[i].speed;
		game->pickups[i].pos.x = SCREEN_WIDTH/2 + game->pickups[i].roadOffsetX;
	}
}

//zniszcz pickupy poza mapą
void destroyPickups(gameData *game) {
	int deleted = 0;
	for (int i = 0; i < game->actPickups; i++) {
		if (game->pickups[i].pos.y >= SCREEN_HEIGHT + 100) {
			game->pickups[i].pos.x = -1;
			game->pickups[i].pos.y = -1;
			game->pickups[i].speed = -1;
			game->pickups[i].roadOffsetX = -1;
			game->pickups[i].mode = -1;
			deleted++;
		}
	}

	if (deleted != 0) {		//reindekswoanie tablicy
		for (int i = 0; i < game->actPickups; i++) {
			if (game->pickups[i].pos.x == -1) {
				for (int j = i + 1; j < game->actPickups; j++) {
					if (game->pickups[j].pos.x != -1) {
						game->pickups[i].pos.x = game->pickups[j].pos.x;
						game->pickups[i].pos.y = game->pickups[j].pos.y;
						game->pickups[i].speed = game->pickups[j].speed;
						game->pickups[i].roadOffsetX = game->pickups[j].roadOffsetX;
						game->pickups[i].mode = game->pickups[j].mode;
						game->pickups[j].pos.x = -1;
						game->pickups[j].pos.y = -1;
						game->pickups[j].speed = -1;
						game->pickups[j].roadOffsetX = -1;
						game->pickups[j].mode = -1;
						break;
					}
				}
			}
		}
	}

	game->actPickups -= deleted;
}

//sprawdz czy samochod najechal na pickup, jezeli tak to dodaj do tego samochodu efekt
void pickupsCheckIfPicked(gameData* game, SDLdata data) {
	short players = game->twoPlayers ? 1 : 0;
	for (int i = 0; i < game->actPickups; i++) {
		for (int j = 0; j <= players; j++) {
			if (fabs(game->pickups[i].pos.x - game->car[j].pos.x) < data.moreDis->w / 2 + data.car->w / 2 &&
				fabs(game->pickups[i].pos.y - game->car[j].pos.y) < data.moreDis->h / 2 + data.car->h / 2) {
				applyEffect(game, game->pickups[i].mode, &game->car[j]);
				game->pickups[i].pos.y = SCREEN_HEIGHT + 1000;
			}
		}
	}
}

//obsłuż pickupy
void handlePickups(gameData* game, SDLdata data) {
	createPickups(game, data);
	movePickups(game);
	pickupsCheckIfPicked(game, data);
	destroyPickups(game);
}

//obsłuż dodawanie nowych samochodów co ptsReqToRev(stała zadeklarowana przed main()) punktów
void updateReviveTimer(gameData* game) {
	if (game->pts > game->lastRevive + ptsReqToRev) {
		game->lastRevive = game->pts;
		game->revives++;
	}
}

//odśwież globalne timery, fps, dystans, punkty etc
void updateGameData(gameData* game, SDLdata* data) {
	updateReviveTimer(game);
	updateTimer(game);
	updateFPS(game);
	updateDistance(game);
	handlePtsLock(game, data);
	updatePoints(game, *data);
}

//obsłuż obiekty poruszane przez program
void handleMovableObjects(gameData* game, SDLdata* data) {
	handleDecs(game, data);
	handleRoad(game);
	handleNPCs(game, data);
	handleEnemies(game, *data);
	handlePickups(game, *data);
}

//przygotuj dane do wyrenderowania
void prepareDataToRender(gameData* game, SDLdata* data) {
	SDL_FillRect(data->screen, NULL, data->zielony);
	drawSprites(game, data);
	drawTopbar(game, data);
	drawDownbar(data);
}

//odśwież ekran, wyrenderuj dane
void renderData(SDLdata* data) {
	SDL_UpdateTexture(data->scrtex, NULL, data->screen->pixels, data->screen->pitch);
	SDL_RenderCopy(data->renderer, data->scrtex, NULL, NULL);
	SDL_RenderPresent(data->renderer);
}

//zapisz dane do pliku
void savingMechanism(FILE* plik, gameData game) {
	time_t t = time(NULL);
	tm* locTime = localtime(&t);
	
	char text[bufSize];
	sprintf_s(text, "name:%d.%d.%d - %d:%d:%d\n", locTime->tm_year + 1900, locTime->tm_mon + 1, locTime->tm_mday, locTime->tm_hour, locTime->tm_min, locTime->tm_sec);
	fprintf(plik, text);
	for (int i = 0; i < 2; i++) {
		fprintf(plik, "cPsX:%lf\n", game.car[i].pos.x);
		fprintf(plik, "cPsY:%lf\n", game.car[i].pos.y);
		fprintf(plik, "pPsX:%lf\n", game.car[i].prevPos.x);
		fprintf(plik, "pPsY:%lf\n", game.car[i].prevPos.y);
		fprintf(plik, "dstr:%d\n", game.car[i].destroyed);
		fprintf(plik, "shDs:%d\n", game.car[i].shootingDis);
		fprintf(plik, "shRt:%d\n", game.car[i].shootingRate);
		fprintf(plik, "amFL:%d\n", game.car[i].ammoFasterLeft);
		fprintf(plik, "amLL:%d\n", game.car[i].ammoLongerLeft);
		fprintf(plik, "dead:%d\n", game.car[i].dead);
	}

	fprintf(plik, "rPrt:%d\n", game.revivedProtection);
	fprintf(plik, "twPl:%d\n", game.twoPlayers);
	fprintf(plik, "wdTm:%lf\n", game.worldTime);
	fprintf(plik, "sped:%lf\n", game.speed);
	fprintf(plik, "dist:%lf\n", game.distance);
	fprintf(plik, "pnts:%lf\n", game.pts);
	fprintf(plik, "prvD:%lf\n", game.prevDistance);
	fprintf(plik, "lPts:%lf\n", game.lockPts);
	fprintf(plik, "lRev:%lf\n", game.lastRevive);
	fprintf(plik, "kilT:%lf\n", game.timers.killColorTimer);
	fprintf(plik, "aDec:%d\n", game.actDecs);
	fprintf(plik, "aNPC:%d\n", game.actNPCs);
	fprintf(plik, "aEne:%d\n", game.actEnemies);
	fprintf(plik, "aPck:%d\n", game.actPickups);
	fprintf(plik, "rdCY:%d\n", game.roadChangePosY);
	fprintf(plik, "rdWd:%d\n", game.roadWidth);
	fprintf(plik, "rdNW:%d\n", game.roadNewWidth);
	fprintf(plik, "nRdD:%lf\n", game.disToNewRoad);
	fprintf(plik, "cols:%d\n", game.col);

	fprintf(plik, "decs:\n");
	for (int i = 0; i < game.actDecs; i++) {
		fprintf(plik, "PsX%d:%lf\n", i, game.decs[i].pos.x);
		fprintf(plik, "PxY%d:%lf\n", i, game.decs[i].pos.y);
		fprintf(plik, "Des%d:%d\n", i, game.decs[i].destroyed);
		fprintf(plik, "Typ%d:%d\n", i, game.decs[i].type);
	}

	fprintf(plik, "NPCs:\n");
	for (int i = 0; i < game.actNPCs; i++) {
		fprintf(plik, "PsX%d:%lf\n", i, game.npcCars[i].pos.x);
		fprintf(plik, "PxY%d:%lf\n", i, game.npcCars[i].pos.y);
		fprintf(plik, "OfX%d:%lf\n", i, game.npcCars[i].roadOffsetX);
		fprintf(plik, "Des%d:%d\n", i, game.npcCars[i].destroyed);
		fprintf(plik, "Chn%d:%d\n", i, game.npcCars[i].changedRoad);
	}

	fprintf(plik, "enms:\n");
	for (int i = 0; i < game.actEnemies; i++) {
		fprintf(plik, "PsX%d:%lf\n", i, game.enemies[i].pos.x);
		fprintf(plik, "PxY%d:%lf\n", i, game.enemies[i].pos.y);
		fprintf(plik, "OPX%d:%lf\n", i, game.enemies[i].oldPos.x);
		fprintf(plik, "OPY%d:%lf\n", i, game.enemies[i].oldPos.y);
		fprintf(plik, "OfX%d:%lf\n", i, game.enemies[i].roadOffsetX);
		fprintf(plik, "Spe%d:%lf\n", i, game.enemies[i].speed);
		fprintf(plik, "Dir%d:%d\n", i, game.enemies[i].direction);
		fprintf(plik, "Des%d:%d\n", i, game.enemies[i].destroyed);
		fprintf(plik, "Act%d:%d\n", i, game.enemies[i].action);
		fprintf(plik, "Act%d:%d\n", i, game.enemies[i].changedRoad);
		fprintf(plik, "Act%d:%d\n", i, game.enemies[i].goBackToRoad);
	}

	fprintf(plik, "pckp:\n");
	for (int i = 0; i < game.actPickups; i++) {
		fprintf(plik, "PsX%d:%lf\n", i, game.pickups[i].pos.x);
		fprintf(plik, "PxY%d:%lf\n", i, game.pickups[i].pos.y);
		fprintf(plik, "OfX%d:%lf\n", i, game.pickups[i].roadOffsetX);
		fprintf(plik, "mde%d:%d\n", i, game.pickups[i].mode);
		fprintf(plik, "mde%d:%lf\n", i, game.pickups[i].speed);
	}
}

//posortuj scoreboard wg argumenty by (wrldTime lub pnts)
void sortScoreBoard(double** t, int size, short by)
{
	short id;
	if (by == pnts)	id = 0;
	else id = 1;
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size - i - 1; j++)
			if (t[j][id] < t[j + 1][id]) {
				double temp = t[j][0];
				t[j][0] = t[j + 1][0];
				t[j + 1][0] = temp;
				temp = t[j][1];
				t[j][1] = t[j + 1][1];
				t[j + 1][1] = temp;
			}
	}
		
}

//obsłuż zapis gry
void saveGame(gameData* game) {
	destroyBullets(game);
	destroyDecs(game);
	destroyEnemies(game);
	destroyNPCs(game);
	destroyPickups(game);
	FILE* plik;
	plik = fopen("saves", "a");
	if (plik == NULL) {
		printf("Can't open file\n");
		return;
	}
	savingMechanism(plik, *game);
	fclose(plik);
	printf("Saved successfully\n");
}

//dodaj wynik do pliku z najlepszymi wynikami
void addHighScore(double pts, double time, bool *canAdd) {
	*canAdd = 0;
	FILE* plik;
	plik = fopen("scores", "a");
	if (plik == NULL) {
		printf("Can't open file\n");
		return;
	}
	char text[bufSize];
	sprintf_s(text, "name:none\npnts:%lf\ntime:%lf\n", pts, time);
	fprintf(plik, text);
	fclose(plik);
}

//zwraca rekord z pliku
double readSavedRecord(FILE* plik) {
	char c;
	double result;
	do{	//przejscie przez tytul rekordu
		c = getc(plik);
	} while (c != ':' && c!= EOF);

	fscanf(plik, "%lf", &result);
	c = getc(plik);		//spalenie dodatkowego znaku za entera
	return result;
}

//wczytaj dane z pliku do zmiennej
void loadMechanism(FILE* plik, gameData* game) {
	newGame(game);
	double d = 0;
	int a = 0;
	for (int i = 0; i < 2; i++) {
		game->car[i].pos.x = readSavedRecord(plik);
		game->car[i].pos.y = readSavedRecord(plik);
		game->car[i].prevPos.x = readSavedRecord(plik);
		game->car[i].prevPos.y = readSavedRecord(plik);
		game->car[i].destroyed = readSavedRecord(plik);
		game->car[i].shootingDis = readSavedRecord(plik);
		game->car[i].shootingRate = readSavedRecord(plik);
		game->car[i].ammoFasterLeft = readSavedRecord(plik);
		game->car[i].ammoLongerLeft = readSavedRecord(plik);
		game->car[i].dead = readSavedRecord(plik);
	}
	game->revivedProtection = readSavedRecord(plik);
	game->twoPlayers = readSavedRecord(plik);
	game->worldTime = readSavedRecord(plik);
	game->speed = readSavedRecord(plik);
	game->distance = readSavedRecord(plik);
	game->pts = readSavedRecord(plik);
	game->prevDistance = readSavedRecord(plik);
	game->lockPts = readSavedRecord(plik);
	game->lastRevive = readSavedRecord(plik);
	game->timers.killColorTimer = readSavedRecord(plik);
	game->actDecs = readSavedRecord(plik);
	game->actNPCs = readSavedRecord(plik);
	game->actEnemies = readSavedRecord(plik);
	game->actPickups = readSavedRecord(plik);
	game->roadChangePosY = readSavedRecord(plik);
	game->roadWidth = readSavedRecord(plik);
	game->roadNewWidth = readSavedRecord(plik);
	game->disToNewRoad = readSavedRecord(plik);
	game->col = readSavedRecord(plik);

	readSavedRecord(plik);
	for (int i = 0; i < game->actDecs; i++) {
		game->decs[i].pos.x = readSavedRecord(plik);
		game->decs[i].pos.y = readSavedRecord(plik);
		game->decs[i].destroyed = readSavedRecord(plik);
		game->decs[i].type = readSavedRecord(plik);
	}

	readSavedRecord(plik);
	for (int i = 0; i < game->actNPCs; i++) {
		game->npcCars[i].pos.x = readSavedRecord(plik);
		game->npcCars[i].pos.y = readSavedRecord(plik);
		game->npcCars[i].roadOffsetX = readSavedRecord(plik);
		game->npcCars[i].destroyed = readSavedRecord(plik);
		game->npcCars[i].changedRoad = readSavedRecord(plik);
	}

	readSavedRecord(plik);
	for (int i = 0; i < game->actEnemies; i++) {
		game->enemies[i].pos.x = readSavedRecord(plik);
		game->enemies[i].pos.y = readSavedRecord(plik);
		game->enemies[i].oldPos.x = readSavedRecord(plik);
		game->enemies[i].oldPos.y = readSavedRecord(plik);
		game->enemies[i].roadOffsetX = readSavedRecord(plik);
		game->enemies[i].speed = readSavedRecord(plik);
		game->enemies[i].direction = readSavedRecord(plik);
		game->enemies[i].destroyed = readSavedRecord(plik);
		game->enemies[i].action = readSavedRecord(plik);
		game->enemies[i].changedRoad = readSavedRecord(plik);
		game->enemies[i].goBackToRoad = readSavedRecord(plik);
	}

	readSavedRecord(plik);
	for (int i = 0; i < game->actPickups; i++) {
		game->pickups[i].pos.x = readSavedRecord(plik);
		game->pickups[i].pos.y = readSavedRecord(plik);
		game->pickups[i].roadOffsetX = readSavedRecord(plik);
		game->pickups[i].mode = readSavedRecord(plik);
		game->pickups[i].speed = readSavedRecord(plik);
	}
}  
 
//mechanizm wczytywania gry
void loadGame(gameData* game, char* name) {
	FILE* plik;
	char text[bufSize] = "";
	plik = fopen("saves", "r");
	if (plik == NULL) {
		printf("can't open file\n");
		return;
	}
	char c = getc(plik);
	short charPos = 0;
	while (c != EOF) {
		if (c != '\n') {
			text[charPos] = c;
			charPos++;
		}
		c = getc(plik);

		if (c == '\n') {
			if (strstr(text, "name:") == text) {
				if (strstr(text, name) != NULL)
					break;
			}
			charPos = 0;
			strcpy(text, "");
		}
	}
	loadMechanism(plik, game);
	fclose(plik);
}

//przygotuj tablicę do wyswietlenia w menu
void initGameMenu(gameData *game, SDLdata *data, short type) {
	char text[bufSize];
	game->menuPage = 0;
	game->pickedMenuPos = 1;
	game->actMenuPos = 0;
	game->menuMaxOnPage = -1;
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

	if (type == chPlayers) {
		game->actMenuPos = 2;
		game->menuMaxOnPage = 2;
		game->menuPos = new char* [2];
		game->menuPos[0] = new char[bufSize];
		game->menuPos[1] = new char[bufSize];
		strcpy(game->menuPos[0], "jeden gracz");
		strcpy(game->menuPos[1], "dwoch graczy");
		renderData(data);
		return;
	}

	FILE* plik = NULL;
	if (type == load)
		plik = fopen("saves", "r");
	else //scores
		plik = fopen("scores", "r");

	if (plik == NULL) {
		printf("can't open file");
		return;
	}

	int j;
	int counter=0;
	sprintf_s(text, "");


	do {		//petla wrzucajaca do tablic rzeczy do wsyswietlenia w menu
		j = 0;
		do {
			text[j++] = fgetc(plik);
		} while (text[j - 1] != '\n' && text[j - 1] != EOF);

		if (text[j - 1] == EOF)
			break;

		text[j - 1] = '\0';
		char* needle = strstr(text, "name");

		if (needle == text) {
			if (type == load) {
				game->actMenuPos++;
				char** temp = game->menuPos;

				game->menuPos = new char* [game->actMenuPos];
				for (int i = 0; i < game->actMenuPos; i++)
					game->menuPos[i] = new char[bufSize];

				for (int i = 0; i < game->actMenuPos - 1; i++)
					strcpy(game->menuPos[i], temp[i]);

				needle = strstr(needle, ":") + 1;
				strcpy(game->menuPos[game->actMenuPos - 1], needle);

				for (int i = 0; i < game->actMenuPos - 2; i++)
					delete temp[i];
				delete[] temp;

				if (SCREEN_HEIGHT < menuTitleOffY + (letterSize + versesOffset) * (4 + savesTitleOffY + counter++) && game->menuMaxOnPage == -1)
					game->menuMaxOnPage = counter;
			}
			else {
				game->actMenuPos++;
				double** temp = game->scoreBoard;

				game->scoreBoard = new double* [game->actMenuPos];
				for (int i = 0; i < game->actMenuPos; i++) {
					game->scoreBoard[i] = new double[2];
				}

				for (int i = 0; i < game->actMenuPos - 1; i++) {
					game->scoreBoard[i][0] = temp[i][0];
					game->scoreBoard[i][1] = temp[i][1];
				}

				game->scoreBoard[game->actMenuPos - 1][0] = readSavedRecord(plik);
				game->scoreBoard[game->actMenuPos - 1][1] = readSavedRecord(plik);

				for (int i = 0; i < game->actMenuPos - 2; i++)
					delete temp[i];
				delete[] temp;

				if (SCREEN_HEIGHT < menuTitleOffY + (letterSize + versesOffset) * (5 + savesTitleOffY + counter++) && game->menuMaxOnPage == -1)
					game->menuMaxOnPage = counter;
			}
		}

		sprintf_s(text, "");
	} while (1);

	if (game->menuMaxOnPage == -1)
		game->menuMaxOnPage = counter + 1;
	if (type == scores)
		sortScoreBoard(game->scoreBoard, game->actMenuPos, pnts);

	fclose(plik);
	renderData(data);
}

//wyrysuj widok menu
void updateMenuView(gameData game, SDLdata* data, short type) {
	SDL_FillRect(data->screen, NULL, data->czarny);
	char text[bufSize];
	const int nextVerse = letterSize + versesOffset;

	if (type == load)	sprintf_s(text, "Wczytaj gre");
	else if(type == scores)	sprintf_s(text, "Najlepsze wyniki");
	else sprintf_s(text, "Wybierz ilosc graczy");
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, menuTitleOffY, text, data->charset);

	if (type == load)	sprintf_s(text, "L - wroc do gry    esc - wyjdz z gry");
	else if(type == scores)	sprintf_s(text, "N - nowa gra    esc - wyjdz z gry");
	else sprintf_s(text, "strzalki - nawiguj     enter - wybierz");
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, menuTitleOffY + nextVerse * navPosY, text, data->charset);

	if (type == load)	sprintf_s(text, "strzalki - nawiguj     enter - wczytaj");
	else if (type == scores) sprintf_s(text, "strzalki - nawiguj     enter - dodaj swoj wynik");
	else sprintf_s(text, "");
	DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, menuTitleOffY + nextVerse * (navPosY + 1), text, data->charset);

	if (type == scores)	{
		sprintf_s(text, "sortuj wg: p - punktow     t - czasu"); 
		DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, menuTitleOffY + nextVerse * (navPosY + 2), text, data->charset);
	}

	for (int i = game.menuMaxOnPage * game.menuPage; i < game.menuMaxOnPage * (game.menuPage+1); i++) {
		if (i == game.actMenuPos)
			break;
		
		if (type == scores) {
			SDL_Surface* textColor = data->charset;
			if (i + 1 == game.pickedMenuPos)
				textColor = data->charsetRed;
			sprintf_s(text, "Punkty: %lf   Czas: %lf", game.scoreBoard[i][0], game.scoreBoard[i][1]);	
			DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, menuTitleOffY + nextVerse * (4 + savesTitleOffY + i % game.menuMaxOnPage), text, textColor);
		} 
		else {
			SDL_Surface* textColor = data->charset;
			if (i + 1 == game.pickedMenuPos)
				textColor = data->charsetGreen;

			strcpy(text, game.menuPos[i]);
			DrawString(data->screen, (data->screen->w - strlen(text) * letterSize) / 2, menuTitleOffY + nextVerse * (3 + savesTitleOffY + i % game.menuMaxOnPage), text, textColor);
		}
	}
}

//obsłuż input w menu, wyrysuj dane
void gameMenu(gameData* game, SDLdata* data, short type) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			//all types
			if (event.key.keysym.sym == SDLK_ESCAPE) game->quit = 1;
			if (event.key.keysym.sym == SDLK_UP) { 
				if (game->pickedMenuPos != 1) game->pickedMenuPos--; 
				if (game->pickedMenuPos % game->menuMaxOnPage == 0) game->menuPage--;
			}
			if (event.key.keysym.sym == SDLK_DOWN) { 
				if (game->actMenuPos != game->pickedMenuPos) {
					game->pickedMenuPos++;
					if (game->pickedMenuPos % game->menuMaxOnPage == 1)
						game->menuPage++;
				}
			}
			if (type == load) {		//load game menu
				if (event.key.keysym.sym == SDLK_l) {
					if (game->menuPos != NULL) {
						for (int i = 0; i < game->actMenuPos; i++)
							delete[] game->menuPos[i];
						delete[] game->menuPos;
						game->menuPos = NULL;
					}
					game->loadMenu = 0;
					game->actMenuPos = 0;
					return;
				}
				if (event.key.keysym.sym == SDLK_RETURN) {
					if (game->pickedMenuPos >= 1 && game->pickedMenuPos <= game->actMenuPos) {
						loadGame(game, game->menuPos[game->pickedMenuPos - 1]);
						return;
					}
				}
			}
			else if (type == scores) {					//high score menu
				if (event.key.keysym.sym == SDLK_n) {
					newGame(game);
					initGameMenu(game, data, chPlayers);
					game->startMenu = 1;
					return;
				}
				if (event.key.keysym.sym == SDLK_RETURN) {
					if (game->canAddScore) {
						addHighScore(game->pts, game->worldTime, &(game->canAddScore));
						initGameMenu(game, data, scores);
						return;
					}
				}
				if (event.key.keysym.sym == SDLK_p) sortScoreBoard(game->scoreBoard, game->actMenuPos, pnts);
				if (event.key.keysym.sym == SDLK_t) sortScoreBoard(game->scoreBoard, game->actMenuPos, wrldTime);
			}
			else {		//choose game mode (1 or 2 players)
				if (event.key.keysym.sym == SDLK_RETURN) {
					if (game->pickedMenuPos == 2)
						game->twoPlayers = 1;
					else
						game->twoPlayers = 0;
					newGame(game);
					return;
				}
			}
			break;
		};
	};
	updateMenuView(*game, data, type);
	renderData(data);
}

//stworz nowa gre, wyzeruj wszystkie tablice, ustaw podstawowe wartosci zmiennych
void newGame(gameData* game) {
	for (int i = 0; i < game->actDecs; i++) {
		game->decs[i].pos.y = SCREEN_HEIGHT + 1000;
	}
	destroyDecs(game);

	for (int i = 0; i < game->actNPCs; i++) {
		game->npcCars[i].pos.y = SCREEN_HEIGHT + renderDis + 1000;
	}
	destroyNPCs(game);

	for (int i = 0; i < game->actEnemies; i++) {
		game->enemies[i].pos.y = SCREEN_HEIGHT + renderDis + 1000;
	}
	destroyEnemies(game);

	for (int i = 0; i < game->actBullets; i++) {
		game->bullets[i].pos.y = -1000;
	}
	destroyBullets(game);

	for (int i = 0; i < game->actPickups; i++) {
		game->pickups[i].pos.y = SCREEN_HEIGHT + renderDis + 1000;
	}
	destroyPickups(game);

	setDefValues(game);
}

//glowna petla gry
void gameLoop(SDLdata *data, gameData *game) {
	while (!game->quit) {
		if (!game->paused && !game->car[0].dead && !game->car[1].dead && !game->loadMenu && !game->scoreMenu && !game->startMenu) {
			handleEvent(game, data);

			updateGameData(game, data);
			handlePlayer(game, *data);
			handleMovableObjects(game, data);
			handleEffects(game);

			if(!game->car[0].dead && !game->car[1].dead)
				detectPlayerCollisions(game, *data);

			prepareDataToRender(game, data);
			renderData(data);

			game->frames++;

		}
		else if (game->loadMenu) {
			updateTimerPaused(game);
			gameMenu(game, data, load);
		}
		else if (game->scoreMenu) {
			updateTimerPaused(game);
			gameMenu(game, data, scores);
		}
		else if (game->startMenu) {
			updateTimerPaused(game);
			gameMenu(game, data, chPlayers);
		}
		else if (game->paused) {
			updateTimerPaused(game);
			checkUnpause(game, data);
			prepareDataToRender(game, data);
			renderData(data);
		}
		else if (game->waitForNewGame) {	//dead + endscreen rendered
			waitForNewGame(game, data);
		}
		else if (game->car[0].dead || game->car[1].dead) {
			if (game->revives > 0){
				handleEvent(game, data);
				handlePlayer(game, *data);
				handleMovableObjects(game, data);
				updateTimerPaused(game);
				handleRevive(game, *data);
				updateFPS(game);
				prepareDataToRender(game, data);
				renderData(data);
			}
			else {
				handleDeathScreen(game, data);
				renderData(data);
			}
		}
	}
}

int main(int argc, char **argv) {
	gameData game;
	SDLdata data;
	
	if (SDLsetup(&data) || loadImgs(&data))
		return 1;

	SDL_SetColorKey(data.charset, true, bialy);
	game.t1 = SDL_GetTicks();
	setDefValues(&game);
	game.startMenu = 1;
	initGameMenu(&game, &data, chPlayers);
	gameLoop(&data, &game);

	freeMem(&data);
	SDL_Quit();
	return 0;
};