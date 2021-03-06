/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * @file
 *
 * Audio clip editor backend.
 */

#ifndef __AUDIO_AUDIO_CLIP_EDITOR_H__
#define __AUDIO_AUDIO_CLIP_EDITOR_H__

#include "gui/backend/editor_settings.h"
#include "utils/yaml.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUDIO_CLIP_EDITOR \
  (&CLIP_EDITOR->audio_clip_editor)

/**
 * Audio clip editor serializable backend.
 *
 * The actual widgets should reflect the
 * information here.
 */
typedef struct AudioClipEditor
{
  EditorSettings  editor_settings;
} AudioClipEditor;

static const cyaml_schema_field_t
audio_clip_editor_fields_schema[] =
{
  YAML_FIELD_MAPPING_EMBEDDED (
    AudioClipEditor, editor_settings,
    editor_settings_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
audio_clip_editor_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AudioClipEditor,
    audio_clip_editor_fields_schema),
};

void
audio_clip_editor_init (
  AudioClipEditor * self);

/**
 * @}
 */

#endif
