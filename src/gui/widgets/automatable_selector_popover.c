/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/channel_track.h"
#include "gui/widgets/automatable_selector_button.h"
#include "gui/widgets/automatable_selector_popover.h"
#include "gui/widgets/automation_track.h"
#include "plugins/plugin.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (AutomatableSelectorPopoverWidget,
               automatable_selector_popover_widget,
               GTK_TYPE_POPOVER)

static int
update_info_label (
  AutomatableSelectorPopoverWidget * self,
  char * label)
{
  gtk_label_set_text (self->info, label);

  return G_SOURCE_REMOVE;
}

static GtkTreeModel *
create_model_for_automatables (
  AutomatableSelectorPopoverWidget * self,
  AutomatableSelectorType            type)
{
  GtkListStore *list_store;
  GtkTreeIter iter;

  /* file name, index */
  list_store =
    gtk_list_store_new (3,
                        G_TYPE_STRING,
                        G_TYPE_STRING,
                        G_TYPE_POINTER);

  Track * track =
    self->owner->owner->at->track;
  AutomationTrack * at;
  if (type == AS_TYPE_CHANNEL)
    {
      for (int i = 0;
           i < track->channel->num_ats; i++)
        {
          /* get selected automation track */
          at =
            track->channel->ats[i];

           /*if this automation track is not already*/
           /*in a visible lane*/
          if (!at->created || !at->visible ||
              at == self->owner->owner->at)
            {
              Automatable * a = at->automatable;

              // Add a new row to the model
              gtk_list_store_append (list_store, &iter);
              gtk_list_store_set (
                list_store, &iter,
                0, "z-text-x-csrc",
                1, a->label,
                2, a,
                -1);
            }
        }
    }
  else if (type > AS_TYPE_CHANNEL)
    {
      Plugin * plugin =
        track->channel->plugins[type - 1];

      if (plugin)
        {
          for (int j = 0;
               j < plugin->num_ats; j++)
            {
              /* get selected automation track */
              at =
                plugin->ats[j];

               /*if this automation track is not already*/
               /*in a visible lane*/
              if (!at->created || !at->visible ||
                  at == self->owner->owner->at)
                {
                  Automatable * a =
                    at->automatable;

                  // Add a new row to the model
                  gtk_list_store_append (
                    list_store, &iter);
                  gtk_list_store_set (
                    list_store, &iter,
                    0, "z-plugins",
                    1, a->label,
                    2, a,
                    -1);
                }
            }
        }
    }

  return GTK_TREE_MODEL (list_store);
}


static GtkTreeModel *
create_model_for_types (
  AutomatableSelectorPopoverWidget * self)
{
  GtkListStore *list_store;
  GtkTreeIter iter;

  /* icon, type name, type enum */
  list_store = gtk_list_store_new (3,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_INT);

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter,
    0, "z-text-x-csrc",
    1, "Channel",
    2, AS_TYPE_CHANNEL,
    -1);

  Track * track =
    self->owner->owner->at->track;

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      Plugin * plugin = track->channel->plugins[i];

      if (plugin)
        {
          char * label =
            g_strdup_printf (
              "[%d] %s",
              i,
              plugin->descr->name);
          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (
            list_store, &iter,
            0, "z-plugins",
            1, label,
            2, AS_TYPE_PLUGIN_0 + i,
            -1);
          g_free (label);
        }
    }


  return GTK_TREE_MODEL (list_store);
}

static GtkTreeView *
tree_view_create (
  AutomatableSelectorPopoverWidget * self,
  GtkTreeModel * model);

static void
on_selection_changed (
  GtkTreeSelection * ts,
  AutomatableSelectorPopoverWidget * self)
{
  GtkTreeView * tv =
    gtk_tree_selection_get_tree_view (ts);
  GtkTreeModel * model =
    gtk_tree_view_get_model (tv);
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (ts,
                                          NULL);
  if (selected_rows)
    {
      GtkTreePath * tp =
        (GtkTreePath *)
          g_list_first (selected_rows)->data;
      GtkTreeIter iter;
      gtk_tree_model_get_iter (model,
                               &iter,
                               tp);
      GValue value = G_VALUE_INIT;

      if (model == self->type_model)
        {
          gtk_tree_model_get_value (model,
                                    &iter,
                                    2,
                                    &value);
          self->selected_type =
            g_value_get_int (&value);
          self->automatable_model =
            create_model_for_automatables (
              self, self->selected_type);
          self->automatable_treeview =
            tree_view_create (
              self,
              self->automatable_model);
          z_gtk_container_destroy_all_children (
            GTK_CONTAINER (
              self->automatable_treeview_box));
          gtk_container_add (
            GTK_CONTAINER (
              self->automatable_treeview_box),
            GTK_WIDGET (
              self->automatable_treeview));

          self->selected_automatable = NULL;
          update_info_label (self,
                             "No control selected");
        }
      else if (model ==
                 self->automatable_model)
        {
          gtk_tree_model_get_value (model,
                                    &iter,
                                    2,
                                    &value);
          Automatable * a =
            g_value_get_pointer (&value);

          char * val_type =
            automatable_stringize_value_type (a);

          char * label =
            g_strdup_printf (
            "%s\nType: %s\nMin: %f\n"
            "Max: %f",
            a->label,
            val_type ? val_type : "Unknown",
            a->minf,
            a->maxf);

          self->selected_automatable = a;
          update_info_label (self,
                             label);
        }
    }
}

static GtkTreeView *
tree_view_create (
  AutomatableSelectorPopoverWidget * self,
  GtkTreeModel * model)
{
  /* instantiate tree view using model */
  GtkWidget * tree_view =
    gtk_tree_view_new_with_model (
      GTK_TREE_MODEL (model));

  /* init tree view */
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;

  /* column for icon */
  renderer =
    gtk_cell_renderer_pixbuf_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      "icon", renderer,
      "icon-name", 0,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view),
    column);

  /* column for name */
  renderer =
    gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      "name", renderer,
      "text", 1,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view),
    column);

  /* set search column */
  gtk_tree_view_set_search_column (
    GTK_TREE_VIEW (tree_view),
    1);

  /* set headers invisible */
  gtk_tree_view_set_headers_visible (
            GTK_TREE_VIEW (tree_view),
            0);

  g_signal_connect (
    G_OBJECT (gtk_tree_view_get_selection (
                GTK_TREE_VIEW (tree_view))),
    "changed",
     G_CALLBACK (on_selection_changed),
     self);

  gtk_widget_set_visible (tree_view, 1);

  return GTK_TREE_VIEW (tree_view);
}

static void
on_closed (AutomatableSelectorPopoverWidget *self,
               gpointer    user_data)
{
  automatable_selector_button_set_automatable (
    self->owner, self->selected_automatable);
  automatable_selector_button_widget_refresh (
    self->owner);
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
AutomatableSelectorPopoverWidget *
automatable_selector_popover_widget_new (
  AutomatableSelectorButtonWidget * owner)
{
  AutomatableSelectorPopoverWidget * self =
    g_object_new (
      AUTOMATABLE_SELECTOR_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;
  self->type_model =
    create_model_for_types (self);
  self->type_treeview =
    tree_view_create (
      self,
      self->type_model);
  gtk_container_add (
    GTK_CONTAINER (self->type_treeview_box),
    GTK_WIDGET (self->type_treeview));

  AutomatableSelectorType type =
    AS_TYPE_CHANNEL;
  Automatable * a =
    self->owner->owner->at->automatable;
  if (a->type >= AUTOMATABLE_TYPE_CHANNEL_FADER)
    type = AS_TYPE_CHANNEL;
  else if (a->slot > -1)
    type = AS_TYPE_PLUGIN_0 + a->slot;

  self->automatable_model =
    create_model_for_automatables (self, type);
  self->automatable_treeview =
    tree_view_create (
      self,
      self->automatable_model);
  gtk_container_add (
    GTK_CONTAINER (self->automatable_treeview_box),
    GTK_WIDGET (self->automatable_treeview));

  update_info_label (self,
                     "No control selected");

  return self;
}

static void
automatable_selector_popover_widget_class_init (
  AutomatableSelectorPopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "automatable_selector.ui");

  gtk_widget_class_bind_template_child (
    klass,
    AutomatableSelectorPopoverWidget,
    type_treeview_box);
  gtk_widget_class_bind_template_child (
    klass,
    AutomatableSelectorPopoverWidget,
    automatable_treeview_box);
  gtk_widget_class_bind_template_child (
    klass,
    AutomatableSelectorPopoverWidget,
    info);
  gtk_widget_class_bind_template_callback (
    klass,
    on_closed);
}

static void
automatable_selector_popover_widget_init (
  AutomatableSelectorPopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
