#include <SDL.h>
#include <SDL_mixer.h> // For audio
#include <SDL_image.h> // For image loading
#include <SDL_ttf.h>   // For text rendering
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <vector> // For coins
#include <string> // For std::to_string

#include "coin.h" // Include your coin system header

// --- Constants ---
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

const int BALL_DIAMETER = 30;
const int BALL_RADIUS = BALL_DIAMETER / 2;
const float INITIAL_BALL_SPEED = 4.0f;
float current_ball_speed = INITIAL_BALL_SPEED;
const float BALL_BOOST_FACTOR = 1.2f;
const int BALL_BOOST_DURATION_FRAMES = 30;
int ball_boost_timer = 0;

const int PADDLE_WIDTH = 20;
const int PADDLE_HEIGHT = 100;
const float PADDLE_SPEED = 6.0f;

const float COIN_DRAW_SCALE = 0.8f; // Scale for drawing coins
const int COIN_APPEAR_INTERVAL_FRAMES = 300; // Roughly 5 seconds at 60 FPS
const int COIN_DURATION_FRAMES = 600; // Roughly 10 seconds at 60 FPS

// --- Game State Variables ---
float ball_x = WINDOW_WIDTH / 2.0f;
float ball_y = WINDOW_HEIGHT / 2.0f;
float ball_dx = 0.0f;
float ball_dy = 0.0f;

SDL_Rect leftPaddle = {
    0,
    (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2,
    PADDLE_WIDTH,
    PADDLE_HEIGHT
};

SDL_Rect rightPaddle = {
    WINDOW_WIDTH - PADDLE_WIDTH,
    (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2,
    PADDLE_WIDTH,
    PADDLE_HEIGHT
};

struct Coin {
    float x, y;
    int timer;
    // 'active' is now implicit: if it's in the vector, it's active.
    // If its timer runs out, it's removed from the vector.
};
std::vector<Coin> coins;
int coin_spawn_timer = 0;

// Score variables
int left_score = 0;
int right_score = 0;

// For scoring rule 1: consecutive hits
enum class LastHit { None, LeftPaddle, RightPaddle };
LastHit last_ball_hit = LastHit::None;
int left_consecutive_hits = 0;
int right_consecutive_hits = 0;

// --- Audio ---
Mix_Chunk* coin_sound = nullptr;

// --- Text Rendering ---
TTF_Font* gFont = nullptr;
SDL_Color textColor = { 255, 255, 255, 255 }; // White color for text

// --- Function Declarations ---
void drawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
bool checkCircleRectCollision(float circleX, float circleY, int circleRadius, const SDL_Rect& rect);
bool checkRectRectCollision(const SDL_Rect& rect1, const SDL_Rect& rect2);
void spawnCoin();
void playCoinSound();
void renderText(SDL_Renderer* renderer, const std::string& text, int x, int y, SDL_Color color);


// --- Function Definitions ---
void drawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    for (int y = -radius; y <= radius; y++) {
        int x = static_cast<int>(sqrt(static_cast<float>(radius * radius - y * y)));
        SDL_RenderDrawLine(renderer, centerX - x, centerY + y, centerX + x, centerY + y);
    }
}

bool checkCircleRectCollision(float circleX, float circleY, int circleRadius, const SDL_Rect& rect) {
    float closestX = std::max(static_cast<float>(rect.x), std::min(circleX, static_cast<float>(rect.x + rect.w)));
    float closestY = std::max(static_cast<float>(rect.y), std::min(circleY, static_cast<float>(rect.y + rect.h)));

    float distanceX = circleX - closestX;
    float distanceY = circleY - closestY;

    return (distanceX * distanceX + distanceY * distanceY) < (circleRadius * circleRadius);
}

bool checkRectRectCollision(const SDL_Rect& rect1, const SDL_Rect& rect2) {
    return SDL_HasIntersection(&rect1, &rect2);
}


void spawnCoin() {
    // Ensure coin is spawned within bounds and not too close to paddles
    // Coin size will depend on COIN_FRAME_WIDTH * COIN_DRAW_SCALE
    int coinEffectiveWidth = getCoinRenderedWidth(COIN_DRAW_SCALE);
    int coinEffectiveHeight = getCoinRenderedHeight(COIN_DRAW_SCALE);

    // Random X position: from PADDLE_WIDTH + coinEffectiveWidth/2 to WINDOW_WIDTH - PADDLE_WIDTH - coinEffectiveWidth/2
    float coin_x = static_cast<float>(rand() % (WINDOW_WIDTH - 2 * PADDLE_WIDTH - coinEffectiveWidth) + PADDLE_WIDTH + coinEffectiveWidth / 2);
    // Random Y position: from coinEffectiveHeight/2 to WINDOW_HEIGHT - coinEffectiveHeight/2
    float coin_y = static_cast<float>(rand() % (WINDOW_HEIGHT - coinEffectiveHeight) + coinEffectiveHeight / 2);

    coins.push_back({ coin_x, coin_y, COIN_DURATION_FRAMES });
}

void playCoinSound() {
    if (coin_sound) {
        Mix_PlayChannel(-1, coin_sound, 0); // Play on first available channel, once
    }
}

void renderText(SDL_Renderer* renderer, const std::string& text, int x, int y, SDL_Color color) {
    if (!gFont) {
        std::cerr << "Font not loaded! Cannot render text." << std::endl;
        return;
    }
    SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, text.c_str(), color);
    if (!textSurface) {
        std::cerr << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cerr << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
    }
    else {
        SDL_Rect renderQuad = { x, y, textSurface->w, textSurface->h };
        SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
    }
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}


// --- Main Function ---
int main(int argc, char* args[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_image (ensure PNG support for your coin images)
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_mixer for sound
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_ttf for text rendering (scores)
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Seed random number generator for coin spawning
    srand(static_cast<unsigned int>(time(0)));

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Pong Clone with Animated Coins",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        TTF_Quit();
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize the coin system (this will load coin_01.png to coin_08.png)
    if (!initCoinSystem(renderer)) {
        std::cerr << "Failed to initialize coin system. Exiting." << std::endl;
        // Perform comprehensive cleanup if coin system initialization fails
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load coin sound effect
    coin_sound = Mix_LoadWAV("coin_sound.mp3");
    if (coin_sound == nullptr) {
        std::cerr << "Failed to load coin_sound.mp3! SDL_mixer Error: " << Mix_GetError() << std::endl;
        // The game can continue without sound if it fails to load
    }

    // Load font for score display
    gFont = TTF_OpenFont("arial.ttf", 24); // Make sure "arial.ttf" is accessible, or use your own font file
    if (gFont == nullptr) {
        std::cerr << "Failed to load font! SDL_ttf Error: " << TTF_GetError() << std::endl;
        // The game can continue without text if font fails to load
    }


    bool quit = false;
    SDL_Event e;

    // Main game loop
    while (!quit) {
        // --- Event Handling ---
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                // Ball launch logic on click (only if the ball is currently stationary)
                if (ball_dx == 0.0f && ball_dy == 0.0f) {
                    int mouseX = e.button.x;
                    int mouseY = e.button.y;

                    // Check if the click was on the ball
                    float distance = sqrt(pow(mouseX - ball_x, 2) + pow(mouseY - ball_y, 2));

                    if (distance <= BALL_RADIUS) {
                        float angle;
                        do {
                            // Generate a random angle, ensuring it's not too flat or too steep (to keep gameplay engaging)
                            angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * M_PI;
                        } while (std::abs(std::sin(angle)) < 0.2f || std::abs(std::cos(angle)) < 0.2f); // Avoid angles too close to 0, 90, 180, 270 degrees

                        ball_dx = cos(angle);
                        ball_dy = sin(angle);

                        // Normalize the direction vector to maintain a consistent speed
                        float magnitude = sqrt(ball_dx * ball_dx + ball_dy * ball_dy);
                        if (magnitude != 0) {
                            ball_dx /= magnitude;
                            ball_dy /= magnitude;
                        }

                        current_ball_speed = INITIAL_BALL_SPEED; // Reset to initial speed on launch
                        ball_boost_timer = 0; // Clear any boost timer
                    }
                }
            }
        }

        // --- Paddle Movement ---
        const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

        // Left paddle movement (W, S keys)
        if (currentKeyStates[SDL_SCANCODE_W]) {
            leftPaddle.y -= PADDLE_SPEED;
        }
        if (currentKeyStates[SDL_SCANCODE_S]) {
            leftPaddle.y += PADDLE_SPEED;
        }

        // Right paddle movement (Up, Down arrow keys)
        if (currentKeyStates[SDL_SCANCODE_UP]) {
            rightPaddle.y -= PADDLE_SPEED;
        }
        if (currentKeyStates[SDL_SCANCODE_DOWN]) {
            rightPaddle.y += PADDLE_SPEED;
        }

        // Clamp paddles within vertical bounds of the window
        if (leftPaddle.y < 0) leftPaddle.y = 0;
        if (leftPaddle.y + PADDLE_HEIGHT > WINDOW_HEIGHT) leftPaddle.y = WINDOW_HEIGHT - PADDLE_HEIGHT;

        if (rightPaddle.y < 0) rightPaddle.y = 0;
        if (rightPaddle.y + PADDLE_HEIGHT > WINDOW_HEIGHT) rightPaddle.y = WINDOW_HEIGHT - PADDLE_HEIGHT;

        // --- Ball Movement ---
        ball_x += ball_dx * current_ball_speed;
        ball_y += ball_dy * current_ball_speed;

        bool reflected_this_frame = false;

        // --- Wall Collisions (top and bottom) ---
        if (ball_y + BALL_RADIUS > WINDOW_HEIGHT) {
            ball_y = WINDOW_HEIGHT - BALL_RADIUS; // Reposition to prevent sticking
            ball_dy = -std::abs(ball_dy); // Reverse Y direction
            reflected_this_frame = true;
        }
        else if (ball_y - BALL_RADIUS < 0) {
            ball_y = BALL_RADIUS; // Reposition
            ball_dy = std::abs(ball_dy); // Reverse Y direction
            reflected_this_frame = true;
        }

        // --- Paddle Collisions ---
        // Left Paddle Collision
        if (ball_dx < 0 && checkCircleRectCollision(ball_x, ball_y, BALL_RADIUS, leftPaddle)) {
            ball_x = leftPaddle.x + PADDLE_WIDTH + BALL_RADIUS; // Push ball out to prevent sticking
            ball_dx = std::abs(ball_dx); // Reverse X direction
            reflected_this_frame = true;
            playCoinSound(); // Play a sound on paddle hit

            // Scoring Rule 1: Consecutive hits for the left player
            if (last_ball_hit == LastHit::LeftPaddle) {
                left_consecutive_hits++;
                if (left_consecutive_hits >= 2) {
                    left_score++; // Award point for 2 consecutive hits
                    left_consecutive_hits = 0; // Reset after scoring
                }
            }
            else {
                left_consecutive_hits = 1; // Start a new consecutive streak
                right_consecutive_hits = 0; // Reset opponent's streak
            }
            last_ball_hit = LastHit::LeftPaddle;

        }
        // Right Paddle Collision
        else if (ball_dx > 0 && checkCircleRectCollision(ball_x, ball_y, BALL_RADIUS, rightPaddle)) {
            ball_x = rightPaddle.x - BALL_RADIUS; // Push ball out to prevent sticking
            ball_dx = -std::abs(ball_dx); // Reverse X direction
            reflected_this_frame = true;
            playCoinSound(); // Play a sound on paddle hit

            // Scoring Rule 1: Consecutive hits for the right player
            if (last_ball_hit == LastHit::RightPaddle) {
                right_consecutive_hits++;
                if (right_consecutive_hits >= 2) {
                    right_score++; // Award point for 2 consecutive hits
                    right_consecutive_hits = 0; // Reset after scoring
                }
            }
            else {
                right_consecutive_hits = 1; // Start a new consecutive streak
                left_consecutive_hits = 0; // Reset opponent's streak
            }
            last_ball_hit = LastHit::RightPaddle;
        }

        // --- Out of Bounds (Scoring and Ball Reset) ---
        // Ball goes past left paddle (right player scores)
        if (ball_x - BALL_RADIUS < 0) {
            right_score++;
            // Reset ball to center of the screen
            ball_x = WINDOW_WIDTH / 2.0f;
            ball_y = WINDOW_HEIGHT / 2.0f;
            // Set a new random initial direction for the next serve
            float angle;
            do {
                angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * M_PI;
            } while (std::abs(std::sin(angle)) < 0.2f || std::abs(std::cos(angle)) < 0.2f);
            ball_dx = cos(angle);
            ball_dy = sin(angle);

            // Normalize the direction vector
            float magnitude = sqrt(ball_dx * ball_dx + ball_dy * ball_dy);
            if (magnitude != 0) {
                ball_dx /= magnitude;
                ball_dy /= magnitude;
            }

            current_ball_speed = INITIAL_BALL_SPEED; // Reset ball speed
            ball_boost_timer = 0; // Clear any speed boost
            left_consecutive_hits = 0; // Reset consecutive hit counters
            right_consecutive_hits = 0;
            last_ball_hit = LastHit::None; // Reset last hit
        }
        // Ball goes past right paddle (left player scores)
        else if (ball_x + BALL_RADIUS > WINDOW_WIDTH) {
            left_score++;
            // Reset ball to center of the screen
            ball_x = WINDOW_WIDTH / 2.0f;
            ball_y = WINDOW_HEIGHT / 2.0f;
            // Set a new random initial direction for the next serve
            float angle;
            do {
                angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * M_PI;
            } while (std::abs(std::sin(angle)) < 0.2f || std::abs(std::cos(angle)) < 0.2f);
            ball_dx = cos(angle);
            ball_dy = sin(angle);

            // Normalize the direction vector
            float magnitude = sqrt(ball_dx * ball_dx + ball_dy * ball_dy);
            if (magnitude != 0) {
                ball_dx /= magnitude;
                ball_dy /= magnitude;
            }

            current_ball_speed = INITIAL_BALL_SPEED; // Reset ball speed
            ball_boost_timer = 0; // Clear any speed boost
            left_consecutive_hits = 0; // Reset consecutive hit counters
            right_consecutive_hits = 0;
            last_ball_hit = LastHit::None; // Reset last hit
        }


        // --- Ball Speed Boost Logic ---
        if (reflected_this_frame) {
            ball_boost_timer = BALL_BOOST_DURATION_FRAMES; // Start the boost timer
            current_ball_speed = INITIAL_BALL_SPEED * BALL_BOOST_FACTOR; // Apply speed boost
        }

        if (ball_boost_timer > 0) {
            ball_boost_timer--; // Decrement the timer each frame
            if (ball_boost_timer == 0) {
                current_ball_speed = INITIAL_BALL_SPEED; // Revert to initial speed when timer expires
            }
        }

        // --- Coin Logic ---
        coin_spawn_timer++;
        if (coin_spawn_timer >= COIN_APPEAR_INTERVAL_FRAMES) {
            spawnCoin(); // Spawn a new coin
            coin_spawn_timer = 0; // Reset the timer
        }

        Uint32 currentTime = SDL_GetTicks(); // Get current time for coin animation frame calculation

        // Get the effective size of the coin for collision calculations
        int coinEffectiveWidth = getCoinRenderedWidth(COIN_DRAW_SCALE);
        int coinEffectiveHeight = getCoinRenderedHeight(COIN_DRAW_SCALE);

        // Iterate through all active coins to check for expiry and collisions
        for (auto it = coins.begin(); it != coins.end(); ) {
            // Decrement coin's internal timer
            it->timer--;
            if (it->timer <= 0) {
                it = coins.erase(it); // Remove coin if its timer has run out
                continue; // Move to the next element (iterator is already advanced by erase)
            }

            // Create an SDL_Rect representing the coin's bounding box for collision checks
            SDL_Rect coinRect = {
                static_cast<int>(it->x - coinEffectiveWidth / 2),
                static_cast<int>(it->y - coinEffectiveHeight / 2),
                coinEffectiveWidth,
                coinEffectiveHeight
            };

            bool coin_collected = false; // Flag to check if the coin was collected this frame

            // Collision with ball (Scoring Rule 3: Ball collects coin)
            if (checkCircleRectCollision(ball_x, ball_y, BALL_RADIUS, coinRect)) {
                // Award score to the player who last hit the ball
                if (last_ball_hit == LastHit::LeftPaddle) {
                    left_score++;
                }
                else if (last_ball_hit == LastHit::RightPaddle) {
                    right_score++;
                }
                playCoinSound(); // Play coin collection sound
                coin_collected = true;
            }

            // Collision with left paddle (Scoring Rule 2: Paddle collects coin)
            // Only check if coin hasn't already been collected by the ball
            if (!coin_collected && checkRectRectCollision(leftPaddle, coinRect)) {
                left_score++;
                playCoinSound();
                coin_collected = true;
            }

            // Collision with right paddle (Scoring Rule 2: Paddle collects coin)
            // Only check if coin hasn't already been collected by ball or left paddle
            if (!coin_collected && checkRectRectCollision(rightPaddle, coinRect)) {
                right_score++;
                playCoinSound();
                coin_collected = true;
            }

            if (coin_collected) {
                it = coins.erase(it); // Remove the collected coin from the vector
            }
            else {
                ++it; // Move to the next coin if it was not collected this frame
            }
        }

        // --- Rendering ---
        SDL_SetRenderDrawColor(renderer, 0x1A, 0x20, 0x2C, 0xFF); // Set background color (Dark Slate Gray)
        SDL_RenderClear(renderer); // Clear the screen with the background color

        // Draw ball
        drawFilledCircle(renderer, static_cast<int>(ball_x), static_cast<int>(ball_y), BALL_RADIUS, 0xFF, 0x00, 0x00, 0xFF); // Red ball

        // Draw paddles
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0xFF); // Blue for left paddle
        SDL_RenderFillRect(renderer, &leftPaddle);

        SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF); // Green for right paddle
        SDL_RenderFillRect(renderer, &rightPaddle);

        // Draw all active coins using the coin system's draw function
        for (const auto& coin : coins) {
            draw_Coin(static_cast<int>(coin.x), static_cast<int>(coin.y), COIN_DRAW_SCALE, renderer, currentTime);
        }

        // Render scores for both players
        renderText(renderer, "Player 1: " + std::to_string(left_score), 50, 20, textColor);
        renderText(renderer, "Player 2: " + std::to_string(right_score), WINDOW_WIDTH - 200, 20, textColor);

        SDL_RenderPresent(renderer); // Update the screen with everything rendered
    }

    // --- Cleanup ---
    if (gFont) {
        TTF_CloseFont(gFont); // Close the loaded font
    }
    closeCoinSystem(); // Clean up all coin textures
    if (coin_sound) {
        Mix_FreeChunk(coin_sound); // Free the loaded sound effect
    }
    Mix_CloseAudio(); // Close SDL_mixer subsystem
    TTF_Quit();       // Quit SDL_ttf subsystem
    IMG_Quit();       // Quit SDL_image subsystem
    SDL_DestroyRenderer(renderer); // Destroy the renderer
    SDL_DestroyWindow(window);     // Destroy the window
    SDL_Quit();       // Quit all SDL subsystems

    return 0;
}