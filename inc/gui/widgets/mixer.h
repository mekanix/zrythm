/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_MIXER_H__
#define __GUI_WIDGETS_MIXER_H__

#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

#define MIXER_WIDGET_TYPE \
  (mixer_widget_get_type ())
G_DECLARE_FINAL_TYPE (MixerWidget,
                      mixer_widget,
                      Z,
                      MIXER_WIDGET,
                      GtkBox)

#define MW_MIXER MW_BOT_DOCK_EDGE->mixer

typedef struct _DragDestBoxWidget DragDestBoxWidget;
typedef struct Channel Channel;

typedef struct _MixerWidget
{
  GtkBox                   parent_instance;
  DragDestBoxWidget *      ddbox;  ///< for DNDing plugins

  /**
   * Box containing all channels except master.
   */
  GtkBox *                 channels_box;

  /** The track to drop before, used in
   * drag-data-get.
   *
   * If this is null it means the last position
   * (after invisible tracks). */
  //Track *             drop_before;

  /**
   * The track where dnd originated from.
   *
   * Used to decide if left/right highlight means
   * the track should be dropped before or after.
   */
  Track *             start_drag_track;

  GtkButton *              channels_add;
  GtkBox *                 master_box;
} MixerWidget;

/**
 * To be called once.
 */
void
mixer_widget_setup (MixerWidget * self,
                    Channel *     master);

/**
 * Deletes and readds all channels.
 *
 * To be used sparingly.
 */
void
mixer_widget_hard_refresh (MixerWidget * self);

/**
 * Calls refresh on each channel.
 */
void
mixer_widget_soft_refresh (MixerWidget * self);

#endif
