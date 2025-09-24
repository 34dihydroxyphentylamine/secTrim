#include "coin.h"
#include <SDL_image.h> // Required for IMG_LoadTexture
#include <iostream>    // For error output
#include <string>      // For std::string and std::to_string
#include <vector>      // For std::vector

// Global vector to hold all coin animation textures
std::vector<SDL_Texture*> gCoinTextures;

// Number of frames in the animation (coin_01.png to coin_08.png means 8 frames)
const int NUM_COIN_FRAMES = 8;
// How long each frame is displayed in milliseconds.
// A smaller number makes the animation faster.
const int COIN_ANIMATION_SPEED_MS = 100; // 100ms per frame = 10 frames per second

// Default dimensions for a coin frame. Used if textures aren't loaded yet for collision checks.
// You might need to adjust these if your coin images have different inherent sizes.
const int DEFAULT_COIN_FRAME_WIDTH = 32;
const int DEFAULT_COIN_FRAME_HEIGHT = 32;

// Initializes the coin system by loading all individual coin textures.
// Returns true if all textures are loaded successfully, false otherwise.
bool initCoinSystem(SDL_Renderer* renderer) {
    // Clear any existing textures in case init is called multiple times
    closeCoinSystem();

    for (int i = 1; i <= NUM_COIN_FRAMES; ++i) {
        // Construct the filename: "coin_01.png", "coin_02.png", etc.
        std::string filename = "coin_0" + std::to_string(i) + ".png";
        SDL_Texture* texture = IMG_LoadTexture(renderer, filename.c_str());
        if (texture == nullptr) {
            std::cerr << "Failed to load coin texture: " << filename << "! SDL_image Error: " << IMG_GetError() << std::endl;
            // Clean up any textures that were loaded before the failure
            closeCoinSystem(); // Call close to free any partially loaded textures
            return false;
        }
        gCoinTextures.push_back(texture);
    }
    std::cout << "Successfully loaded " << gCoinTextures.size() << " coin textures." << std::endl;
    return true;
}

// Draws an animated coin at the given position and scale.
void draw_Coin(int x, int y, float scale, SDL_Renderer* renderer, Uint32 currentTime) {
    // Ensure textures are loaded and renderer is valid before attempting to draw
    if (gCoinTextures.empty() || renderer == nullptr) {
        // std::cerr << "Cannot draw coin: textures not loaded or renderer invalid." << std::endl;
        return;
    }

    // Determine the current frame to display based on time
    // This will cycle through frames 0, 1, 2, ..., NUM_COIN_FRAMES-1
    int frame_index = (currentTime / COIN_ANIMATION_SPEED_MS) % NUM_COIN_FRAMES;

    // Get the texture for the current frame
    SDL_Texture* currentTexture = gCoinTextures[frame_index];

    // Query the original dimensions of the texture to calculate scaled dimensions
    int originalWidth, originalHeight;
    SDL_QueryTexture(currentTexture, NULL, NULL, &originalWidth, &originalHeight);

    // Calculate the scaled width and height for drawing
    int scaledWidth = static_cast<int>(originalWidth * scale);
    int scaledHeight = static_cast<int>(originalHeight * scale);

    // Define the destination rectangle on the screen.
    // We center the coin around the provided (x,y) coordinates.
    SDL_Rect dstRect = { x - scaledWidth / 2, y - scaledHeight / 2, scaledWidth, scaledHeight };

    // Render the current frame of the coin animation
    SDL_RenderCopy(renderer, currentTexture, NULL, &dstRect);
}

// Returns the effective rendered width of a coin, taking into account the scale.
// Uses the first loaded texture's dimensions as a reference.
int getCoinRenderedWidth(float scale) {
    if (gCoinTextures.empty()) {
        // If textures aren't loaded, return a default width to avoid issues in collision
        return static_cast<int>(DEFAULT_COIN_FRAME_WIDTH * scale);
    }
    int originalWidth, originalHeight;
    SDL_QueryTexture(gCoinTextures[0], NULL, NULL, &originalWidth, &originalHeight);
    return static_cast<int>(originalWidth * scale);
}

// Returns the effective rendered height of a coin, taking into account the scale.
// Uses the first loaded texture's dimensions as a reference.
int getCoinRenderedHeight(float scale) {
    if (gCoinTextures.empty()) {
        // If textures aren't loaded, return a default height to avoid issues in collision
        return static_cast<int>(DEFAULT_COIN_FRAME_HEIGHT * scale);
    }
    int originalWidth, originalHeight;
    SDL_QueryTexture(gCoinTextures[0], NULL, NULL, &originalWidth, &originalHeight);
    return static_cast<int>(originalHeight * scale);
}

// Cleans up all loaded coin textures by destroying them and clearing the vector.
void closeCoinSystem() {
    for (SDL_Texture* texture : gCoinTextures) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    gCoinTextures.clear(); // Ensure the vector is empty after freeing textures
    std::cout << "Coin system textures cleaned up." << std::endl;
}