class GUI {
  public:
  PPU* ppu;
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  SDL_Rect baselayer;
  bool valid;
  int frames;

  void set_ppu(PPU*);
  void initialize();
  void close_gui();
  void render_frame(uint8_t*);
};

void GUI::initialize() {
  SDL_Init(SDL_INIT_VIDEO);
	// Create window
	window = SDL_CreateWindow(
    "NESmerize",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    256,
    240,
    SDL_WINDOW_RESIZABLE
  );
  renderer = SDL_CreateRenderer(
    window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );
  SDL_RenderSetLogicalSize(renderer, 256, 240);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    256,
    240
  );
  if (texture == NULL) {
    exit(1);
  }
  baselayer = {
    0,
    0,
    256,
    240
  };
  valid = true;
  frames = 0;
}

void GUI::close_gui() {
	// Destroy window
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	// Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
  valid = false;
}

void GUI::render_frame(uint8_t* framebuffer) {
	SDL_Event e;

	while (SDL_PollEvent(&e) != 0) {
		if (e.type == SDL_QUIT) {
      close_gui();
			return;
		}
	}
  // update with new frame data
  SDL_UpdateTexture(texture, nullptr, framebuffer, 256 * 4);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
  frames++;
}