#include <assert.h>
#include <string.h>
#include <readline/readline.h>

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>


SDL_Surface *screen;

#define LENGTH 500
#define RANGE 0xFF
#define SCALE(i) (i >> 1) /* adjust volume */
const int BORDER = 1;
#define NOISE_RANGE 4

Uint8 data[LENGTH];

int lastx = -1, lasty = -1;
int linex = 0, liney = 0;
int drawline = 0;



void draw_derivative() {
  for (int i = 1; i < LENGTH - 1; i++) {
    pixelRGBA(screen, BORDER+LENGTH*(0.5)+i, BORDER+RANGE-abs(data[i]-data[i+1]), 0x00, 0x80, 0x00, 0xFF);
  }
}

void draw_data() {
  for (int i = 0; i < LENGTH; i++) {
    pixelRGBA(screen, BORDER+LENGTH*(0.5)+i, BORDER+data[i], 0xFF, 0xFF, 0x00, 0xFF);
  }
}

void draw_shadow() {
  //right
  for (int i = 0; i < LENGTH/2; i++) {
    pixelRGBA(screen, BORDER+LENGTH*(0.5+1)+i, BORDER+data[i], 0x80, 0x80, 0xFF, 0xFF);
  }
  //left
  for (int i = LENGTH/2; i < LENGTH; i++) {
    pixelRGBA(screen, BORDER+LENGTH*(0.5-1)+i, BORDER+data[i], 0x80, 0x80, 0xFF, 0xFF);
  }
  
}

void draw_line() {
  if (drawline && linex != -1 && lastx != -1) {
    int sx = BORDER+LENGTH*(0.5), sy = BORDER;
    lineRGBA(screen, sx+lastx, sy+lasty, sx+linex, sy+liney, 0x80, 0x80, 0x80, 0xFF);
  }
}

void draw() {
  SDL_FillRect(screen, NULL, 0);
  draw_derivative();
  draw_shadow();
  draw_data();
  draw_line();

  rectangleRGBA(screen, 0, 0, BORDER+LENGTH*2, BORDER+RANGE, 0xFF, 0x00, 0x00, 0xFF);
  SDL_Flip(screen);
}



void init_video() {
  screen = SDL_SetVideoMode(LENGTH*(.5+1+.5) + 2*BORDER, RANGE + 2*BORDER, 24, SDL_SWSURFACE);

  if (screen == NULL) {
    printf("Didn't get screen: %s\n", SDL_GetError());
    exit(-1);
  }
  SDL_WM_SetCaption("Wavelet", "Wavelet");
}

void fill_audio(void *udata, Uint8 *stream, int len) {
  static int offset = 0;
  //TODO: use memcpy instead
  for (int i = 0; i != len; i++) {
    offset = (offset+1) % LENGTH;
    stream[i] = SCALE(data[offset]);
  }
}

void blank() {
  memset(data, 0x80, LENGTH); //set to neutral
}

void init_audio() {
  blank();
  SDL_AudioSpec spec;
  spec.freq = 22050;
  spec.format = AUDIO_S8;
  spec.channels = 1;
  spec.samples = 1024; //or maybe LENGTH?
  spec.callback = fill_audio;
  if (SDL_OpenAudio(&spec, NULL) < 0) {
    printf("Didn't open audio: %s\n", SDL_GetError());
    exit(-1);
  }
  SDL_PauseAudio(0);
}


Uint32 timer_interval;
Uint32 timer_callback(Uint32 interval, void *param) {
  SDL_Event e;
  e.type = SDL_USEREVENT;
  SDL_PushEvent(&e);
  return timer_interval;
}

void init_time() {
  timer_interval = 10;
  SDL_AddTimer(10, timer_callback, NULL);
}

Uint8 get(int i) {
  i %= LENGTH;
  if (i < 0) {
    i += LENGTH;
  }
  return data[i];
}

void Xsmooth() {
  Uint8 mean[LENGTH];
  for (int i = 0; i < LENGTH; i++) {
    mean[i] = (get(i-1)+get(i+1))/2;
  }
  memcpy(data, mean, sizeof(Uint8)*LENGTH);
}

void smooth() {
  data[0] = (data[1]+data[LENGTH-1])/2;
  for (int i = 1; i < LENGTH - 1; i++) {
    data[i] = (data[i-1]+data[i+1])/2;
  }
  data[LENGTH-1] = (data[0]+data[LENGTH-2])/2;
}



void mouser(int x, int y) {
  if (lastx != -1 && lasty != -1) {
    //put a line 'tween them
    float m = ((float)(lasty - y))/((float)(lastx - x));
    int b = y - m*x;
    int dx = x - lastx;
    if (dx) {
      int delta = dx > 0 ? 1 : -1;
      for (int xp = lastx; xp != x; xp += delta) {
        data[xp] = m*xp+b;
      }
    }
  }
  data[x] = y;
  lastx = x, lasty = y;
}

void noise(int x) {
  int start = x - NOISE_RANGE, end = x + NOISE_RANGE;
  if (start < 0) {
    start = 0;
  }
  if (end > LENGTH) {
    end = LENGTH - 1;
  }
  for (int i = start; i < end; i++) {
    data[i] = rand() % RANGE;
  }
}

void drag(int x, int y) {
  int start = x - 5*NOISE_RANGE, end = x + 5*NOISE_RANGE;
  if (start < 0) {
    start = 0;
  }
  if (end > LENGTH) {
    end = LENGTH - 1;
  }
  for (int i = start; i < end; i++) {
    int delta = data[i] - ((data[i]+y)/2);
    data[i] -= delta/5;
  }
}

void output() {
  char *filename = readline("output filename: ");
  FILE *f = fopen(filename, "w");
  if (f == NULL) {
    printf("Couldn't open file.\n");
    return;
  }
  for (int i = 0; i < LENGTH; i++) {
    fprintf(f, "%i\n", data[i]);
  }
  fclose(f);
  free(filename);
  printf("Success.\n");
}

void input() {
  char *filename = readline("input filename: ");
  FILE *f = fopen(filename, "r");
  if (f == NULL) {
    printf("Couldn't open file.\n");
    return;
  }
  blank();
  for (int i = 0; i < LENGTH; i++) {
    int val;
    fscanf(f, "%i\n", &val);
    data[i] = val;
  }
  free(filename);
  printf("Success.\n");
}

void loopdy_loop() {
  SDL_Event event;
  int mousedown = 0;
  int need_draw = 1;
  int dopause = 0;

  int do_smooth = 1;
  int do_noise = 0;
  int do_drag = 0;

  while (SDL_WaitEvent(&event)) {
    switch (event.type) {
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_q) {
          return;
        }
        if (event.key.keysym.sym == SDLK_s || event.key.keysym.sym == SDLK_d) {
          do_smooth = 1;
        }
        if (event.key.keysym.sym == SDLK_m) {
          dopause = !dopause;
          SDL_PauseAudio(dopause);
        }
        if (event.key.keysym.sym == SDLK_l) {
          drawline = 1;
        }
        if (event.key.keysym.sym == SDLK_b) {
          blank();
          need_draw = 1;
        }
        if (event.key.keysym.unicode == L'>') {
          timer_interval = timer_interval > 1 ? timer_interval - 1 : 1;
          printf("Smooth speed: %i\n", (int)timer_interval);
        }
        if (event.key.keysym.unicode == L'<') {
          timer_interval++;
          printf("Smooth speed: %i\n", (int)timer_interval);
        }
        if (event.key.keysym.sym == SDLK_i) {
          if (dopause == 0) {
            SDL_PauseAudio(1);
          }
          input();
          need_draw = 1;
          if (dopause == 0) {
            SDL_PauseAudio(0);
          }
        }
        if (event.key.keysym.sym == SDLK_o) {
          if (dopause == 0) {
            SDL_PauseAudio(1);
          }
          output();
          if (dopause == 0) {
            SDL_PauseAudio(0);
          }
        }
        break;
      case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_l) {
          drawline = 0;
          mouser(linex, liney);
          need_draw = 1;
          linex = -1, liney = -1;
        }
        if (event.key.keysym.sym == SDLK_s) {
          do_smooth = 0;
        }
        break;
      case SDL_MOUSEBUTTONDOWN:
        lastx = -1, lasty = -1;
        mousedown = 1;
        if (event.button.button == SDL_BUTTON_RIGHT) {
          do_noise = 1;
        }
        if (event.button.button == SDL_BUTTON_MIDDLE) {
          do_drag = 1;
        }
        break;
      case SDL_MOUSEBUTTONUP:
        mousedown = 0;
        if (event.button.button == SDL_BUTTON_RIGHT) {
          do_noise = 0;
        }
        if (event.button.button == SDL_BUTTON_MIDDLE) {
          do_drag = 0;
        }
        break;
      case SDL_MOUSEMOTION: {
        int mx = (event.motion.x - BORDER - LENGTH/2);
        if (mx >= LENGTH) {
          mx = LENGTH - 1;
        }
        if (mx < 0) {
          mx = 0;
        }
        int my = event.motion.y;
        if (mousedown) {
          if (do_noise) {
            noise(mx);
            need_draw = 1;
          }
          else if (do_drag) {
            drag(mx, my);
            need_draw = 1;
          }
          else if (drawline) {
            linex = mx, liney = my;
          }
          else {
            mouser(mx, my);
            need_draw = 1;
          }
        }
        break;
      }
      case SDL_VIDEOEXPOSE:
        need_draw = 1;
        break;
      case SDL_USEREVENT:
        if (do_smooth) {
          smooth();
          need_draw = 1;
        }
        break;
        
      default:
        break;
    }


    if (need_draw) {
      draw();
      need_draw = 0;
    }
  }
}


int main() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    printf("Didn't initialize SDL: %s\n", SDL_GetError());
    exit(-1);
  }

  init_video();
  init_audio();
  init_time();
  SDL_EnableUNICODE(1);

  loopdy_loop();
  printf("Goodbye\n");
  SDL_QuitSubSystem(SDL_INIT_VIDEO); //ending audio takes like 3 seconds. On my computer, anyways.  Maybe disown ourselves?
  SDL_PauseAudio(1);

  SDL_Quit();

  return 0;
}

