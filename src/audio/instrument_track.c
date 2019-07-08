/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "config.h"

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/instrument_track.h"
#include "audio/midi.h"
#include "audio/midi_note.h"
#include "audio/position.h"
#include "audio/region.h"
#include "audio/track.h"
#include "audio/velocity.h"
#include "plugins/lv2/control.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "gui/widgets/track.h"
#include "gui/widgets/automation_track.h"
#include "utils/arrays.h"

#include <gtk/gtk.h>

/**
 * Initializes an instrument track.
 */
void
instrument_track_init (Track * track)
{
  track->type = TRACK_TYPE_INSTRUMENT;
  gdk_rgba_parse (&track->color, "#F79616");
}

void
instrument_track_setup (InstrumentTrack * self)
{
  channel_track_setup (self);
}

/**
 * Fills MIDI event queue from track.
 *
 * The events are dequeued right after the call to
 * this function.
 *
 * NOTE: This function is used real-time.
 *
 * @param pos Start Position to check.
 * @param nframes Number of frames at start
 *   Position.
 * @param midi_events MidiEvents to fill (from
 *   Piano Roll Port for example).
 */
__attribute__((annotate("realtime")))
void
instrument_track_fill_midi_events (
  InstrumentTrack * track,
  Position *        pos,
  uint32_t          nframes,
  MidiEvents *      midi_events)
{
  int i, j;
  Position end_pos;
  Position local_pos, local_end_pos;
  Position loop_start_adjusted,
           loop_end_adjusted,
           region_end_adjusted;
  Region * region, * r;
  MidiNote * midi_note;
  position_set_to_pos (&end_pos, pos);
  position_add_frames (&end_pos, nframes);

  zix_sem_wait (&midi_events->access_sem);

  TrackLane * lane;
  for (j = 0; j < track->num_lanes; j++)
    {
      lane = track->lanes[j];

      for (i = 0; i < lane->num_regions; i++)
        {
          region = lane->regions[i];
          r = (Region *) region;

          if (!region_is_hit (r,
                              pos))
            continue;

          /* get local positions */
          region_timeline_pos_to_local (
            r, pos, &local_pos, 1);
          region_timeline_pos_to_local (
            r, &end_pos, &local_end_pos, 1);

          /* add clip_start position to start from
           * there */
          long clip_start_ticks =
            position_to_ticks (
              &r->clip_start_pos);
          long region_length_ticks =
            region_get_full_length_in_ticks (
              r);
          long loop_length_ticks =
            region_get_loop_length_in_ticks (r);
          position_set_to_pos (
            &loop_start_adjusted,
            &r->loop_start_pos);
          position_set_to_pos (
            &loop_end_adjusted,
            &r->loop_end_pos);
          position_init (&region_end_adjusted);
          position_add_ticks (
            &local_pos, clip_start_ticks);
          position_add_ticks (
            &local_end_pos, clip_start_ticks);
          position_add_ticks (
            &loop_start_adjusted, - clip_start_ticks);
          position_add_ticks (
            &loop_end_adjusted, - clip_start_ticks);
          position_add_ticks (
            &region_end_adjusted, region_length_ticks);

          /* send all MIDI notes off if end if the
           * region (at loop point or actual end) is
           * within this cycle */
          if (position_compare (
                &region_end_adjusted,
                &local_pos) >= 0 &&
              position_compare (
                &region_end_adjusted,
                &local_end_pos) <= 0)
            {
              for (i = 0;
                   i < region->num_midi_notes;
                   i++)
                {
                  midi_note =
                    region->midi_notes[i];

                  /* check for note on event on the
                   * boundary */
                  if (position_is_before (
                        &midi_note->start_pos,
                        &region_end_adjusted) &&
                      position_is_after_or_equal (
                        &midi_note->end_pos,
                        &region_end_adjusted))
                    {
                      midi_events_add_note_off (
                        midi_events,
                        1, midi_note->val,
                        position_to_frames (
                          &region_end_adjusted) -
                          position_to_frames (
                            &local_pos), 1);
                    }
                }
            }
          /* if region actually ends on the timeline
           * within this cycle */
          else if (position_is_after_or_equal (
                     &r->end_pos,
                     pos) &&
                   position_is_before_or_equal (
                     &r->end_pos,
                     &end_pos))
            {
              for (i = 0;
                   i < region->num_midi_notes;
                   i++)
                {
                  midi_note =
                    region->midi_notes[i];

                  /* add num_loops * loop_ticks to the
                   * midi note start & end poses to get
                   * last pos in region */
                  Position tmp_start, tmp_end;
                  position_set_to_pos (
                    &tmp_start,
                    &midi_note->start_pos);
                  position_set_to_pos (
                    &tmp_end,
                    &midi_note->end_pos);
                  int num_loops =
                    region_get_num_loops (r, 0);
                  for (int jj = 0;
                       jj < num_loops;
                       jj++)
                    {
                      position_add_ticks (
                        &tmp_start,
                        loop_length_ticks);
                      position_add_ticks (
                        &tmp_end,
                        loop_length_ticks);
                    }

                  /* check for note on event on the
                   * boundary */
                  if (position_compare (
                        &tmp_start,
                        &region_end_adjusted) < 0 &&
                      position_compare (
                        &tmp_end,
                        &region_end_adjusted) >= 0)
                    {
                      midi_events_add_note_off (
                        midi_events, 1,
                        midi_note->val,
                        position_to_frames (
                          &r->end_pos) -
                          position_to_frames (pos),
                        1);
                    }
                }
            }

          /* if crossing loop
           * (local end pos will be back to the loop
           * start) */
          if (position_compare (
                &local_end_pos,
                &local_pos) <= 0)
            {
              for (i = 0;
                   i < region->num_midi_notes;
                   i++)
                {
                  midi_note =
                    region->midi_notes[i];

                  /* check for note on event on the
                   * boundary */
                  if (position_compare (
                        &midi_note->start_pos,
                        &loop_end_adjusted) < 0 &&
                      position_compare (
                        &midi_note->end_pos,
                        &loop_end_adjusted) >= 0)
                    {
                      midi_events_add_note_off (
                        midi_events, 1,
                        midi_note->val,
                        position_to_frames (
                          &loop_end_adjusted) -
                          position_to_frames (
                            &local_pos), 1);
                    }
                }
            }

          /* readjust position */
          position_add_ticks (
            &local_pos, - clip_start_ticks);
          position_add_ticks (
            &local_end_pos, - clip_start_ticks);

          for (i = 0;
               i < region->num_midi_notes;
               i++)
            {
              midi_note =
                region->midi_notes[i];

              /* check for note on event */
              if (position_compare (
                    &midi_note->start_pos,
                    &local_pos) >= 0 &&
                  position_compare (
                    &midi_note->start_pos,
                    &local_end_pos) <= 0)
                {
                  midi_events_add_note_on (
                    midi_events, 1,
                    midi_note->val,
                    midi_note->vel->vel,
                    position_to_frames (
                      &midi_note->start_pos) -
                      position_to_frames (
                        &local_pos), 1);
                }

              /* note off event */
              if (position_compare (
                    &midi_note->end_pos,
                    &local_pos) >= 0 &&
                  position_compare (
                    &midi_note->end_pos,
                    &local_end_pos) <= 0)
                {
                  midi_events_add_note_off (
                    midi_events, 1,
                    midi_note->val,
                    position_to_frames (
                      &midi_note->end_pos) -
                      position_to_frames (
                        &local_pos), 1);
                }
            }
        }
    }

  zix_sem_post (&midi_events->access_sem);
}

/**
 * Frees the track.
 *
 * TODO
 */
void
instrument_track_free (InstrumentTrack * track)
{
}
