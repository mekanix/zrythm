#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_oss.h"
#include "audio/midi.h"
#include "audio/router.h"
#include "audio/port.h"
#include "audio/tempo_track.h"
#include "project.h"
#include "settings/settings.h"
#include "zrythm_app.h"


#ifdef HAVE_OSS

int
engine_oss_setup (
  AudioEngine *self)
{
  g_message ("setting up OSS...");
  g_message ("OSS setup complete");
  return 0;
}


void
engine_oss_tear_down (AudioEngine *self)
{
}

#endif
