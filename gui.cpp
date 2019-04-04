class GUI {
  public:
  PPU* ppu;
  Memory* memory;
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
  uint8_t get_input();
};

struct InputData {
  bool a : 1;
  bool b : 1;
  bool select : 1;
  bool start : 1;
  bool up : 1;
  bool down : 1;
  bool left : 1;
  bool right : 1;
};

union Input {
  uint8_t value;
  InputData data;
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

uint8_t GUI::get_input() {
  Input input_data;
  input_data.value = 0;
  SDL_PumpEvents(); // update event queue to avoid being a frame behind
  const uint8_t* keystate = SDL_GetKeyboardState(NULL);

  // update bitfield with information about keyboard
  if (keystate[SDL_SCANCODE_DOWN]) {
    input_data.data.down = 1;
  }
  if (keystate[SDL_SCANCODE_UP]) {
    input_data.data.up = 1;
  }
  if (keystate[SDL_SCANCODE_RIGHT]) {
    input_data.data.right = 1;
  }
  if (keystate[SDL_SCANCODE_LEFT]) {
    input_data.data.left = 1;
  }
  if (keystate[SDL_SCANCODE_Z]) {
    input_data.data.a = 1;
  }
  if (keystate[SDL_SCANCODE_X]) {
    input_data.data.b = 1;
  }
  if (keystate[SDL_SCANCODE_RSHIFT]) {
    input_data.data.select = 1;
  }
  if (keystate[SDL_SCANCODE_RETURN]) {
    input_data.data.start = 1;
  }

  return input_data.value;
}

// TODO: why does this work at 60fps even without vsync or anything?
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
