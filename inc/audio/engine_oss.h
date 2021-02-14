#ifdef HAVE_ALSA
#ifndef __AUDIO_ENGINE_OSS_H__
#define __AUDIO_ENGINE_OSS_H__

int
engine_oss_setup (
  AudioEngine * self);

void
engine_oss_tear_down (
  AudioEngine * self);

#endif
#endif
