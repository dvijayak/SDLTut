#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

using namespace std;


/**
 * Log an SDL error to the specified output stream.
 * @param os The output stream to write the message to
 * @param message The SDL error message
 */
void logSDLError (ostream &os, const string &message) {
	os << message << " Error: " << SDL_GetError() << endl;
}

/**
 * Load an image file into a texture on the rendering device.
 * @param file The image file to load
 * @param renderer The renderer to load the texture onto
 * @return The loaded texture, or nullptr if something went wrong
 */
SDL_Texture* loadTexture (const string &file, SDL_Renderer *renderer) {
	SDL_Texture *texture = IMG_LoadTexture(renderer, file.c_str());
	if (texture == nullptr) {
		logSDLError(cout, "SDL_LoadTexture");
	}

	return texture;
}

/**
 * Draw a texture to a renderer at position (x,y) while applying any desired scaling.
 * Note: textures are always drawn back to front, first to last.
 * (So always draw background tiles first)
 * @param texture The source texture we want to draw
 * @param renderer The renderer we want to draw to
 * @param x The x coordinate to draw at
 * @param y The y coordinate to draw at
 * @param wScale The factor by which the width is scaled (1 = original size)
 * @param hScale The factor by which the height is scaled (1 = original size)
 * @param clip The sub-section of the texture to draw (clipping rectangle); default of nullptr draws the entire texture
 */
void renderTexture (SDL_Texture *texture, SDL_Renderer *renderer,
		int x, int y, float wScale=1.0, float hScale=1.0, SDL_Rect *clip=nullptr) {
	// Get the width and height of the texture
	int textureWidth, textureHeight;
	SDL_QueryTexture(texture, NULL, NULL, &textureWidth, &textureHeight);

	// Set up the destination rectangle to be at the position and size that we want
	SDL_Rect destination;
	destination.x = x;
	destination.y = y;
	// Ensure that scale factors are greater than 0
	if (wScale == 0) {
		wScale = 1;
	}
	else if (wScale < 0) {
		wScale *= -1;
	}
	if (hScale == 0) {
		hScale = 1;
	}
	else if (hScale < 0) {
		hScale *= -1;
	}
	// Apply the requested scaling to the texture
	destination.w = textureWidth * wScale;
	destination.h = textureHeight * hScale;

	// Apply the clipping rectangle if requested
	if (clip != nullptr) {
		destination.w = clip->w * wScale;
		destination.h = clip->h * hScale;
	}

	// Copy the texture to the rendering context
	SDL_RenderCopy(renderer, texture, clip, &destination);
}

/**
 * Fill up the window with a texture in a tiled fashion. Apply scaling as desired.
 * @param renderer The rendering context we want to draw to
 * @param source The source texture we want to tile
 * @param destination The destination texture on which we wish to tile the iamge
 * @param wScale The factor by which the width is scaled (1 = original size)
 * @param hScale The factor by which the height is scaled (1 = original size)
 */
void drawTiles (SDL_Renderer *renderer, SDL_Texture *source, SDL_Texture *destination,
		float wScale=1.0, float hScale=1.0, SDL_Rect *clip=nullptr) {
	// Retrieve the width and height of the texture
	int textureWidth, textureHeight;
	SDL_QueryTexture(source, NULL, NULL, &textureWidth, &textureHeight);
	if (clip != nullptr) {
		cout << "\tLOG:\t" << textureWidth << " " << clip->w << " " << endl;
		textureWidth = clip->w;
		textureHeight = clip->h;
	}
	// Apply the scaling factors
	textureWidth *= wScale;
	textureHeight *= hScale;

	// Compute the number of rows and columns for the tile matrix
	int destinationWidth, destinationHeight;
	SDL_QueryTexture(destination, NULL, NULL, &destinationWidth, &destinationHeight);
	int rows = destinationWidth/textureWidth;
	int cols = destinationHeight/textureHeight;
	cout << "\tLOG:\t" << rows << " " << cols << " " << endl;

	// Draw the texture as if we were filling up a matrix
	int width = 0, height = 0;
	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < cols; col++) {
			cout << "\tLOG:\t" << width << " " << height << " " << endl;
			// Draw the texture on to the rendering context
			renderTexture(source, renderer, width, height, wScale, hScale, clip);

			// Shift the width
			width += textureWidth;
		}
		// Shift the height
		height += textureHeight;

		// Reset the width; we are starting a new row
		width = 0;
	}

}

/**
 * The main thread of execution
 * @param argc Number of command-line arguments
 * @param argv List of command-line arguments
 */
int main (int argc, char **argv) {
	const int SCREEN_WIDTH = 640;
	const int SCREEN_HEIGHT = 480;
	const float SCALE_WIDTH = 0.2;
	const float SCALE_HEIGHT = SCALE_WIDTH; /* We'll just be using square tiles for now */
	const int TILESET_ROW_SIZE = 2; /* The number of row in a uniform tileset/tilesheet */
	const int TILESET_COL_SIZE = 2; /* The number of tiles per row in a uniform tileset/tilesheet */

	// Initialize the various SDL subsystems we want to use
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { /* Bare minimum is SDL_INIT_VIDEO */
		logSDLError(cout, "SDL_Init");
		return 1;
	}
	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
		logSDLError(cout, "IMG_Init");
	}

	// Open a window to display our renderings in
	SDL_Window *window = SDL_CreateWindow("SDL Lesson 2"/*title*/, 100/*x*/, 100/*y*/,
			SCREEN_WIDTH/*w*/, SCREEN_HEIGHT/*h*/, SDL_WINDOW_SHOWN);
	if (window == nullptr/* New C++11 dedicated null pointer */) {
		logSDLError(cout, "SDL_CreateWindow");
		return 2;
	}

	// Create a renderer to draw to the window
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1/*index of rendering drive to be used*/,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == nullptr) {
		logSDLError(cout, "SDL_CreateRenderer");
		return 3;
	}

	/********************/

	// Load the background tile
	SDL_Texture *background = loadTexture("./tilesett-960x240.png", renderer);
	// Load the foreground image
	SDL_Texture *mainImage = loadTexture("./image.png", renderer);
	// Make sure the images are loaded OK
	if (background == nullptr || mainImage == nullptr) {
		return 4;
	}

	// Obtain a texture of the window
	SDL_Texture *windowTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
			SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Draw the clipped foreground image centered in the window
	int iWidth, iHeight;
	SDL_QueryTexture(mainImage, NULL, NULL, &iWidth, &iHeight);
	iWidth /= TILESET_ROW_SIZE;
	iHeight /= TILESET_COL_SIZE;
	// Offset the draw position in order to center the image
	int x = SCREEN_WIDTH/2 - iWidth/2;
	int y = SCREEN_HEIGHT/2 - iHeight/2;

	// Prepare the clipping rectangle for our foreground texture
	SDL_Rect clip;
	clip.x = clip.y = 0;
	clip.w = iWidth;
	clip.h = iHeight;

	// Prepare the clipping rectangle for our background texture
	SDL_Rect bgClip;
	bgClip.x = bgClip.y = 0;
	SDL_QueryTexture(background, NULL, NULL, &bgClip.w, &bgClip.h);
	bgClip.w /= 3;
	/******************/

	SDL_Event event;
	bool quit = false;
	// Main loop
	while (!quit) {
		// Process every event from the event queue as long as there are queued events
		while (SDL_PollEvent(&event)) {
			// Recognize the X button (close) on the window
			if (event.type == SDL_QUIT) {
				quit = true;
			}

			// Use number input to select which clip should be drawn
			if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
					case SDLK_UP:
						clip.y -= 2;;
						break;
					case SDLK_DOWN:
						clip.y += 2;;
						break;
					case SDLK_LEFT:
						clip.x -= 2;
						break;
					case SDLK_RIGHT:
						clip.x += 2;;
						break;
					case SDLK_ESCAPE:
						quit = true;
						break;
					default:
						break;
				}
				// Enables cycling through the textures in the tileset
				clip.x = clip.x % (iWidth*TILESET_ROW_SIZE);
				clip.y = clip.y % (iHeight*TILESET_COL_SIZE);

				cout << "\tLOG:\t" << clip.x << " " << clip.y << " " << endl;
			}
		}

		SDL_RenderClear(renderer); /* Clear the renderer (Why?) */
		// Draw the background tiles
		drawTiles(renderer, background, windowTexture, SCALE_WIDTH, SCALE_HEIGHT, &bgClip);
		// Update the foreground texture based on the clipping rectangle
		renderTexture(mainImage, renderer, x, y, 1.0, 1.0, &clip); /* Initially render only the first tile */

		// Present the updated screen to the user
		SDL_RenderPresent(renderer);
	}

	// Clean-up
	SDL_DestroyTexture(background);
	SDL_DestroyTexture(mainImage);
	SDL_DestroyTexture(windowTexture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}