#include <sys/ioctl.h>

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

/* Error state is indicated by value=-1 in which case application exits
 * with error
 */
void
checkError(const int value, const char *message)
{
  if (value == -1)
  {
    g_message("OSS error: %s %s\n", message, strerror(errno));
    exit(1);
  }
}



/* Calculate frag by giving it minimal size of buffer */
int
size2frag(int x)
{
  int frag = 0;
  while ((1 << frag) < x) { ++frag; }
  return frag;
}


int
engine_oss_setup (AudioEngine *self)
{
  g_message ("setting up OSS...");
  int error;
  int tmp;
  /* Open the device for read and write */
  self->device = "/dev/dsp";
  self->fd = open(self->device, O_RDWR);
  checkError(self->fd, "open");

  /* Get device information */
  self->audioInfo.dev = -1;
  error = ioctl(self->fd, SNDCTL_ENGINEINFO, &(self->audioInfo));
  checkError(error, "SNDCTL_ENGINEINFO");
  if ((sample_rate_t)self->audioInfo.min_rate > self->sample_rate || self->sample_rate > (sample_rate_t)self->audioInfo.max_rate)
  {
    g_message("%s doesn't support chosen ", self->device);
    g_message("samplerate of %uHz!\n", self->sample_rate);
    exit(1);
  }
  self->audioInfo.max_channels = 2;

  tmp = self->audioInfo.max_channels;
  error = ioctl(self->fd, SNDCTL_DSP_CHANNELS, &tmp);
  checkError(error, "SNDCTL_DSP_CHANNELS");
  if (tmp != self->audioInfo.max_channels) /* or check if tmp is close enough? */
  {
    g_message("%s doesn't support chosen ", self->device);
    g_message("channel count of %d", self->audioInfo.max_channels);
    g_message(", set to %d!\n", tmp);
    exit(1);
  }

  /* Set format, or bit size: 8, 16, 24 or 32 bit sample */
  self->format = AFMT_S32_NE; /* Signed 32bit native endian format */
  tmp = self->format;
  error = ioctl(self->fd, SNDCTL_DSP_SETFMT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFMT");
  if (tmp != self->format)
  {
    g_message("%s doesn't support chosen sample format!\n", self->device);
    exit(1);
  }

  /* Most common values for samplerate (in kHz): 44.1, 48, 88.2, 96 */
  tmp = self->sample_rate;
  error = ioctl(self->fd, SNDCTL_DSP_SPEED, &tmp);
  checkError(error, "SNDCTL_DSP_SPEED");

  /* Get and check device capabilities */
  error = ioctl(self->fd, SNDCTL_DSP_GETCAPS, &(self->audioInfo.caps));
  checkError(error, "SNDCTL_DSP_GETCAPS");
  if (!(self->audioInfo.caps & PCM_CAP_DUPLEX))
  {
    g_message("Device doesn't support full duplex!\n");
    exit(1);
  }

  int minFrag = size2frag(4 * self->audioInfo.max_channels);
  if (self->frag < minFrag) { self->frag = minFrag; }

  /* Allocate buffer in fragments. Total buffer will be split in number
   * of fragments (2 by default)
   */
  if (self->bufferInfo.fragments < 0) { self->bufferInfo.fragments = 2; }
  tmp = ((self->bufferInfo.fragments) << 16) | self->frag;
  error = ioctl(self->fd, SNDCTL_DSP_SETFRAGMENT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFRAGMENT");

  /* When all is set and ready to go, get the size of buffer */
  error = ioctl(self->fd, SNDCTL_DSP_GETOSPACE, &(self->bufferInfo));
  checkError(error, "SNDCTL_DSP_GETOSPACE");
  self->block_length = self->bufferInfo.bytes / 4 / self->audioInfo.max_channels;

  g_message ("OSS setup complete");
  return 0;
}


void engine_oss_activate (
  AudioEngine * self,
  gboolean      activate)
{
  if (activate)
  {
    g_message ("activating...");
  }
  else
  {
    g_message ("deactivating...");
  }
}


int
engine_oss_test (
  GtkWindow * win)
{
  return 0;
}

void
engine_oss_tear_down (AudioEngine *self)
{
  close(self->fd);
}

#endif
