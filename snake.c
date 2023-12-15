#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <raylib.h>

int win_width = 1080;
int win_height = 1080;
int border_px = 50;

unsigned snack_apothem = 10;
unsigned snake_apothem = 10;

#define INIT_BUF_SZ 128
#define INIT_SNAKE_LEN 3

enum MovementDirection {
	NONE = 0,
	UP = 1,
	DOWN = 2,
	LEFT = 3,
	RIGHT = 4,
};

typedef struct {
	size_t nrows;
	size_t ncols;

	size_t pxlen_y;
	size_t pxlen_x;

	Rectangle borderrect;

	Rectangle snack;
	unsigned score;
	bool game_over;
	bool game_paused;

	enum MovementDirection snakedir;
	Rectangle *snake;
	size_t snake_cap;
	size_t snake_len;
} PlayingField;

void choose_next_snack_pos(PlayingField *field) {
	int xrange = field->borderrect.width - 2 - 2*snack_apothem;
	int yrange = field->borderrect.height - 2 - 2*snack_apothem;
	int x = rand() % xrange;
	int y = rand() % yrange;
	field->snack = (Rectangle) {.x = border_px + x, .y = border_px + y, .height = 2*snack_apothem, .width = 2*snack_apothem};
}

void grow_snake(PlayingField *field) {
	if (field->snake_len == field->snake_cap) {
		field->snake = reallocarray(field->snake, field->snake_cap*2, sizeof(Rectangle));
		if (!field->snake) {
			perror("error: allocation failure");
			exit(2);
		}
	}

	if (field->snake_len == 0) {
		field->snake[0] = (Rectangle) {.x = win_width / 2 - snake_apothem, .y = win_height / 2 - snake_apothem, .width = 2*snake_apothem, .height = 2*snake_apothem};
		++field->snake_len;
		return;
	} else if (field->snake_len == 1) {
		float dx = 0, dy = 0;

		switch(field->snakedir) {
		case UP:
			--dy;
			break;
		case DOWN:
			++dy;
			break;
		case LEFT:
			--dx;
			break;
		case RIGHT:
			++dx;
			break;
		default:
			assert(0 && "unhandled invalid MovementDirection");
		}

		// NOTE: Inverting dx, dy!!!
		Rectangle tail = field->snake[0];
		tail.x = tail.x - dx * snake_apothem * 2;
		tail.y = tail.y - dy * snake_apothem * 2;

		field->snake[1] = tail;
		++field->snake_len;
		return;
	}

	Rectangle *last = &field->snake[field->snake_len-1];
	Rectangle *second2last = &field->snake[field->snake_len-2];
	
	float dx = last->x - second2last->x;
	float dy = last->y - second2last->y;

	Rectangle new = *last;
	new.x += dx;
	new.y += dy;

	field->snake[field->snake_len] = new;
	++field->snake_len;
}


void init_playing_field(PlayingField *field)  {
	field->score = 0;
	field->game_over = false;
	field->game_paused = true;

	field->nrows = 40;
	field->ncols = 40;

	field->pxlen_x = (win_width - 2*border_px) / field->ncols;
	field->pxlen_y = (win_height - 2*border_px) / field->nrows;

	field->borderrect = (Rectangle) { .x = border_px - 1, .y = border_px - 1, .width = win_width - 2*border_px, .height = win_height - 2*border_px};
	field->snakedir = rand() % 4 + 1;
	
	field->snake_cap = INIT_BUF_SZ;
	field->snake_len = 0;
	field->snake = calloc(INIT_BUF_SZ, sizeof(Rectangle));
	for (int i = 0; i < INIT_SNAKE_LEN; ++i)
		grow_snake(field);

	choose_next_snack_pos(field);
}

void move_snake_head_forward(PlayingField *field) {
	assert(field->snake_len && "snake inexistent");

	float dx = 0, dy = 0;
	switch(field->snakedir) {
	case UP:
		--dy;
		break;
	case DOWN:
		++dy;
		break;
	case LEFT:
		--dx;
		break;
	case RIGHT:
		++dx;
		break;
	case NONE: // fallthrough
	default:
		assert(0 && "unhandled move_dir");
	}

	Rectangle *head = &field->snake[0];
	head->x += dx * snake_apothem * 2;
	head->y += dy * snake_apothem * 2;

	if (fabs(field->borderrect.x - head->x) < snake_apothem
			|| fabs(field->borderrect.x + field->borderrect.width - head->x) < snack_apothem
			|| fabs(field->borderrect.y - head->y) < snack_apothem
			|| fabs(field->borderrect.y + field->borderrect.height - head->y) < snack_apothem) {
		field->game_over = true;
	}
}

void move_snake(PlayingField *field, enum MovementDirection head_dir) {
	assert(field->snake_len >= INIT_SNAKE_LEN && "snake_len < INIT_SNAKE_LEN");

	if (field->game_over) return;

	for (int i = field->snake_len - 1; i >= 0; --i) {
		if (i == 0) {
			field->snakedir = head_dir;
			move_snake_head_forward(field);
			continue;
		}

		assert(i > 0 && "not the tail");
		field->snake[i] = field->snake[i-1];
	}

	for (size_t i = 1; i < field->snake_len; ++i) {
		if (CheckCollisionRecs(field->snake[0], field->snake[i])) {
			field->game_over = true;
		}
	}
}

void eat_snack(PlayingField *field) {
	grow_snake(field);
	choose_next_snack_pos(field);
	++field->score;
}

void draw_playing_field(PlayingField *field) {
	Color bgcolor = GetColor(0xd8f79aff);
	Color snackcolor = PINK;
	Color snakecolor = field->game_over ? RED : GetColor(0x836953ff);

	ClearBackground(RAYWHITE);
	DrawRectangleRec(field->borderrect, bgcolor);
	DrawRectangleLinesEx(field->borderrect, 1, BLACK);

	DrawRectangleRec(field->snack, snackcolor);
	
	for (size_t i = 0; i < field->snake_len; ++i) {
		DrawRectangleRec(field->snake[i], snakecolor);
	}

	char buf[200];
	snprintf(buf, sizeof(buf), "score: %u                                                                press space to pause", field->score);
	DrawText(buf, border_px, win_height - 2*border_px / 3, 25, ORANGE);
}

int main(int argc, char **argv) {
	srand(time(NULL));

	(void) argc; (void) argv;
	PlayingField field;
	init_playing_field(&field);

	InitWindow(win_width, win_height, "Snake");
	SetTargetFPS(30);

	while (!WindowShouldClose()) {
		BeginDrawing();
		draw_playing_field(&field);
		EndDrawing();

		if (field.game_over) continue;

		if (IsKeyPressed(KEY_A)) {
			if (field.snakedir != RIGHT) field.snakedir = LEFT;	
			field.game_paused = false;
		} else if (IsKeyPressed(KEY_D)) {
			if (field.snakedir != LEFT) field.snakedir = RIGHT;	
			field.game_paused = false;
		} else if (IsKeyPressed(KEY_W)) {
		 	if (field.snakedir != DOWN) field.snakedir = UP;  
			field.game_paused = false;
		} else if (IsKeyPressed(KEY_S)) {
			if (field.snakedir != UP) field.snakedir = DOWN;
			field.game_paused = false;
		} else if (IsKeyPressed(KEY_SPACE)) {
			field.game_paused = !field.game_paused;
		}

		if (!field.game_paused) {
			move_snake(&field, field.snakedir);
			if (CheckCollisionRecs(field.snake[0], field.snack)) {
				eat_snack(&field);
			}
		}
	}

	if (field.snake) free(field.snake);
	return 0;
}

