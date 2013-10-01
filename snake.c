#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_mixer.h"
#include <stdlib.h>
#include <error.h>
#include "constants.h"
#include <string.h>
#include <time.h>

typedef struct _snake_body {
    SDL_Rect rect;
    struct _snake_body *next;
    int digesting;
    int position;
    int current_pos;
} SnakeBody;

typedef struct _snake {
    SDL_Rect rect;
    SnakeBody *next;
    SnakeBody *tail;
    int size;
} Snake;

typedef struct eat_que {
    SnakeBody *sb;
    struct eat_que *next;
    struct eat_que *tail;
} EatQue;

typedef struct queue {
    EatQue *tail;
    EatQue *head;
} Queue;


SDL_Surface *screen = NULL;
Uint32 move_timer = 0;
TTF_Font *font = NULL;
SDL_Color txt_color = {255, 255, 255};


/**
 * This function is used to initiate different SDL stuff like SDL itself, the mixer, and
 * and TTF.
 *
 * @return The function returns 1 if successful, and 0 if not.
 */
int init() {
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        fprintf(stderr, "SDL_Init() failed.\n");

        return 0;
    }

    screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE);
    if (screen == NULL) {
        fprintf(stderr, "SDL_SetVideoMode Failed.\n");

        return 0;
    }

    if (TTF_Init() == -1) {
        printf("TTF_Init() failed\n");

        return 0;
    }

    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1) {
        printf("Mix_OpenAudio() failed.\n");

        return 0;
    }

    SDL_WM_SetCaption("snake.c", NULL);

    srand(time(NULL));

    return 1;
}


/**
 * This function is used to cleanup different SDL related stuff.
 */
void cleanup() {
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
}


void draw_snake(Snake snake) {
    SnakeBody *sb = NULL;
    sb = snake.next;

    //Lime green aka hacker green.
    SDL_FillRect(screen, &snake.rect, 0x32CD32);

    while (sb != NULL) {
        //printf("SB x:%d, y:%d, snake x:%d, y:%d\n", sb->rect.x, sb->rect.y, snake.rect.x,
        //        snake.rect.y);
        SDL_FillRect(screen, &sb->rect, 0x32CD32);
        sb = sb->next;
    }
}


/**
 * This function moves the snake. It also takes care of the digestion of food,
 * the food has to pass through the snake to reach the end of the snake before 
 * the snake grows. To do this a queue of food is created, and each time the 
 * snake moves whatever food is in the queue has it current position incremented
 * When the food's current position is the same as it position in the snake it 
 * has reached the end of the snake, and is the added to the snake.
 * 
 * The function also does collision detection to see if the snake is eating it
 * self. It also checks if the snake has moved outside of the screen.
 * 
 * @param snake Pointer to the snake structure,
 * @param direction The direction the snake is moving.
 * @param score The current player score.
 * @param queue Pointer to the food queue.
 * @return Returns 1 if everything is OKAY, returns 0 if the snake is eating 
 * itself, or has moved outside the screen.
 */
int move_snake(Snake *snake, int direction, int score, Queue *queue) {
    SnakeBody *sb = NULL;
    Uint32 time = SDL_GetTicks();
    time = time - move_timer;
        
    // The snake only moves if 250ms has passed since the last time, minus the
    // the current score.
    if (time >= (250 - score)) {
        if(snake->next != NULL) {
            
        }
        int old_x = snake->rect.x;
        int old_y = snake->rect.y;
        
        if (direction == DOWN) {
            snake->rect.y = snake->rect.y + SNAKE_HEIGHT;
        }

        else if (direction == UP) {
            snake->rect.y = snake->rect.y - SNAKE_HEIGHT;
        }

        else if (direction == LEFT) {
            snake->rect.x = snake->rect.x - SNAKE_WIDTH;
        }

        else if (direction == RIGHT) {
            snake->rect.x = snake->rect.x + SNAKE_WIDTH;
        }
        
        sb = snake->next;
        
        // Iterate over the snake, and move each body part.
        while(sb != NULL) {
            int tmp_x = sb->rect.x;
            int tmp_y = sb->rect.y;
            
            sb->rect.x = old_x;
            sb->rect.y = old_y;
            
            old_x = tmp_x;
            old_y = tmp_y;
            
            if(snake->rect.x == old_x && snake->rect.y == old_y) {
                return 0;
            }
            
            sb = sb->next;
        }
        
        // Here the food queue is checked.
        if (queue->head != NULL) {
            EatQue *eq = queue->head;
            
            eq->sb->current_pos += 1;
            
            if(eq->sb->current_pos == eq->sb->position) {
                printf("Body part has reached and of line\n");
                
                if (snake->next == NULL) {
                    snake->next = eq->sb;
                    snake->tail = eq->sb;
                    snake->size += 1;
                }
                
                else {
                    snake->tail->next = eq->sb;
                    snake->tail = eq->sb;
                    snake->size += 1;
                }
                
                if (queue->head != queue->tail) {
                    queue->head = queue->head->next;
                }
                
                else {
                    printf("Nothing on the queue, setting both pointer to Null\n");
                    queue->head = NULL;
                    queue->tail = NULL;
                }
                
                free(eq);
            }
        }
        
        // Check if the snake has gone over the edge.
        if (snake->rect.x < 0 || (snake->rect.x + SNAKE_WIDTH) > SCREEN_WIDTH) {
            return 0;
        }
        
        if (snake->rect.y < BORDER_LINE ||
                (snake->rect.y + SNAKE_HEIGHT) > SCREEN_HEIGHT) {
            return 0;
        }

        move_timer = SDL_GetTicks();
    }

    return 1;
}

/**
 * This function adds a new body part, a SnakeBody element is allocated, and is
 * stored as a pointer in a new element on the food queue.
 * @param x X cord of where the food was eaten.
 * @param y Y cord of the same.
 * @param snake Pointer to the snake.
 * @param que Pointer to the food queue.
 * @return  nada.
 */
SnakeBody *add_body_part(int x, int y, Snake *snake, Queue *que) {
    SnakeBody *sb = (SnakeBody *) calloc(1, sizeof(SnakeBody));
    EatQue *eq = (EatQue *) calloc(1, sizeof(EatQue));
    
    sb->rect.x = x;
    sb->rect.y = y;
    sb->rect.w = SNAKE_WIDTH;
    sb->rect.h = SNAKE_HEIGHT;
    sb->digesting = 1;
    sb->position = snake->size;
    sb->current_pos = 0;
    
    eq->sb = sb;
    
    if (que->head == NULL) {
        que->head = eq;
        que->tail = eq;
    }
    
    else {
        que->tail->next = eq;
        que->tail = eq;
    }
}


/**
 * Draws a surface to the screen.
 * @param x The x cord of where on the screen you want it drawn.
 * @param y The y cord of the same.
 * @param source The surface you want to draw.
 * @param destination The surface you want the source to be drawn on.
 */
void apply_surface(int x, int y, SDL_Surface *source, SDL_Surface *destination) {
    SDL_Rect offset;

    offset.x = x;
    offset.y = y;

    SDL_BlitSurface(source, NULL, destination, &offset);
}

/**
 * Generates a new food piece in a random location.
 * @param food Pointer to the food.
 */
void random_food(SDL_Rect *food) {
    int x, y, c = 1;

    while (c) {
        x = rand() % (SCREEN_WIDTH - SNAKE_WIDTH);

        if ((x % 10) == 0) {
            c = 0;
        }
    }

    c = 1;

    while (c) {
        y = rand() % (SCREEN_HEIGHT - BORDER_LINE - 1 - SNAKE_HEIGHT);

        if ((y % 10) == 0) {
            c = 0;
        }
    }

    y = y + BORDER_LINE + 1;

    food->x = x;
    food->y = y;
}


/**
 * Creates the death screen a displays a game over message with the score.
 * The function will listen until the return key is pressed or a quit event.
 * If return is pressed the function returns, if quit is pressed the program
 * exits.
 * 
 * The function also cleans up the snake, and the food queue freeing anything in
 * it.
 * @param score_msg The score message to be displayed.
 * @param background The background to be used.
 * @param snake Pointer to the snake.
 * @param queue Pointer to the food queue.
 */
void death_screen(SDL_Surface *score_msg, SDL_Rect background, Snake *snake, Queue *queue) {
    SDL_Surface *game_over = NULL;
    SDL_Surface *cont = NULL;
    SnakeBody *sb = NULL;
    EatQue *eq = NULL;
    
    SDL_Event event;
    
    game_over =TTF_RenderText_Solid(font, "GAME OVER", txt_color);
    cont = TTF_RenderText_Solid(font, "PRESS ENTER TO CONTINUE", txt_color);
    
    SDL_FillRect(screen, &background, 0x000000);
    apply_surface(5, 5, game_over, screen);
    apply_surface(50, 50, cont, screen);
    apply_surface(100, 100, score_msg, screen);
    
    //Free the snake and eating queue.
    eq = queue->head;
    while (eq != NULL) {
        EatQue *tmp = eq->next;
        free(eq->sb);
        free(eq);
        eq = tmp;
    }
    
    // Set pointers back to NULL
    queue->head = NULL;
    queue->tail = NULL;
    
    sb = snake->next;
    while (sb != NULL) {
        SnakeBody *tmp = sb->next;
        free(sb);
        sb = tmp;
    }
    
    // Dangling pointers again.
    snake->next = NULL;
    snake->tail = NULL;
    snake->size = 1;
    
    // Show post game message.
    if (SDL_Flip(screen) == -1) {
        exit(0);
    }
    
    // Wait for return button pressed, or exit.
    while(1) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            }

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_RETURN: return; break;
                }
            }
        }
    }
}


int main(int argc, char *args[]) {
    int score = 0;
    int quit = 0;
    int direction = DOWN;
    char *score_str = "Score: ";
    char score_n_str[100] = {0};
    char joined_score[110] = {0};
    SDL_Surface *score_msg = NULL;
    SDL_Event event;
    SDL_Color text_color = {255, 255, 255};
    Snake snake;
    SDL_Rect background, border, food, game_over;
    Queue queue;
    Mix_Chunk *beep = NULL, *game_over_sound = NULL;
    
    if (!init()) {
        return 1;
    }
    
    
    queue.head = NULL;
    
    background.x = 0;
    background.y = 0;
    background.w = SCREEN_WIDTH;
    background.h = SCREEN_HEIGHT;

    border.x = 0;
    border.y = BORDER_LINE;
    border.w = SCREEN_WIDTH;
    border.h = 1;

    snake.rect.x = START_X;
    snake.rect.y = START_Y;
    snake.rect.w = SNAKE_WIDTH;
    snake.rect.h = SNAKE_HEIGHT;
    snake.next = NULL;
    snake.size = 1;

    food.w = SNAKE_WIDTH;
    food.h = SNAKE_HEIGHT;
    
    

    random_food(&food);

    
    
    beep = Mix_LoadWAV("beep.wav");
    game_over_sound = Mix_LoadWAV("game_over.wav");
    if (beep == NULL || game_over_sound == NULL) {
        printf("What? An error? Really? Here it is: %s", Mix_GetError());
    }

    font = TTF_OpenFont("lazy.ttf", 26);

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP: direction = UP; break;
                    case SDLK_DOWN: direction = DOWN; break;
                    case SDLK_LEFT: direction = LEFT; break;
                    case SDLK_RIGHT: direction = RIGHT; break;
                }
            }
        }

        //Paint background black.
        SDL_FillRect(screen, &background, 0x000000);
        SDL_FillRect(screen, &border, 0xFFFFFF);
        SDL_FillRect(screen, &food, 0xFF0000);

        sprintf(score_n_str, "%d", score);
        strncpy(joined_score, score_str, sizeof(joined_score));
        strcat(joined_score, score_n_str);
        score_msg = TTF_RenderText_Solid(font, joined_score, txt_color);
        apply_surface(5, 2, score_msg, screen);

        if (!move_snake(&snake, direction, score, &queue)) {
            printf("game over, score %d\n", score);
            if(Mix_PlayChannel( -1, game_over_sound, 0 ) == -1) {
                return 1;    
            }
            death_screen(score_msg, background, &snake, &queue);
            
            snake.rect.x = START_X;
            snake.rect.y = START_Y;
            
            score = 0;
            
            continue;
        }

        draw_snake(snake);

        if(snake.rect.x == food.x && snake.rect.y == food.y) {
            SnakeBody *sb = NULL;
            score += 10;
            sb = add_body_part(food.x, food.y, &snake, &queue);
            
            if(Mix_PlayChannel( -1, beep, 0 ) == -1) {
                return 1;    
            }
            
            random_food(&food);
        }

        if (SDL_Flip(screen) == -1) {
            return 1;
        }
    }

    cleanup();

    return 0;
}
