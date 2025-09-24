#include <SDL.h>
#include <iostream>

// Function to create a simple 1x1 white texture
// This can be used to draw colored rectangles directly with SDL_RenderCopyEx
SDL_Texture* createWhiteTexture(SDL_Renderer* renderer) {
    // Create a 1x1 texture
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
    if (!texture) {
        SDL_Log("Failed to create white texture: %s", SDL_GetError());
        return nullptr;
    }

    // Set the texture's blend mode for alpha support
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    // Create a pixel array with a single white pixel (RGBA)
    Uint32 pixel = 0xFFFFFFFF; // White opaque (RRGGBBAA)
    // Update the 1x1 texture with this pixel
    SDL_UpdateTexture(texture, NULL, &pixel, sizeof(Uint32));

    return texture;
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Rotate SDL_Rect Example",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        800, 600,
        SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    // SDL_RENDERER_ACCELERATED uses hardware acceleration, SDL_RENDERER_PRESENTVSYNC caps frame rate
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Enable alpha blending for the renderer
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Create a simple white texture to represent our rectangle
    SDL_Texture* rectTexture = createWhiteTexture(renderer);
    if (!rectTexture) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Define the destination rectangle
    SDL_Rect rotatingRect = {
        .x = 300, // X position
        .y = 200, // Y position
        .w = 200, // Width
        .h = 100  // Height
    };

    // Define the rotation center point relative to the rotatingRect's top-left corner
    // For rotation around itself, this is simply half the width and half the height
    SDL_Point rotationCenter = {
        .x = rotatingRect.w / 2,
        .y = rotatingRect.h / 2
    };

    double currentAngle = 0.0; // Current rotation angle in degrees

    // Event loop flag
    bool quit = false;
    SDL_Event e;

    Uint32 lastFrameTime = SDL_GetTicks(); // Time of the last frame

    while (!quit) {
        // Event handling
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        // Calculate delta time for consistent animation speed
        Uint32 currentFrameTime = SDL_GetTicks();
        float deltaTime = (currentFrameTime - lastFrameTime) / 1000.0f; // Convert to seconds
        lastFrameTime = currentFrameTime;

        // Update the rotation angle (e.g., 60 degrees per second)
        currentAngle += 60.0 * deltaTime;
        if (currentAngle >= 360.0) {
            currentAngle -= 360.0; // Keep angle within 0-360 range
        }

        // Clear the renderer with a background color (e.g., dark gray)
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);

        // Set the color modulation for the texture to give our rectangle a color
        SDL_SetTextureColorMod(rectTexture, 255, 100, 0); // Orange color

        // Render the texture, rotated around the center of its destination rect
        // The last NULL parameter means no source rectangle (use entire texture, which is 1x1 here)
        // SDL_FLIP_NONE means no flipping
        SDL_RenderCopyEx(renderer,
            rectTexture,
            NULL, // Source rect (NULL for entire texture)
            &rotatingRect, // Destination rect
            currentAngle, // Angle in degrees
            &rotationCenter, // Point of rotation relative to destination rect's top-left
            SDL_FLIP_NONE // No flipping
        );

        // Update the screen
        SDL_RenderPresent(renderer);
    }

    // Clean up
    SDL_DestroyTexture(rectTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
