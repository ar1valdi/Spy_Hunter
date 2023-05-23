#define _USE_MATH_DEFINES

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#ifdef __cplusplus
extern "C"
#endif

//programming stuff
#define SCREEN_WIDTH	1040
#define SCREEN_HEIGHT	680
#define deathScreenWidth SCREEN_WIDTH /2
#define bufSize			128
#define bialy			0x000000
#define devModeRed		0
#define devModeYel		0

//HUD	
#define borderOffsizeY	14
#define topbarX			4
#define topbarY			4
#define versesOffset	8
#define letterSize		8
#define downbarY		SCREEN_HEIGHT - 18
#define downbarX		SCREEN_WIDTH - 11

#define ptsColsPos		1
#define effectsPos		2
#define revsPos			3

//menu pos
#define menuTitleOffY	15
#define navPosY			1
#define savesTitleOffY	1
#define load			1
#define scores			2
#define chPlayers		3
#define wrldTime		4
#define pnts			5

//modes
#define notDestroyed	0
#define fromRight		1
#define fromLeft		2
#define fromUp			3
#define fromDown		4
#define explode 		5

#define mUp				1
#define mDown			2
#define mLeft			3
#define mRight			4

#define idle			1
#define attack			2
#define justRide		3
#define chase			4
#define idleModeTime	3
#define rideModeTime	2
#define chaseModeTime	2

//game config
#define npcKillLockTime	3
#define killPoints		1000
#define renderDis		500
#define greenAfterKillTime 1
#define scoreSpeed		0.1
#define endsreenTime	4

//pickups
#define maxPickups		1
#define maxPickupSpeed	200
#define pickupSpawnChance 0.1
#define ammoLonger		1
#define ammoFaster		2
#define addCar			3
#define ammoLongerChance 33
#define ammoFasterChance 33
#define ammoFasterAmount 100
#define ammoLongerAmount 100

//weapons
#define basicShootingDis 150
#define longShootingDis 300
#define basicShootingRate 4	//per sec
#define fasterShootingRate 8
#define bulletSpeed		600

//decs
#define maxDecsNumber	100
#define decsSpawnrate	100
#define decsOffset		40
#define decsOffsetBorder 20
#define decsFromRoadOffset 70
#define drzewo1			1
#define drzewo2			2
#define skala1			3

//player
#define startSpeed		1600
#define speedingSpeed	500
#define slowingSpeed	500
#define maxSpeed		2500
#define moveSpeedH		300
#define startX			SCREEN_WIDTH / 2
#define twoPlrsOff		defRoadWidth/4
#define startY			SCREEN_HEIGHT - 150		
#define pushSpeed		700
#define defRevives		2
#define infRevivesTime	10
#define ptsReqToRev		15000
#define deathSlowdown	1500
#define startSpeedAfterDeath 1600

//road
#define defRoadWidth	200
#define roadChangeDis	10000
#define maxRoadWidth	300
#define minRoadWidth	100
#define maxRoadDelta	50
#define roadChangeSpeed 1000

//cars general
#define carRoadOffset	40
#define carEachOffset	10

//npc cars
#define NPCmaxCars		2
#define NPCspawnChance	1
#define NPCOffsetX		40
#define NPCOffsetY		40
#define NPCspeed		1500

//enemies
#define maxEnemies		1
#define enemySpawnChance 0.02
#define enemyDefSpeed	1500
#define enemySpeedV		200
#define enemySpeedup	50
#define enemyAttTrigDis	55
#define enemyAlwaysAttSpeed 400


struct SDLdata {
	SDL_Surface* screen = NULL, * charset = NULL;
	SDL_Surface* charsetRed = NULL;
	SDL_Surface* charsetGreen = NULL;
	SDL_Surface* car = NULL;
	SDL_Surface* car2 = NULL;
	SDL_Surface* tree1 = NULL;
	SDL_Surface* tree2 = NULL;
	SDL_Surface* rock1 = NULL;
	SDL_Surface* enemy1 = NULL;
	SDL_Surface* carNPC = NULL;
	SDL_Surface* carNPCdesR = NULL;
	SDL_Surface* carNPCdesL = NULL;
	SDL_Surface* carNPCdesR_green = NULL;
	SDL_Surface* carNPCdesL_green = NULL;
	SDL_Surface* boomGreen = NULL;
	SDL_Surface* boomGrey = NULL;
	SDL_Surface* bullet = NULL;
	SDL_Surface* moreDis = NULL;
	SDL_Surface* fasterShoot  = NULL;
	SDL_Surface* newCar = NULL;

	SDL_Texture* scrtex = NULL;
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	int czarny, zielony, czerwony, niebieski ,szary;
};

struct position {
	double x = -1;
	double y = -1;
};

struct decoration {
	position pos;
	int type = -1;
	bool destroyed = 0;
};

struct NPC {
	position pos;
	double roadOffsetX = 0;
	short destroyed = notDestroyed;
	bool changedRoad = 0;
};

struct enemy {
	position pos, oldPos;
	double roadOffsetX = 0;
	double speed = enemyDefSpeed;
	short direction = mUp;
	short destroyed = notDestroyed;
	short action = justRide;
	bool changedRoad = 0;
	bool goBackToRoad = 0;
};

struct player {
	position pos;
	position prevPos;
	short destroyed = notDestroyed;
	bool shooting = false;
	bool lastBulletOnLeft = false;
	short shootingDis;
	short shootingRate;
	int ammoFasterLeft = 0;
	int ammoLongerLeft = 0;
	double bulletOffTimer;
	bool dead = 0;
};

struct singleTimers {
	double killColorTimer;
	double deathScreenTimer;
	double fpsTimer;
};

struct bullet {
	position pos;
	double doneDistance = 0;
	double range = basicShootingDis;
};

struct pickup {
	position pos;
	double speed = maxPickupSpeed;
	double roadOffsetX = roadOffsetX;
	short mode = ammoLonger; 
};

struct gameData {
	int t1, t2, quit, frames, col, revives;
	double delta, worldTime, fps, speed, distance, pts, prevDistance, lockPts, paused, waitForNewGame, lastRevive;
	bool revivedProtection = 0, startMenu,loadMenu, scoreMenu, canAddScore, twoPlayers;

	singleTimers timers;
	player car[2];

	decoration decs[maxDecsNumber];
	int actDecs;

	NPC npcCars[NPCmaxCars];
	int actNPCs;

	enemy enemies[maxEnemies];
	int actEnemies;

	bullet* bullets;
	int actBullets = 0;

	int roadChangePosY;
	int roadWidth;
	int roadNewWidth;
	double disToNewRoad;

	pickup pickups[maxPickups];
	int actPickups = 0;

	int pickedMenuPos = 1;
	int actMenuPos = 0;
	short menuPage = 0;
	short menuMaxOnPage = 0;
	char** menuPos = NULL;
	double** scoreBoard = NULL;
};

//ustaw podstawowe dane zmiennym
void setDefValues(gameData* game);

//zwolnij pamiec zarezerwowana dla SDL_Surface
void freeMem(SDLdata* data);

//zwraca losowego inta miedzy 0 a maxVal wlacznie
int random(int maxValue, int seedOff, gameData game);

//losuje true lub false z szansa chance% na wylosowanie true, randOff pozwala przemieszac wyniki
bool precChance(double chance, int randOff, gameData game);

//zwaraca true jezelei pos jest na drodze, false w przeciwnym razie
bool isOnRoad(position pos, gameData game, SDLdata data);

//ustawia niezbedne rzeczy do wyswietlania okna, true jezeli wszystko poprawnie ustawiono, przeciwnie false
bool SDLsetup(SDLdata* data);

//sprawdza czy poprawnia zaladowano bitmape
bool checkSurface(SDL_Surface* surf, SDLdata* data, char name[]);

//zaladowanie bitmap do struktury data, zwraca true jezeli nie udalo sie zaladowac jednej z grafik, przeciwnie false
bool loadImgs(SDLdata* data);

//obs³u¿ zapis gry
void saveGame(gameData* game);

//stworz nowa gre, wyzeruj wszystkie tablice, ustaw podstawowe wartosci zmiennych
void newGame(gameData* game);

//obs³u¿ input w menu, wyrysuj dane
void gameMenu(gameData* game, SDLdata* data, short type);

//przygotuj tablicê do wyswietlenia w menu
void initGameMenu(gameData* game, SDLdata* data, short type);

//dodaj wynik do pliku z najlepszymi wynikami
void addHighScore(double pts, double time, bool* canAdd);

void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset);
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y);
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color);
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color);
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor);