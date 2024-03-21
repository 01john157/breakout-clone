#include <config_parser.hpp>
#include <sdl2/sdl.h>
#include <sdl2/sdl_mixer.h>
#include <sdl2/sdl_image.h>
#include <iostream>
#include <array>
#include <random>

void draw_number(int num, int x_offset, int y_offset);
void draw_0(int x_offset, int y_offset);
void draw_1(int x_offset, int y_offset);
void draw_2(int x_offset, int y_offset);
void draw_3(int x_offset, int y_offset);
void draw_4(int x_offset, int y_offset);
void draw_5(int x_offset, int y_offset);
void draw_6(int x_offset, int y_offset);
void draw_7(int x_offset, int y_offset);
void draw_8(int x_offset, int y_offset);
void draw_9(int x_offset, int y_offset);

SDL_Renderer* renderer;
std::mt19937 rng {std::random_device{}()};

constexpr int BORDER_HEIGHT = 23;

int main(int argc, char* args[])
{
    /* initialisation */
    constexpr int RENDER_WIDTH = 480;
    constexpr int RENDER_HEIGHT = 628;

    std::unordered_map<std::string, float>* config = new std::unordered_map<std::string, float>;
    *config = parse_config_file("./data/config.txt");

    const float MAX_FPS = config->at("MAX_FPS");

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
    IMG_Init(IMG_INIT_PNG);

    /* window initialisation */
    bool fullscreen;
    Uint32 fullscreen_mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
    Uint32 window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
    switch(static_cast<int>(config->at("WINDOW_MODE")))
    {
        case 2: fullscreen_mode = SDL_WINDOW_FULLSCREEN; window_flags += fullscreen_mode; fullscreen = true; break;
        case 1: window_flags += fullscreen_mode; fullscreen = true; break;
        case 0: fullscreen = false; break;
    }
    SDL_Window* window = SDL_CreateWindow("breakout-clone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, static_cast<int>(config->at("WINDOW_RESOLUTION_X")), static_cast<int>(config->at("WINDOW_RESOLUTION_Y")), window_flags);
    SDL_SetWindowIcon(window, IMG_Load("./assets/window_icon.png"));
    int window_w {};
    int window_h {};
    /* window initialisation */ 

    /* renderer initialisation */
    Uint32 renderer_flags = 0;
    if(static_cast<bool>(config->at("VSYNC")))
    {
        renderer_flags += SDL_RENDERER_PRESENTVSYNC;
    }
    renderer = SDL_CreateRenderer(window, -1, renderer_flags);
    SDL_RenderSetLogicalSize(renderer, RENDER_WIDTH, RENDER_HEIGHT);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderSetIntegerScale(renderer, static_cast<SDL_bool>(config->at("SCALING_MODE")));
    SDL_ShowCursor(SDL_DISABLE);
    /* renderer initialisation */

    /* audio initialisation */
    if(SDL_GetNumAudioDevices(0) != 0)
    {
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    }
    Mix_Chunk* ball_hit_brick_1 = Mix_LoadWAV("./assets/audio/ball_hit_brick_1.mp3");
    Mix_Chunk* ball_hit_brick_2 = Mix_LoadWAV("./assets/audio/ball_hit_brick_2.mp3");
    Mix_Chunk* ball_hit_brick_3 = Mix_LoadWAV("./assets/audio/ball_hit_brick_3.mp3");
    Mix_Chunk* ball_hit_brick_4 = Mix_LoadWAV("./assets/audio/ball_hit_brick_4.mp3");
    Mix_Chunk* ball_hit_paddle = Mix_LoadWAV("./assets/audio/ball_hit_paddle.mp3");
    Mix_Chunk* ball_hit_wall = Mix_LoadWAV("./assets/audio/ball_hit_wall.mp3");
    /* audio initialisation */

    /* input initialisation */
    SDL_GameController* controller = SDL_GameControllerOpen(0);
    constexpr int STICK_DEADZONE = 6000;
    /* input initialisation */

    float dT = 0.0013888889f;
    float accumulator = 0.0f;

    delete config;
    config = nullptr;
    /* initialisation */

    constexpr int BORDER_WIDTH = 8;

    struct Paddle {
        float x = RENDER_WIDTH / 2 - (30) / 2;
        const float y = 547;
        float w = 30;
        const float h = 11;
        const float velocity = RENDER_WIDTH * 2;
    } paddle;

    enum Vertical_Direction {
        UP = -1,
        DOWN = 1
    };

    enum Horizontal_Direction {
        LEFT = -1,
        RIGHT = 1
    };

    struct Ball {
        float x = RENDER_WIDTH / 2;
        float y = RENDER_HEIGHT / 2;
        const float w = 6;
        const float h = 6;
        Horizontal_Direction hor = std::uniform_int_distribution{0, 1}(rng) ? LEFT : RIGHT;
        Vertical_Direction ver = std::uniform_int_distribution{0, 1}(rng) ? UP : DOWN;
        float velocity = RENDER_WIDTH * 0.8;
        float angle = 1;
    } ball;

    const float MIN_ANGLE = (0.4f);

    struct Colour {
        const Uint8 r;
        const Uint8 g;
        const Uint8 b;
    };
    const Uint8 a = 255;

    const int brick_w = 29;
    const int brick_h = 8;
    struct Row_of_Bricks {
        const int y;
        const Colour colour;
        const int score_given;
        const int audio_play_count;
        std::vector<int> x = {0, 34, 68, 102, 136, 170, 204, 238, 272, 306, 340, 374, 408, 443};
    };
    std::vector<Row_of_Bricks> bricks {
        {117, {163, 30, 10}, 7, 4},
        {128, {163, 30, 10}, 7, 4},
        {141, {194, 133, 10}, 5, 3},
        {152, {194, 133, 10}, 5, 3},
        {165, {10, 133, 51}, 3, 2},
        {176, {10, 133, 51}, 3, 2},
        {189, {194, 194, 41}, 1, 1},
        {200, {194, 194, 41}, 1, 1}
    };

    int score = 0;
    int lives_used = 0;
    int bricks_hit = 0;
    bool hit_orange = false;
    bool hit_red = false;

    std::array<int, 3> score_digit_positions {101, 67, 33};

    Uint64 previous_time = SDL_GetPerformanceCounter();

    bool running = true;
    while(running)
    {
        Uint64 frame_start_time = SDL_GetPerformanceCounter();

        /* input handling */
        SDL_Event event;
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        while(SDL_PollEvent(&event) != 0){
            if(event.type == SDL_QUIT)
            {
                running = false;
            }
            if(event.type == SDL_KEYDOWN)
            {
                if(keys[SDL_SCANCODE_F11])
                {
                    if(fullscreen)
                    {
                        SDL_SetWindowFullscreen(window, 0);
                        fullscreen = false;
                    }
                    else
                    {
                        SDL_SetWindowFullscreen(window, fullscreen_mode);
                        fullscreen = true;
                    }
                }
                if(keys[SDL_SCANCODE_F5])
                {
                    ball.x = RENDER_WIDTH / 2;
                    ball.y = RENDER_HEIGHT / 2;
                    ball.hor = std::uniform_int_distribution{0, 1}(rng) ? LEFT : RIGHT;
                    ball.ver = DOWN;
                    ball.velocity = RENDER_WIDTH * 0.8;
                    ball.angle = 1;
                    bricks_hit = 0;
                    hit_orange = false;
                    hit_red = false;
                    lives_used = 0;
                    score = 0;
                    bricks.clear();
                    bricks = std::vector<Row_of_Bricks>{
                        {117, {163, 30, 10}, 7, 4},
                        {128, {163, 30, 10}, 7, 4},
                        {141, {194, 133, 10}, 5, 3},
                        {152, {194, 133, 10}, 5, 3},
                        {165, {10, 133, 51}, 3, 2},
                        {176, {10, 133, 51}, 3, 2},
                        {189, {194, 194, 41}, 1, 1},
                        {200, {194, 194, 41}, 1, 1}
                    };
                }
            }
        }

        // controller
        float stick_angle = 0.0f;
        if((SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX)*-1) > STICK_DEADZONE || SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) > STICK_DEADZONE)
        {
            stick_angle = static_cast<float>(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) / 32768.0f);
        }
        /* input handling */

        /* logic */
        Uint64 current_time = SDL_GetPerformanceCounter();
        accumulator += static_cast<float>(current_time - previous_time) / static_cast<float>(SDL_GetPerformanceFrequency());
        previous_time = current_time;

        // paddle
        while(accumulator >= dT)
        {
            if(lives_used == 3)
            {
                ball.x = RENDER_WIDTH / 2;
                ball.y = RENDER_HEIGHT / 2;
                ball.hor = std::uniform_int_distribution{0, 1}(rng) ? LEFT : RIGHT;
                ball.ver = DOWN;
                ball.velocity = RENDER_WIDTH * 0.8;
                ball.angle = 1;
                bricks_hit = 0;
                hit_orange = false;
                hit_red = false;
                lives_used = 0;
                score = 0;
                bricks.clear();
                bricks = std::vector<Row_of_Bricks>{
                    {117, {163, 30, 10}, 7, 4},
                    {128, {163, 30, 10}, 7, 4},
                    {141, {194, 133, 10}, 5, 3},
                    {152, {194, 133, 10}, 5, 3},
                    {165, {10, 133, 51}, 3, 2},
                    {176, {10, 133, 51}, 3, 2},
                    {189, {194, 194, 41}, 1, 1},
                    {200, {194, 194, 41}, 1, 1}
                };
            }

            if(stick_angle != 0)
            {
                float distance_to_move = (paddle.velocity * stick_angle) * dT;
                if(paddle.x + distance_to_move <= BORDER_WIDTH)
                {
                    paddle.x = BORDER_WIDTH;
                }
                else if((paddle.x + paddle.w-1) + distance_to_move >= RENDER_WIDTH-BORDER_WIDTH)
                {
                    paddle.x = RENDER_WIDTH-BORDER_WIDTH - paddle.w;
                }
                else
                {
                    paddle.x += distance_to_move;
                }
            }

            // ball
            if(ball.y <= BORDER_HEIGHT+1)
            {
                paddle.w = 15;
            }
            if(ball.x < BORDER_WIDTH)
            {
                Mix_PlayChannel(-1, ball_hit_wall, 0);
                ball.hor = RIGHT;
            }
            else if((ball.x + ball.w-1) > RENDER_WIDTH-BORDER_WIDTH)
            {
                Mix_PlayChannel(-1, ball_hit_wall, 0);
                ball.hor = LEFT;
            }

            if(ball.y < BORDER_HEIGHT)
            {
                Mix_PlayChannel(-1, ball_hit_wall, 0);
                ball.ver = DOWN;
            }
            else if((ball.y + ball.h-1) > RENDER_HEIGHT)
            {
                ball.ver = UP;
                ball.x = static_cast<float>(std::uniform_int_distribution{BORDER_WIDTH, RENDER_WIDTH-BORDER_WIDTH}(rng));
                ball.y = RENDER_HEIGHT / 2;
                ball.hor = std::uniform_int_distribution{0, 1}(rng) ? LEFT : RIGHT;
                ball.ver = DOWN;
                ball.velocity = RENDER_WIDTH * 0.8;
                ball.angle = 1;
                bricks_hit = 0;
                hit_orange = false;
                hit_red = false;
                lives_used++;
            }
            else if((ball.y + ball.h >= paddle.y && paddle.y + paddle.h > ball.y) && (ball.x + ball.w >= paddle.x && paddle.x + paddle.w > ball.x))
            {
                Mix_PlayChannel(-1, ball_hit_paddle, 0);
                ball.angle = -((paddle.x + ((paddle.w-1)/2)) - ball.x) / ((paddle.w-1)/2);
                if(ball.angle < 0)
                {
                    ball.hor = LEFT;
                    ball.angle = 1 - -ball.angle;
                }
                else
                {
                    ball.hor = RIGHT;
                    ball.angle = 1 - ball.angle;
                }
                if(ball.angle < MIN_ANGLE)
                {
                    ball.angle = MIN_ANGLE;
                }
                ball.ver = UP;
            }
            else
            {
                for(size_t row = 0; row < bricks.size(); row++)
                {
                    if(ball.y + ball.h >= static_cast<float>(bricks[row].y) && static_cast<float>(bricks[row].y + brick_h) > ball.y)
                    {
                        for(size_t brick = 0; brick < bricks[row].x.size(); brick++)
                        {
                            if(ball.x + ball.w >= static_cast<float>(bricks[row].x[brick]) && static_cast<float>(bricks[row].x[brick] + brick_w) > ball.x)
                            {
                                switch(bricks[row].audio_play_count)
                                {
                                    case 1: Mix_PlayChannel(-1, ball_hit_brick_1, 0); break;
                                    case 2: Mix_PlayChannel(-1, ball_hit_brick_2, 0); break;
                                    case 3: Mix_PlayChannel(-1, ball_hit_brick_3, 0); break;
                                    case 4: Mix_PlayChannel(-1, ball_hit_brick_4, 0); break;
                                }
                                bricks_hit++;
                                if(bricks_hit == 4 || bricks_hit == 12)
                                {
                                    ball.velocity += 24;
                                }
                                if(!hit_orange)
                                {
                                    if(bricks[row].colour.b == 10)
                                    {
                                        hit_orange = true;
                                        ball.velocity += 24;
                                    }
                                }
                                if(!hit_red)
                                {
                                    if(bricks[row].colour.b == 10)
                                    {
                                        hit_red = true;
                                        ball.velocity += 24;
                                    }
                                }

                                score += bricks[row].score_given;

                                std::array<float, 4> distances {
                                    (ball.y + ball.h) - static_cast<float>(bricks[row].y + brick_h),
                                    static_cast<float>(bricks[row].y) - ball.y,
                                    static_cast<float>(bricks[row].x[brick]) - ball.x,
                                    (ball.x + ball.w) - static_cast<float>(bricks[row].x[brick] + brick_w)
                                };
                                float largest_distance = 0;
                                for(size_t i = 0; i < distances.size(); i++)
                                {
                                    if(distances[i] > largest_distance)
                                    {
                                        largest_distance = distances[i];
                                    }
                                }
                                if(distances[0] == largest_distance)
                                {
                                    ball.ver = DOWN;
                                }
                                if(distances[1] == largest_distance)
                                {
                                    ball.ver = UP;
                                }
                                if(distances[2] == largest_distance)
                                {
                                    ball.hor = LEFT;
                                }
                                if(distances[3] == largest_distance)
                                {
                                    ball.hor = RIGHT;
                                }

                                bricks[row].x.erase(bricks[row].x.begin() + static_cast<int>(brick));
                            }
                        }
                    }
                }
            }

            ball.x += static_cast<float>(ball.hor) * (ball.velocity * std::cos(ball.angle)) * dT;
            ball.y += static_cast<float>(ball.ver) * (ball.velocity * std::sin(ball.angle)) * dT;

            accumulator -= dT;
        }
        /* logic */

        /* rendering */
        float interpolation = accumulator / dT;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // border
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect top_border = {0, 0, RENDER_WIDTH, BORDER_HEIGHT};
        SDL_Rect left_border = {0, BORDER_HEIGHT, BORDER_WIDTH, RENDER_HEIGHT-BORDER_HEIGHT};
        SDL_Rect right_border = {RENDER_WIDTH-BORDER_WIDTH, BORDER_HEIGHT, BORDER_WIDTH, RENDER_HEIGHT-BORDER_HEIGHT};
        SDL_RenderDrawRect(renderer, &top_border);
        SDL_RenderDrawRect(renderer, &left_border);
        SDL_RenderDrawRect(renderer, &right_border);
        SDL_RenderFillRect(renderer, &top_border);
        SDL_RenderFillRect(renderer, &left_border);
        SDL_RenderFillRect(renderer, &right_border);

        SDL_SetRenderDrawColor(renderer, 163, 30, 10, 255);
        SDL_Rect left_border_red = {0, 115, BORDER_WIDTH, 24};
        SDL_Rect right_border_red = {RENDER_WIDTH-BORDER_WIDTH, 115, BORDER_WIDTH, 24};
        SDL_RenderDrawRect(renderer, &left_border_red);
        SDL_RenderDrawRect(renderer, &right_border_red);
        SDL_RenderFillRect(renderer, &left_border_red);
        SDL_RenderFillRect(renderer, &right_border_red);

        SDL_SetRenderDrawColor(renderer, 194, 133, 10, 255);
        SDL_Rect left_border_orange = {0, 139, BORDER_WIDTH, 24};
        SDL_Rect right_border_orange = {RENDER_WIDTH-BORDER_WIDTH, 139, BORDER_WIDTH, 24};
        SDL_RenderDrawRect(renderer, &left_border_orange);
        SDL_RenderDrawRect(renderer, &right_border_orange);
        SDL_RenderFillRect(renderer, &left_border_orange);
        SDL_RenderFillRect(renderer, &right_border_orange);

        SDL_SetRenderDrawColor(renderer, 10, 133, 51, 255);
        SDL_Rect left_border_green = {0, 163, BORDER_WIDTH, 24};
        SDL_Rect right_border_green = {RENDER_WIDTH-BORDER_WIDTH, 163, BORDER_WIDTH, 24};
        SDL_RenderDrawRect(renderer, &left_border_green);
        SDL_RenderDrawRect(renderer, &right_border_green);
        SDL_RenderFillRect(renderer, &left_border_green);
        SDL_RenderFillRect(renderer, &right_border_green);

        SDL_SetRenderDrawColor(renderer, 194, 194, 41, 255);
        SDL_Rect left_border_yellow = {0, 187, BORDER_WIDTH, 24};
        SDL_Rect right_border_yellow = {RENDER_WIDTH-BORDER_WIDTH, 187, BORDER_WIDTH, 24};
        SDL_RenderDrawRect(renderer, &left_border_yellow);
        SDL_RenderDrawRect(renderer, &right_border_yellow);
        SDL_RenderFillRect(renderer, &left_border_yellow);
        SDL_RenderFillRect(renderer, &right_border_yellow);

        SDL_SetRenderDrawColor(renderer, 10, 133, 194, 255);
        SDL_Rect left_border_blue = {0, 541, BORDER_WIDTH, 24};
        SDL_Rect right_border_blue = {RENDER_WIDTH-BORDER_WIDTH, 541, BORDER_WIDTH, 24};
        SDL_RenderDrawRect(renderer, &left_border_blue);
        SDL_RenderDrawRect(renderer, &right_border_blue);
        SDL_RenderFillRect(renderer, &left_border_blue);
        SDL_RenderFillRect(renderer, &right_border_blue);

        // player icon
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        draw_1(BORDER_WIDTH, BORDER_HEIGHT + 1);

        // score
        int tmp_score = score;
        for(int x_offset : score_digit_positions)
        {
            draw_number(tmp_score % 10, BORDER_WIDTH + x_offset, BORDER_HEIGHT + 46);
            tmp_score /= 10;
        }

        // lives used
        draw_number(lives_used+1, BORDER_WIDTH + 267, BORDER_HEIGHT + 1);

        // bricks
        for(size_t row = 0; row < bricks.size(); row++)
        {
            SDL_SetRenderDrawColor(renderer, bricks[row].colour.r, bricks[row].colour.g, bricks[row].colour.b, a);
            for(size_t brick = 0; brick < bricks[row].x.size(); brick++)
            {
                SDL_Rect brick_rect = {bricks[row].x[brick], bricks[row].y, brick_w, brick_h};
                SDL_RenderDrawRect(renderer, &brick_rect);
                SDL_RenderFillRect(renderer, &brick_rect);
            }
        }

        // paddle
        SDL_SetRenderDrawColor(renderer, 10, 133, 194, 255);
        SDL_FRect paddle_rect = {paddle.x, paddle.y, paddle.w, paddle.h};
        SDL_RenderDrawRectF(renderer, &paddle_rect);
        SDL_RenderFillRectF(renderer, &paddle_rect);

        // ball
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        float interpolated_ball_x = ball.x + (static_cast<float>(ball.hor) * (ball.velocity * std::cos(ball.angle)) * dT) * interpolation;
        float interpolated_ball_y = ball.y + (static_cast<float>(ball.ver) * (ball.velocity * std::sin(ball.angle)) * dT) * interpolation;
        SDL_FRect ball_rect = {interpolated_ball_x, interpolated_ball_y, ball.w, ball.h};
        SDL_RenderDrawRectF(renderer, &ball_rect);
        SDL_RenderFillRectF(renderer, &ball_rect);

        SDL_RenderPresent(renderer);
        /* rendering */

        Uint64 frame_end_time = SDL_GetPerformanceCounter();
        float frame_time = static_cast<float>(frame_end_time - frame_start_time) / static_cast<float>(SDL_GetPerformanceFrequency());
        float time_to_wait = ((1.0f / MAX_FPS) * 1000.0f) - frame_time;
        if(time_to_wait > 0)
        {
            SDL_Delay(static_cast<Uint32>(time_to_wait));
        }
    }

    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();

    return 0;
}

void draw_number(int num, int x_offset, int y_offset)
{
    switch(num)
    {
        case 0: draw_0(x_offset, y_offset); break;
        case 1: draw_1(x_offset, y_offset); break;
        case 2: draw_2(x_offset, y_offset); break;
        case 3: draw_3(x_offset, y_offset); break;
        case 4: draw_4(x_offset, y_offset); break;
        case 5: draw_5(x_offset, y_offset); break;
        case 6: draw_6(x_offset, y_offset); break;
        case 7: draw_7(x_offset, y_offset); break;
        case 8: draw_8(x_offset, y_offset); break;
        case 9: draw_9(x_offset, y_offset); break;
    }
}

void draw_0(int x_offset, int y_offset)
{
    SDL_Rect rect1 = {x_offset, y_offset, 25, 5};
    SDL_RenderDrawRect(renderer, &rect1);
    SDL_RenderFillRect(renderer, &rect1);
    SDL_Rect rect2 = {x_offset, y_offset + 5, 8, 28};
    SDL_RenderDrawRect(renderer, &rect2);
    SDL_RenderFillRect(renderer, &rect2);
    SDL_Rect rect3 = {x_offset + 17, y_offset + 5, 8, 28};
    SDL_RenderDrawRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect3);
    SDL_Rect rect4 = {x_offset, y_offset + 33, 25, 5};
    SDL_RenderDrawRect(renderer, &rect4);
    SDL_RenderFillRect(renderer, &rect4);
}

void draw_1(int x_offset, int y_offset)
{
    SDL_Rect rect3 = {x_offset + 17, y_offset, 8, 38};
    SDL_RenderDrawRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect3);
}

void draw_2(int x_offset, int y_offset)
{
    SDL_Rect rect1 = {x_offset, y_offset, 25, 5};
    SDL_RenderDrawRect(renderer, &rect1);
    SDL_RenderFillRect(renderer, &rect1);
    SDL_Rect rect2 = {x_offset, y_offset + 16, 8, 17};
    SDL_RenderDrawRect(renderer, &rect2);
    SDL_RenderFillRect(renderer, &rect2);
    SDL_Rect rect5 = {x_offset + 8, y_offset + 16, 9, 7};
    SDL_RenderDrawRect(renderer, &rect5);
    SDL_RenderFillRect(renderer, &rect5);
    SDL_Rect rect3 = {x_offset + 17, y_offset + 5, 8, 18};
    SDL_RenderDrawRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect3);
    SDL_Rect rect4 = {x_offset, y_offset + 33, 25, 5};
    SDL_RenderDrawRect(renderer, &rect4);
    SDL_RenderFillRect(renderer, &rect4);
}

void draw_3(int x_offset, int y_offset)
{
    SDL_Rect rect1 = {x_offset, y_offset, 25, 5};
    SDL_RenderDrawRect(renderer, &rect1);
    SDL_RenderFillRect(renderer, &rect1);
    SDL_Rect rect5 = {x_offset, y_offset + 16, 17, 7};
    SDL_RenderDrawRect(renderer, &rect5);
    SDL_RenderFillRect(renderer, &rect5);
    SDL_Rect rect3 = {x_offset + 17, y_offset + 5, 8, 28};
    SDL_RenderDrawRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect3);
    SDL_Rect rect4 = {x_offset, y_offset + 33, 25, 5};
    SDL_RenderDrawRect(renderer, &rect4);
    SDL_RenderFillRect(renderer, &rect4);
}

void draw_4(int x_offset, int y_offset)
{
    SDL_Rect rect2 = {x_offset, y_offset, 8, 23};
    SDL_RenderDrawRect(renderer, &rect2);
    SDL_RenderFillRect(renderer, &rect2);
    SDL_Rect rect5 = {x_offset + 8, y_offset + 16, 9, 7};
    SDL_RenderDrawRect(renderer, &rect5);
    SDL_RenderFillRect(renderer, &rect5);
    SDL_Rect rect3 = {x_offset + 17, y_offset, 8, 38};
    SDL_RenderDrawRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect3);
}

void draw_5(int x_offset, int y_offset)
{
    SDL_Rect rect1 = {x_offset, y_offset, 25, 5};
    SDL_RenderDrawRect(renderer, &rect1);
    SDL_RenderFillRect(renderer, &rect1);
    SDL_Rect rect2 = {x_offset, y_offset + 5, 8, 18};
    SDL_RenderDrawRect(renderer, &rect2);
    SDL_RenderFillRect(renderer, &rect2);
    SDL_Rect rect5 = {x_offset + 8, y_offset + 16, 9, 7};
    SDL_RenderDrawRect(renderer, &rect5);
    SDL_RenderFillRect(renderer, &rect5);
    SDL_Rect rect3 = {x_offset + 17, y_offset + 16, 8, 17};
    SDL_RenderDrawRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect3);
    SDL_Rect rect4 = {x_offset, y_offset + 33, 25, 5};
    SDL_RenderDrawRect(renderer, &rect4);
    SDL_RenderFillRect(renderer, &rect4);
}

void draw_6(int x_offset, int y_offset)
{
    SDL_Rect rect2 = {x_offset, y_offset, 8, 33};
    SDL_RenderDrawRect(renderer, &rect2);
    SDL_RenderFillRect(renderer, &rect2);
    SDL_Rect rect5 = {x_offset + 8, y_offset + 16, 9, 7};
    SDL_RenderDrawRect(renderer, &rect5);
    SDL_RenderFillRect(renderer, &rect5);
    SDL_Rect rect3 = {x_offset + 17, y_offset + 16, 8, 17};
    SDL_RenderDrawRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect3);
    SDL_Rect rect4 = {x_offset, y_offset + 33, 25, 5};
    SDL_RenderDrawRect(renderer, &rect4);
    SDL_RenderFillRect(renderer, &rect4);
}

void draw_7(int x_offset, int y_offset)
{
    SDL_Rect rect1 = {x_offset, y_offset, 25, 5};
    SDL_RenderDrawRect(renderer, &rect1);
    SDL_RenderFillRect(renderer, &rect1);
    SDL_Rect rect3 = {x_offset + 17, y_offset + 5, 8, 33};
    SDL_RenderDrawRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect3);
}

void draw_8(int x_offset, int y_offset)
{
    SDL_Rect rect1 = {x_offset, y_offset, 25, 5};
    SDL_RenderDrawRect(renderer, &rect1);
    SDL_RenderFillRect(renderer, &rect1);
    SDL_Rect rect2 = {x_offset, y_offset + 5, 8, 28};
    SDL_RenderDrawRect(renderer, &rect2);
    SDL_RenderFillRect(renderer, &rect2);
    SDL_Rect rect5 = {x_offset + 8, y_offset + 16, 9, 7};
    SDL_RenderDrawRect(renderer, &rect5);
    SDL_RenderFillRect(renderer, &rect5);
    SDL_Rect rect3 = {x_offset + 17, y_offset + 5, 8, 28};
    SDL_RenderDrawRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect3);
    SDL_Rect rect4 = {x_offset, y_offset + 33, 25, 5};
    SDL_RenderDrawRect(renderer, &rect4);
    SDL_RenderFillRect(renderer, &rect4);
}

void draw_9(int x_offset, int y_offset)
{
    SDL_Rect rect1 = {x_offset, y_offset, 25, 5};
    SDL_RenderDrawRect(renderer, &rect1);
    SDL_RenderFillRect(renderer, &rect1);
    SDL_Rect rect2 = {x_offset, y_offset + 5, 8, 18};
    SDL_RenderDrawRect(renderer, &rect2);
    SDL_RenderFillRect(renderer, &rect2);
    SDL_Rect rect5 = {x_offset + 8, y_offset + 16, 9, 7};
    SDL_RenderDrawRect(renderer, &rect5);
    SDL_RenderFillRect(renderer, &rect5);
    SDL_Rect rect3 = {x_offset + 17, y_offset + 5, 8, 33};
    SDL_RenderDrawRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect3);
}