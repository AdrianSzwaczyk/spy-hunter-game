#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}
#define SCREEN_WIDTH			1000
#define SCREEN_HEIGHT			700
#define SEC_TO_MILISEC			1000										//convert
#define MAX_NPC					30											//max number of npc cars on the road at once
#define LINE_LENGTH				10											//length of the lines in the middle of the road
#define SCREEN_RATIO			2											//if you change the screen w or h, you also want to change this, I didn't manage to calculate the right ratio (it speeds everything up, relative to the road)
#define PLAYER_MAX_SPEED		15		* SCREEN_RATIO					 
#define PLAYER_MIN_SPEED		5		* SCREEN_RATIO
#define PLAYER_DEFAULT_SPEED	10		* SCREEN_RATIO
#define ENEMY_MAX_SPEED			12		* SCREEN_RATIO
#define ENEMY_MIN_SPEED			4		* SCREEN_RATIO
#define ACCELERATION			0.05	* SCREEN_RATIO
#define BULLET_SPEED			30		* SCREEN_RATIO
#define EXPLOSION_FRAMES		13							
#define JAMES_BOND				0.07 * 1.5									//explosion animation speed
#define MAX_BULLETS				30											//max number of bullets existing at once
#define HP_BAR_HIGHT			8
#define HP_BAR_DISTANCE			15
#define CAR_NUM					9											//number of cars
#define PLAYER					0											//player's car model
#define	ENEMY					1											//enemy's car model
#define	BULLET_LENGHT			10
#define	BULLET_WIDTH			5
#define SCORE_FREEZE_TIME_MS	(3 * SEC_TO_MILISEC)
#define MAX_HP					3
#define MIN_ROAD_WIDTH			200
#define MAX_ROAD_WIDTH			(SCREEN_WIDTH - 200)
#define INIT_ROAD_WIDTH			800
#define INIT_ROAD_SIDE_WIDTH	(SCREEN_WIDTH - INIT_ROAD_WIDTH) / 2
#define ROAD_SEGMENT_HEIGHT		10
#define	NUM_OF_ROAD_SEGMENTS	(SCREEN_HEIGHT / ROAD_SEGMENT_HEIGHT + 2)
#define MIN_ROAD_SIDE_WIDTH		(SCREEN_WIDTH - MAX_ROAD_WIDTH) / 2
#define MAX_ROAD_SIDE_WIDTH		(SCREEN_WIDTH - MIN_ROAD_WIDTH) / 2
#define	MIN_ROAD_SIDE_CHANGE	100
#define HITBOX_MARGIN			10
#define NPC_SPAWNRATE			10											//more -> less frequent spawning
#define LOWEST_NPC_SPAWNRATE	200											//more -> less frequent spawning
#define HIGHEST_NPC_SPAWNRATE	50											//more -> less frequent spawning
#define SPAWN_COOLDOWN			(0.1 * SEC_TO_MILISEC)
#define MAX_DIFFICULTY_TIME		(100 * SEC_TO_MILISEC)						//time after which max spawnrate is reached
#define RELOAD_TIME				300

//colors container
class rgb {
public:
	int black;
	int white;
	int green;
	int red;
	int blue;
	int grassGreen;
	rgb(SDL_Surface* screen) {
		black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
		white = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
		green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
		grassGreen = SDL_MapRGB(screen->format, 0x00, 0xAA, 0x00);
		red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
		blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	}
};
//data defining the game
class gameState {
public:
	int t1 = 0, t2 = 0, rc = 0;
	int reloadTimer = 0, createTick = 0, gameTime = 0, frames = 0, freezeScore = 0, roadChange = 0, roadNewWidth = INIT_ROAD_SIDE_WIDTH, resizedRoadCount = 0, slope, lastWidth = INIT_ROAD_SIDE_WIDTH;
	double score = 0, initial = 0, fps = 0, delta = 0, fpsTimer = 0, yChange = 0, linesY = 0;
	bool pause = false, collision = false, startNew = false, quit = false, fScreen = false;
	void timeValuesChange(SDL_Surface* screen, rgb colors, double speed) {
		t2 = SDL_GetTicks();
		yChange += speed / 10 * delta / 4;
		delta = (t2 - t1);
		t1 = t2;
		gameTime += delta;
		if (freezeScore > 0) freezeScore -= delta;
		else score += delta * 0.1 * speed / 8;
		reloadTimer += delta;
		fpsTimer += delta * 0.001;
		linesY += speed / 10 * delta / 4;
		if (fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		};
	}
};
//images
struct bmp {
	SDL_Surface* eti = NULL, * playerCar = NULL, * enemyCar = NULL, * charset = NULL;
	SDL_Surface* exp[EXPLOSION_FRAMES], * cars[CAR_NUM];
};

//functions for loading images, creating window, updating screen, checking if there was no error and one for closing, clearing and ending everything
bool load(SDL_Surface* screen, SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer, bmp* images) {
	bool err = false;
	char fileName[30];
	images->charset = SDL_LoadBMP("./images/cs8x8.bmp");
	images->eti = SDL_LoadBMP("./images/eti.bmp");
	for (int i = 0; i < EXPLOSION_FRAMES; i++) {
		sprintf(fileName, "./images/explosion%d.bmp", i + 1);
		images->exp[i] = SDL_LoadBMP(fileName);
	}
	for (int i = 0; i < EXPLOSION_FRAMES; i++) {
		if (images->exp[i] == NULL) err = true;
	}
	for (int i = 0; i < CAR_NUM; i++) {
		sprintf(fileName, "./images/car%d.bmp", i);
		images->cars[i] = SDL_LoadBMP(fileName);
	}
	for (int i = 0; i < CAR_NUM; i++) {
		if (images->cars[i] == NULL) err = true;
	}
	if (images->charset == NULL || images->eti == NULL || err) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(images->eti);
		SDL_FreeSurface(images->charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	}
	else {
		return 0;
	}
	SDL_SetColorKey(images->charset, true, 0x000000);
}
void setup(SDL_Renderer* renderer, SDL_Window* window) {
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(window, "SpyHunter");
	SDL_ShowCursor(SDL_DISABLE);
}
bool windowCreated(int rc) {
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 0;
	}
	else
		return 1;
}
bool createWindow(SDL_Window** window, SDL_Renderer** renderer, SDL_Surface** screen, SDL_Texture** scrtex, bool fScreen) {
	int rc;
	if (fScreen)
		rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, window, renderer);
	else
		rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, window, renderer);
	if (!windowCreated(rc)) {
		return 1;
	}
	*screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	*scrtex = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	setup(*renderer, *window);
	return 0;
}
void updateScreen(SDL_Texture* scrtex, SDL_Surface* screen, SDL_Renderer* renderer) {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
	SDL_RenderCopy(renderer, scrtex, NULL, NULL);
	SDL_RenderPresent(renderer);
}
void endAll(SDL_Surface* screen, SDL_Texture* scrtex, SDL_Renderer* renderer, SDL_Window* window) {
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

//template drawing functions
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset) {
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
	if (x > 0 && x < SCREEN_WIDTH && y > 0 && y < SCREEN_HEIGHT) {
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
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

//displaying info
void displayInfo(SDL_Surface* screen, bmp images, gameState g, rgb colors) {
	char text[128];
	DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, colors.red, colors.blue);
	sprintf(text, "Score: %.0lf    Time = %.1lf s     %.0lfFPS", g.score, g.gameTime * 0.001, g.fps);
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, images.charset);
	sprintf(text, "Adrian Szwaczyk");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, images.charset);
}
void displayImplementedFeatures(SDL_Surface* screen, bmp images, rgb colors) {
	char features[200] = "Arrows - moving, space - shooting, P - pause, N - new game, F11 - fullscreen, ESC - quit";
	DrawRectangle(screen, SCREEN_WIDTH - strlen(features) * 8 - 12, SCREEN_HEIGHT - 12, strlen(features) * 8 + 4, 12, colors.blue, colors.blue);
	DrawString(screen, SCREEN_WIDTH - strlen(features) * 8 - 10, SCREEN_HEIGHT - 10, features, images.charset);
}

//drawing functions
double drawRoadLines(SDL_Surface* screen, double linesY, int speed, int color) {
	for (int i = 0; i < SCREEN_HEIGHT / LINE_LENGTH / 5 + 1; i++) {
		if (linesY >= 5 * LINE_LENGTH) {
			linesY = 0;
		}
		DrawRectangle(screen, SCREEN_WIDTH / 2, i * 5 * LINE_LENGTH + linesY, 3, LINE_LENGTH, color, color);
	}
	return linesY;
}
int drawExplosion(SDL_Surface* screen, bmp images, double* expFrames, int x, int y, int delta) {
	int destroyed = 1;
	*expFrames += JAMES_BOND * delta / 4;
	int img = int(floor(*expFrames));
	if (*expFrames > EXPLOSION_FRAMES) destroyed = 2;
	if (destroyed != 2) DrawSurface(screen, images.exp[img], x, y);
	return destroyed;
}

//class for grass on both sides of the road
class roadSegment {
public:
	SDL_Rect left, right;
	int yDisplace = 0;
	Uint32 color;
	roadSegment(rgb colors) {
		left.w = INIT_ROAD_SIDE_WIDTH;
		right.w = INIT_ROAD_SIDE_WIDTH;
		left.h = ROAD_SEGMENT_HEIGHT;
		right.h = ROAD_SEGMENT_HEIGHT;
		left.x = 0;
		right.x = SCREEN_WIDTH - INIT_ROAD_SIDE_WIDTH;
		color = colors.grassGreen;
	}
	void changeWidth(int resize, int direction, int slope) {
		left.w += resize * direction;
		right.w += resize * direction;
		right.x = SCREEN_WIDTH - right.w;
	}
	void setWidth(int width) {
		left.w = width;
		right.w = width;
		right.x = SCREEN_WIDTH - right.w;
	}
	void giveY(int number) {
		left.y = (number - 1) * ROAD_SEGMENT_HEIGHT;
		right.y = (number - 1) * ROAD_SEGMENT_HEIGHT;
	}
	void drawRoad(SDL_Surface* screen, rgb colors) {
		DrawRectangle(screen, left.x, left.y, left.w, left.h, color, color);
		DrawRectangle(screen, right.x, right.y, right.w, right.h, color, color);
	}
	bool collide(SDL_Rect entity) {
		entity.w -= 10;
		entity.x += 5;
		if (SDL_HasIntersection(&entity, &left)) return 1;
		if (SDL_HasIntersection(&entity, &right)) return 1;
		return 0;
	}
	void move(double speed, int number, gameState* g) {
		left.y = g->yChange - (number - NUM_OF_ROAD_SEGMENTS - 1 + yDisplace) * ROAD_SEGMENT_HEIGHT;
		right.y = g->yChange - (number - NUM_OF_ROAD_SEGMENTS - 1 + yDisplace) * ROAD_SEGMENT_HEIGHT;
		if (left.y > SCREEN_HEIGHT) {
			yDisplace += NUM_OF_ROAD_SEGMENTS;
			if (g->roadChange) {
				if (g->lastWidth <= g->roadNewWidth - g->slope || g->lastWidth >= g->roadNewWidth + g->slope) setWidth(g->lastWidth + g->roadChange);
				else {
					g->roadChange = 0;
					setWidth(g->lastWidth);
				}
			}
			else setWidth(g->lastWidth);
			left.y = -ROAD_SEGMENT_HEIGHT;
			left.y = -ROAD_SEGMENT_HEIGHT;
		}
		if (left.w > MAX_ROAD_SIDE_WIDTH) {
			g->lastWidth = MAX_ROAD_SIDE_WIDTH;
			g->roadNewWidth = MAX_ROAD_SIDE_WIDTH;
		}
		else if (left.w < MIN_ROAD_SIDE_WIDTH) {
			g->lastWidth = MIN_ROAD_SIDE_WIDTH;
			g->roadNewWidth = MIN_ROAD_SIDE_WIDTH;
		}
		g->lastWidth = left.w;
	}
};
roadSegment* createRoadSegment(rgb colors) {
	roadSegment& r = *(new roadSegment(colors));
	return &r;
}
void collideWithRoad(roadSegment* road[NUM_OF_ROAD_SEGMENTS], SDL_Surface* screen, rgb colors, SDL_Rect hitbox, int* destroyed, double* speed) {
	for (int i = 0; i < SCREEN_HEIGHT / ROAD_SEGMENT_HEIGHT + 2; i++) {
		if (road[i]->collide(hitbox)) {
			*destroyed = 1;
			*speed = 0;
		}
	}
}
void displayAndChangeRoad(roadSegment* road[NUM_OF_ROAD_SEGMENTS], SDL_Surface* screen, rgb colors, double speed, gameState* g, bmp images) {
	if (SDL_GetTicks() % 10 == rand() % 500 && g->roadChange == 0) {
		g->slope = 2 + rand() % 3;
		int range;
		if (g->roadNewWidth <= MIN_ROAD_SIDE_WIDTH + 100) g->roadChange = 1;
		else if (g->roadNewWidth >= MAX_ROAD_SIDE_WIDTH - 100) g->roadChange = -1;
		else g->roadChange = 2 * (rand() % 2) - 1;
		if (g->roadChange == 1) range = MAX_ROAD_SIDE_WIDTH - g->roadNewWidth;
		else range = g->roadNewWidth - MIN_ROAD_SIDE_WIDTH;
		g->roadNewWidth = g->roadNewWidth + (MIN_ROAD_SIDE_CHANGE + rand() % range - MIN_ROAD_SIDE_CHANGE) * g->roadChange;
	}
	for (int i = 0; i < SCREEN_HEIGHT / ROAD_SEGMENT_HEIGHT + 2; i++) {
		road[i]->drawRoad(screen, colors);
		road[i]->move(speed, i, g);
	}
}

class bullet {
public:
	int x, y;
	double speed;
	SDL_Rect hitbox;
	void setValues(double px, double py) {
		x = ceil(px) - BULLET_WIDTH / 2;
		y = ceil(py);
		hitbox.x = ceil(px) - BULLET_WIDTH / 2;
		hitbox.y = ceil(py);
		hitbox.w = BULLET_WIDTH;
		hitbox.h = BULLET_LENGHT;
		speed = BULLET_SPEED;
	}
	void move(double playerSpeed, int delta) {
		y -= (speed / 10 - playerSpeed / 10) * delta / 4;
		hitbox.x = x;
		hitbox.y = y;
	}
	void draw(SDL_Renderer* renderer, rgb color, SDL_Surface* screen) {
		DrawRectangle(screen, hitbox.x, hitbox.y, hitbox.w, hitbox.h, color.white, color.white);
	}
};
bullet* createBullet(double x, double y) {
	bullet& b = *(new bullet);
	b.setValues(x, y);
	return &b;
}

class npc {
public:
	int destroyed = 0;
	int destroyTicks = 0;
	int maxHp = MAX_HP;
	int hp = MAX_HP;
	bool enemy = rand() % 2;
	double speed = PLAYER_DEFAULT_SPEED;
	int x;
	double expFrames = 0;
	double y = SCREEN_HEIGHT / 3 * 2;
	SDL_Surface* carModel;
	SDL_Rect hitbox;
	npc(bmp images, gameState g) {
		if (!enemy) {
			int num = 2 + rand() % (CAR_NUM - 2);
			carModel = images.cars[num];
		}
		else carModel = images.cars[ENEMY];
		hitbox.w = carModel->w - 3;
		hitbox.h = carModel->h - 3;
		if (!(rand() % 4) && enemy) y = SCREEN_HEIGHT + 300;
		else y = -100;
		if (g.roadChange == 1 && y != -200) {
			x = g.roadNewWidth + hitbox.w / 2 + rand() % (SCREEN_WIDTH - 2 * g.roadNewWidth - hitbox.w / 2);
		}
		else x = g.lastWidth + hitbox.w / 2 + rand() % (SCREEN_WIDTH - 2 * g.lastWidth - hitbox.w / 2);
		while (abs(speed - PLAYER_MAX_SPEED) < 2 || abs(speed - PLAYER_DEFAULT_SPEED) < 2 || abs(speed - PLAYER_MIN_SPEED) < 2) {
			if (y > SCREEN_HEIGHT) speed = (PLAYER_MAX_SPEED + rand() % ENEMY_MIN_SPEED);
			else speed = (ENEMY_MIN_SPEED + rand() % (ENEMY_MAX_SPEED - ENEMY_MIN_SPEED));
		}
	}
	void drawNPC(SDL_Surface* screen, bmp images, rgb colors, double delta) {
		if (destroyed == 1) {
			hitbox.w = 0;
			hitbox.h = 0;
			destroyed = drawExplosion(screen, images, &expFrames, x, y, delta);
		}
		else
			DrawSurface(screen, carModel, x, y);
		if (hitbox.x > 0 && hitbox.y + hitbox.h + HP_BAR_DISTANCE > 0 && hitbox.y + hitbox.h + HP_BAR_DISTANCE + HP_BAR_HIGHT < SCREEN_HEIGHT && hp > 0 && hp < maxHp) {
			DrawRectangle(screen, hitbox.x - 1, hitbox.y + hitbox.h + HP_BAR_DISTANCE - 1, hitbox.w, HP_BAR_HIGHT + 2, colors.black, colors.red);
			DrawRectangle(screen, hitbox.x, hitbox.y + hitbox.h + HP_BAR_DISTANCE, hitbox.w / maxHp * hp, HP_BAR_HIGHT, colors.green, colors.green);
		}
	}
	void move(double playerSpeed, double delta) {
		y += (playerSpeed / 10 - speed / 10) * delta / 4;

		hitbox.x = x - hitbox.w / 2;
		hitbox.y = y - hitbox.h / 2;
	}
};
npc* createNPC(SDL_Surface* screen, bmp images, gameState g) {
	npc& e = *(new npc(images, g));
	return &e;
}
void randomNPCspawn(npc* npcCars[MAX_NPC], SDL_Surface* screen, bmp images, gameState* g, int playerDestroyed) {
	double spawIncrease = 1;
	if (g->gameTime / MAX_DIFFICULTY_TIME < 1) {
		spawIncrease = double(g->gameTime) / MAX_DIFFICULTY_TIME;
	}
	if ((rand() % (LOWEST_NPC_SPAWNRATE + HIGHEST_NPC_SPAWNRATE - int(LOWEST_NPC_SPAWNRATE * (spawIncrease))) == g->t2 % (LOWEST_NPC_SPAWNRATE + HIGHEST_NPC_SPAWNRATE - int(LOWEST_NPC_SPAWNRATE * (spawIncrease))) && g->gameTime - g->createTick > SPAWN_COOLDOWN) && !playerDestroyed) {
		for (int i = 0; i < MAX_NPC; i++) {
			if (npcCars[i] == NULL) {
				npcCars[i] = createNPC(screen, images, *g);
				break;
			}
		}
		g->createTick = g->gameTime;
	}
}
void updateNPC(SDL_Surface* screen, bmp images, npc* npcCars[MAX_NPC], int speed, double delta, rgb colors, roadSegment* road[NUM_OF_ROAD_SEGMENTS]) {
	for (int i = 0; i < MAX_NPC; i++) {
		if (npcCars[i] != NULL) {
			npcCars[i]->drawNPC(screen, images, colors, delta);
			npcCars[i]->move(speed, delta);
			collideWithRoad(road, screen, colors, npcCars[i]->hitbox, &npcCars[i]->destroyed, &npcCars[i]->speed);
			if (npcCars[i]->y > SCREEN_HEIGHT * 2 || npcCars[i]->destroyed == 2 || npcCars[i]->y < -SCREEN_HEIGHT * 2) {
				delete npcCars[i];
				npcCars[i] = NULL;
			}
		}
	}
}

class player {
private:
	double initial = 0;
	bool l = 0, r = 0, accUp = 0, accDown = 0;
	int accelerate = 0, moved = 0;
	void positionChange() {
		if (destroyed != 1) {
			if (moved == 1) {
				x += (SDL_GetTicks() - initial) / 4.5;
			}
			else if (moved == -1) {
				x -= (SDL_GetTicks() - initial) / 4.5;
			}
			else {
				hitbox.w = carModel->w - HITBOX_MARGIN;
				hitbox.x = x - hitbox.w / 2;
			}
			if (moved != 0) {
				hitbox.w = carModel->w - HITBOX_MARGIN - 20;
				hitbox.x = x - hitbox.w / 2;
				initial = SDL_GetTicks();
			}
			y = SCREEN_HEIGHT / 3 * 2 - (speed - PLAYER_DEFAULT_SPEED) * 5;
		}
	}
	void speedChange(int delta) {
		if (accelerate == 1 && speed < PLAYER_MAX_SPEED) {
			speed += ACCELERATION * accelerate * delta / 4;
		}
		else if (accelerate == -1) {
			if (speed > PLAYER_MIN_SPEED) speed += ACCELERATION * accelerate * delta / 4;
			else if (speed < PLAYER_MIN_SPEED - ACCELERATION) speed += ACCELERATION * delta / 4;
		}
		else if (accelerate == 0) {
			if (speed > PLAYER_DEFAULT_SPEED) {
				if (PLAYER_DEFAULT_SPEED - speed > ACCELERATION) speed = PLAYER_DEFAULT_SPEED;
				else speed -= ACCELERATION * delta / 4;
			}
			else if (speed < PLAYER_DEFAULT_SPEED) {
				if (PLAYER_DEFAULT_SPEED - speed < ACCELERATION) speed = PLAYER_DEFAULT_SPEED;
				else speed += ACCELERATION * delta / 4;
			}
		}
	}
public:
	SDL_Rect hitbox;
	SDL_Surface* carModel;
	bool shooting = false;
	double expFrames = 0;
	int destroyed = 0;
	double speed = 0, x = SCREEN_WIDTH / 2 + 1, y = SCREEN_HEIGHT / 3 * 2;
	player(SDL_Surface* carModel) : carModel(carModel) {
		hitbox.w = carModel->w - HITBOX_MARGIN;
		hitbox.h = carModel->h - HITBOX_MARGIN * 2;
	}
	void state(SDL_Event event) {
		switch (event.type) {
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_UP) {
				accelerate = 1;
				accUp = 1;
			}
			else if (event.key.keysym.sym == SDLK_DOWN) {
				accelerate = -1;
				accDown = 1;
			}
			else if (event.key.keysym.sym == SDLK_LEFT) {
				moved = -1;
				initial = SDL_GetTicks();
				l = 1;
			}
			else if (event.key.keysym.sym == SDLK_RIGHT) {
				moved = 1;
				initial = SDL_GetTicks();
				r = 1;
			}
			else if (event.key.keysym.sym == SDLK_SPACE) {
				shooting = 1;
			}
			break;
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_p) {
				clearMove();
				shooting = 0;
			}
			else if (event.key.keysym.sym == SDLK_SPACE) shooting = 0;
			if (event.key.keysym.sym == SDLK_LEFT) {
				moved = 1;
				l = 0;
			}
			else if (event.key.keysym.sym == SDLK_RIGHT) {
				moved = -1;
				r = 0;
			}
			else if (event.key.keysym.sym == SDLK_UP) {
				accUp = 0;
				accelerate = -1;
			}
			else if (event.key.keysym.sym == SDLK_DOWN) {
				accDown = 0;
				accelerate = 1;
			}
			if (l == 0 && r == 0) {
				moved = 0;
			}
			if (accUp == 0 && accDown == 0) {
				accelerate = 0;
			}
			break;
		}
	}
	void move(SDL_Surface* screen, double delta, bmp images) {
		positionChange();
		speedChange(delta);
		if (destroyed == 1) {
			destroyed = drawExplosion(screen, images, &expFrames, x, y, delta);
			hitbox.w = 0;
			hitbox.h = 0;
			speed = 0;
			if (destroyed == 2) {
				x = SCREEN_WIDTH / 2;
				y = SCREEN_HEIGHT / 3 * 2;
			}
		}
		else if (destroyed == 2) {
			expFrames += delta;
			if (int(expFrames) % 1000 / 500 == 0) DrawSurface(screen, carModel, x, y);
			if (expFrames > 2000) {
				expFrames = 0;
				destroyed = 3;
			}
		}
		else if (destroyed == 3) respawn();
		else DrawSurface(screen, carModel, x, y);
		hitbox.x = x - hitbox.w / 2;
		hitbox.y = y - hitbox.h / 2;
	};
	void clearMove() {
		l = 0, r = 0;
		accUp = 0, accDown = 0;
		accelerate = 0;
		moved = 0;
	}
	bullet* shoot() {
		return createBullet(x, y);
	}
	void respawn() {
		expFrames = 0;
		destroyed = 0;
		hitbox.w = carModel->w - HITBOX_MARGIN;
		hitbox.h = carModel->h - HITBOX_MARGIN * 2;
		hitbox.x = x - hitbox.w / 2;
		hitbox.y = x - hitbox.w / 2;
	}
	void playerShoot(gameState* g, bullet* bullets[MAX_BULLETS]) {
		if (shooting && g->reloadTimer > RELOAD_TIME && !destroyed) {
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (bullets[i] == NULL) {
					bullets[i] = shoot();
					g->reloadTimer = 0;
					break;
				}
			}
		}
	}
};
void collidePlayerWithNPC(npc* npcCars[MAX_NPC], player* p1, gameState* g) {
	for (int i = 0; i < MAX_NPC; i++) {
		if (npcCars[i] != NULL && SDL_HasIntersection(&p1->hitbox, &npcCars[i]->hitbox)) {
			npcCars[i]->destroyed = 1;
			npcCars[i]->hp = 0;
			p1->destroyed = 1;
			if (npcCars[i]->enemy) g->score += 1000;
			else g->freezeScore = SCORE_FREEZE_TIME_MS;
			npcCars[i]->speed = 0;
			npcCars[i]->destroyTicks = SDL_GetTicks();
			break;
		}
	}
}
void hitEnemiesWithBullets(gameState* g, npc* npcCars[MAX_NPC], bullet* bullets[MAX_BULLETS]) {
	for (int i = 0; i < MAX_NPC; i++) {
		for (int b = 0; b < MAX_BULLETS; b++) {
			if (npcCars[i] != NULL && bullets[b] != NULL && SDL_HasIntersection(&bullets[b]->hitbox, &npcCars[i]->hitbox)) {
				delete bullets[b];
				bullets[b] = NULL;
				npcCars[i]->hp -= 1;
				if (npcCars[i]->hp == 0) {
					if (npcCars[i]->enemy) g->score += 1000;
					else g->freezeScore = SCORE_FREEZE_TIME_MS;
					npcCars[i]->destroyed = 1;
					npcCars[i]->speed = 0;
					npcCars[i]->destroyTicks = SDL_GetTicks();
				}
				break;
			}
		}
	}
}
void drawPlayerBullets(gameState g, bullet* bullets[MAX_BULLETS], player p1, SDL_Renderer* renderer, rgb colors, SDL_Surface* screen) {
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (bullets[i] != NULL) {
			bullets[i]->move(p1.speed, g.delta);
			if (bullets[i]->y < -100) {
				delete bullets[i];
				bullets[i] = NULL;
			}
			else
				bullets[i]->draw(renderer, colors, screen);
		}
	}
}

//check every collision in the game
void handleCollisions(roadSegment* road[NUM_OF_ROAD_SEGMENTS], SDL_Surface* screen, rgb colors, npc* npcCars[MAX_NPC], bullet* bullets[MAX_BULLETS], player* p1, gameState* g) {
	collideWithRoad(road, screen, colors, p1->hitbox, &p1->destroyed, &p1->speed);
	collidePlayerWithNPC(npcCars, p1, g);
	hitEnemiesWithBullets(g, npcCars, bullets);
}

//put pointers/NULLs into arrays of objects existing in a game - initialise them
void createObjectArrays(roadSegment* road[NUM_OF_ROAD_SEGMENTS], npc* npcCars[MAX_NPC], bullet* bullets[MAX_BULLETS], rgb colors) {
	for (int i = 0; i < NUM_OF_ROAD_SEGMENTS; i++) {
		road[i] = createRoadSegment(colors);
		road[i]->giveY(i);
	}
	for (int i = 0; i < MAX_NPC; i++) {
		npcCars[i] = NULL;
	}
	for (int i = 0; i < MAX_BULLETS; i++) {
		bullets[i] = NULL;
	}
}

//change game accordding to player's inoput
int handleEvent(gameState* g, player* p1, SDL_Surface** screen, SDL_Texture** scrtex, SDL_Renderer** renderer, SDL_Window** window, SDL_Event event) {
	if (!g->pause) p1->state(event);
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_ESCAPE) {
			g->quit = 1;
			return 1;
		}
		else if (event.key.keysym.sym == SDLK_n) g->startNew = 1;
		else if (event.key.keysym.sym == SDLK_F11) {
			g->fScreen = !g->fScreen;
			endAll(*screen, *scrtex, *renderer, *window);
			g->quit = createWindow(window, renderer, screen, scrtex, g->fScreen);
			return g->quit;
		}
	}
	else if (event.type == SDL_KEYUP) {
		if (event.key.keysym.sym == SDLK_p) {
			g->pause = !g->pause;
		}
	}
	if (event.type == SDL_QUIT)
		return 1;
	return 0;
}

//function pausing the game
bool paused(gameState* g, player* p1, SDL_Surface** screen, SDL_Texture** scrtex, SDL_Renderer** renderer, SDL_Window** window) {
	bool quit = false;
	while (g->pause) {
		SDL_Event event;
		if (SDL_PollEvent(&event))
			g->t2 = SDL_GetTicks();
		g->t1 = g->t2;
		handleEvent(g, p1, screen, scrtex, renderer, window, event);
		quit = g->quit;
		if (event.key.keysym.sym == SDLK_F11 || g->startNew || quit) {
			break;
		}
	}
	return quit;
}

//draw the game in its current state
void drawGame(gameState* g, player* p1, rgb colors, bmp images, bullet* bullets[MAX_BULLETS], SDL_Surface* screen, SDL_Renderer* renderer, SDL_Texture* scrtex, roadSegment* road[NUM_OF_ROAD_SEGMENTS], npc* npcCars[MAX_NPC]) {
	SDL_FillRect(screen, NULL, colors.black);
	g->linesY = drawRoadLines(screen, g->linesY, p1->speed, colors.white);
	displayAndChangeRoad(road, screen, colors, p1->speed, g, images);
	drawPlayerBullets(*g, bullets, *p1, renderer, colors, screen);
	displayImplementedFeatures(screen, images, colors);
	if (p1->destroyed != 2) p1->move(screen, g->delta, images);
	updateNPC(screen, images, npcCars, p1->speed, g->delta, colors, road);
	if (p1->destroyed == 2) p1->move(screen, g->delta, images);
	displayInfo(screen, images, *g, colors);
	updateScreen(scrtex, screen, renderer);
}

//update the game, according to player actions, movement of every object, changing road etc.
bool gameUpdate(SDL_Surface** screen, SDL_Texture** scrtex, SDL_Window** window, SDL_Renderer** renderer, bmp images, rgb colors, gameState* g, player* p1, roadSegment* road[NUM_OF_ROAD_SEGMENTS], npc* npcCars[MAX_NPC], bullet* bullets[MAX_BULLETS]) {
	bool quit = false;
	SDL_Event event;
	quit = paused(g, p1, screen, scrtex, renderer, window);
	randomNPCspawn(npcCars, *screen, images, g, p1->destroyed);
	g->timeValuesChange(*screen, colors, p1->speed);
	handleCollisions(road, *screen, colors, npcCars, bullets, p1, g);
	p1->playerShoot(g, bullets);
	drawGame(g, p1, colors, images, bullets, *screen, *renderer, *scrtex, road, npcCars);
	while (SDL_PollEvent(&event)) {
		quit = handleEvent(g, p1, screen, scrtex, renderer, window, event);
	};
	g->frames++;
	return quit;
}
//executing until player dies or starts a new game
int game(SDL_Surface** screen, SDL_Texture** scrtex, SDL_Window** window, SDL_Renderer** renderer, bmp images, rgb colors, gameState* g, player* p1, roadSegment* road[NUM_OF_ROAD_SEGMENTS], npc* npcCars[MAX_NPC], bullet* bullets[MAX_BULLETS]) {
	bool quit = 0;
	static bool fScreen;
	createObjectArrays(road, npcCars, bullets, colors);
	g->fScreen = fScreen;
	g->t1 = SDL_GetTicks();
	while (!g->startNew && !quit) {
		quit = gameUpdate(screen, scrtex, window, renderer, images, colors, g, p1, road, npcCars, bullets);
	};
	fScreen = g->fScreen;
	return quit;
}
//executing until player quits
int games(SDL_Surface** screen, SDL_Texture** scrtex, SDL_Window** window, SDL_Renderer** renderer, bmp images, rgb colors) {
	gameState g;
	npc* npcCars[MAX_NPC];
	bullet* bullets[MAX_BULLETS];
	player p1(images.cars[PLAYER]);
	roadSegment* road[NUM_OF_ROAD_SEGMENTS];
	return game(screen, scrtex, window, renderer, images, colors, &g, &p1, road, npcCars, bullets);
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
	int quit = 0;
	SDL_Event event;
	SDL_Surface* screen = NULL;
	SDL_Texture* scrtex = NULL;
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	bmp images;
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		return 1;
	}
	createWindow(&window, &renderer, &screen, &scrtex, false);
	if (load(screen, scrtex, window, renderer, &images)) {								//load images, create window etc. and check if everything was successfull
		return 1;
	}
	rgb colors(screen);
	while (!quit) {																		//every loop execution is a different game
		quit = games(&screen, &scrtex, &window, &renderer, images, colors);
	}
	endAll(screen, scrtex, renderer, window);
	SDL_Quit();
	return 0;
};