#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <queue>
#include <cmath>

const int AMPLITUDE = 28000;
const int FREQUENCY = 44100;


class APU {
  public:
  
}


int queue_audio(int deviceId) {

  Uint32 length = 44100; // should be 1 second
  Uint8* buffer = (Uint8*) malloc(length); // 1 second of audio?

  int flag = 1;

  for (int i = 0; i < length; i++) {
    if (i % 100 == 0) {
      flag = -flag;
    }
    buffer[i] = flag * 10000;
  }


  int success = SDL_QueueAudio(deviceId, buffer, length);
  return success;

}


int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_AUDIO);

    SDL_AudioSpec desiredSpec;

    desiredSpec.freq = FREQUENCY;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = 2048;
    desiredSpec.callback = NULL;
    desiredSpec.userdata = NULL;

    SDL_AudioSpec obtainedSpec;

    int duration = 1000;
    double Hz = 440;

    // start play audio

    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &obtainedSpec, 0);

    int success = queue_audio(deviceId);
    printf("%d", success);
    SDL_PauseAudioDevice(deviceId, 0);

    SDL_Delay(3000);

    SDL_CloseAudioDevice(deviceId);
    SDL_Quit();

    return 0;


    return 0;
}
