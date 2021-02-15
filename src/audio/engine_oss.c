#include <sys/ioctl.h>
#include <sys/soundcard.h>

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
    fprintf(stderr, "OSS error: %s %s\n", message, strerror(errno));
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
  self->fd = open(self->device, O_RDWR);
  checkError(self->fd, "open");

  /* Get device information */
  self->audioInfo.dev = -1;
  error = ioctl(self->fd, SNDCTL_ENGINEINFO, &(self->audioInfo));
  checkError(error, "SNDCTL_ENGINEINFO");
  if (self->audioInfo.min_rate > self->sampleRate || self->sampleRate > self->audioInfo.max_rate)
  {
    fprintf(stderr, "%s doesn't support chosen ", self->device);
    fprintf(stderr, "samplerate of %dHz!\n", self->sampleRate);
    exit(1);
  }
  if (self->channels < 1)
  {
    self->channels = self->audioInfo.max_channels;
  }

  /* Set number of channels. If number of channels is chosen to the value
   * near the one wanted, save it in config
   */
  tmp = self->channels;
  error = ioctl(self->fd, SNDCTL_DSP_CHANNELS, &tmp);
  checkError(error, "SNDCTL_DSP_CHANNELS");
  if (tmp != self->channels) /* or check if tmp is close enough? */
  {
    fprintf(stderr, "%s doesn't support chosen ", self->device);
    fprintf(stderr, "channel count of %d", self->channels);
    fprintf(stderr, ", set to %d!\n", tmp);
  }
  self->channels = tmp;

  /* Set format, or bit size: 8, 16, 24 or 32 bit sample */
  tmp = self->format;
  error = ioctl(self->fd, SNDCTL_DSP_SETFMT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFMT");
  if (tmp != self->format)
  {
    fprintf(stderr, "%s doesn't support chosen sample format!\n", self->device);
    exit(1);
  }

  /* Most common values for samplerate (in kHz): 44.1, 48, 88.2, 96 */
  tmp = self->sampleRate;
  error = ioctl(self->fd, SNDCTL_DSP_SPEED, &tmp);
  checkError(error, "SNDCTL_DSP_SPEED");

  /* Get and check device capabilities */
  error = ioctl(self->fd, SNDCTL_DSP_GETCAPS, &(self->audioInfo.caps));
  checkError(error, "SNDCTL_DSP_GETCAPS");
  if (!(self->audioInfo.caps & PCM_CAP_DUPLEX))
  {
    fprintf(stderr, "Device doesn't support full duplex!\n");
    exit(1);
  }

  /* If desired frag is smaller than minimum, based on number of channels
   * and format (size in bits: 8, 16, 24, 32), set that as frag. Buffer size
   * is 2^frag, but the real size of the buffer will be read when the
   * configuration of the device is successfull
   */
  int minFrag = size2frag(self->sampleSize * self->channels);
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
  /* self->sampleCount = self->bufferInfo.bytes / self->sampleSize; */
  /* self->nsamples =  self->sampleCount / self->channels; */

  g_message ("OSS setup complete");
  return 0;
}


void
engine_oss_tear_down (AudioEngine *self)
{
  close(self->fd);
}

#endif
