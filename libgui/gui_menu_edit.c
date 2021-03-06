/*
*  gui_menu_edit.c
*  Source code file for G-Code generation, simulation, and visualization
*  library. This software is Copyright (C) 2006 by Justin Shumaker
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "gui_menu_edit.h"
#include "gui_menu_util.h"
#include "gui.h"
#include "gui_define.h"
#include "gui_machines.h"
#include "gui_tab.h"


void
gui_menu_edit_remove_menuitem_callback (GtkWidget *widget, gpointer data)
{
  gcode_block_t *selected_block;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreePath *path;
  gui_t *gui;

  gui = (gui_t *) data;

  get_selected_block (gui, &selected_block, &iter);
  if (!selected_block)
    return;

  /* Removing locked blocks is prohibited */
  if (selected_block->flags & GCODE_FLAGS_LOCK)
    return;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (gui->gcode_block_treeview));
  path = gtk_tree_model_get_path (model, &iter);

  if (selected_block->prev)
  {
    gtk_tree_path_prev (path);
  }
  else if (selected_block->next)
  {
    gtk_tree_path_next (path);
  }
  else
  {
    gtk_tree_path_up (path);
  }

  gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (gui->gcode_block_treeview)), path);

  gcode_list_remove (selected_block);
  gtk_tree_store_remove (gui->gcode_block_store, &iter);

  update_block_tree_order (gui);

  get_selected_block (gui, &selected_block, &iter);
  gui_tab_display (gui, selected_block, 0);
  update_menu_options (gui, selected_block);

  gui->opengl.rebuild_view_display_list = 1;
  gui_opengl_context_redraw (&gui->opengl, selected_block);

  gtk_tree_path_free (path);

  gui_menu_util_modified (gui, 1);
}


static void
generate_pattern_callback (GtkWidget *widget, gpointer data)
{
  GtkWidget **wlist;
  GtkTreeIter iter, parent_iter, child_iter;
  gcode_block_t *selected_block, *block_ind;
  gui_t *gui;
  int i;

  wlist = (GtkWidget **) data;

  gui = (gui_t *) wlist[0];

  get_selected_block (gui, &selected_block, &iter);

  if (selected_block->type == GCODE_TYPE_SKETCH)
  {
    gcode_sketch_pattern (selected_block,
			  gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[2])),
			  gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[3])),
			  gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[4])),
			  gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[5])),
			  gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[6])),
			  gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[7])));
  }
  else if (selected_block->type == GCODE_TYPE_DRILL_HOLES)
  {
    gcode_drill_holes_pattern (selected_block,
			       gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[2])),
			       gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[3])),
			       gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[4])),
			       gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[5])),
			       gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[6])),
			       gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[7])));
  }

  /* Destroy the window along with all of its widgets */
  gtk_widget_destroy (wlist[1]);

  free (wlist);

  i = 0;
  for (block_ind = selected_block; block_ind; block_ind = block_ind->prev)
    i++;

  gtk_tree_model_iter_parent (gtk_tree_view_get_model (GTK_TREE_VIEW (gui->gcode_block_treeview)), &parent_iter, &iter);

  if (gtk_tree_store_iter_is_valid (gui->gcode_block_store, &parent_iter))
  {
    child_iter = refresh_gcode_block_tree_recursion (gui, selected_block, &parent_iter, i, 1);
  }
  else
  {
    child_iter = refresh_gcode_block_tree_recursion (gui, selected_block, NULL, i, 1);
  }

  set_selected_row_with_iter (gui, &child_iter);

  gtk_tree_store_remove (gui->gcode_block_store, &iter);

  gui->opengl.rebuild_view_display_list = 1;
  gui_opengl_context_redraw (&gui->opengl, selected_block);

  gui_menu_util_modified (gui, 1);
}


void
gui_menu_edit_pattern_menuitem_callback (GtkWidget *widget, gpointer data)
{
  GtkWidget *window;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *iterations_spin;
  GtkWidget *translatex_spin;
  GtkWidget *translatey_spin;
  GtkWidget *rotate_aboutx_spin;
  GtkWidget *rotate_abouty_spin;
  GtkWidget *rotation_spin;
  GtkWidget *generate_button;
  GtkWidget **wlist;
  gui_t *gui;
  int row = 0;

  gui = (gui_t *) data;

  wlist = (GtkWidget **) malloc (8 * sizeof (GtkWidget *));

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gui->window));
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
  gtk_window_set_title (GTK_WINDOW (window), "Pattern");
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  
  table = gtk_table_new (8, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), TABLE_SPACING);
  gtk_table_set_row_spacings (GTK_TABLE (table), TABLE_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (window), table);

  label = gtk_label_new ("Iterations");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
  iterations_spin = gtk_spin_button_new_with_range (1, 100, 1);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (iterations_spin), 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (iterations_spin), 1);
  gtk_table_attach_defaults (GTK_TABLE (table), iterations_spin, 1, 2, row, row+1);
  row++;

  label = gtk_label_new ("Translate(X)");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
  translatex_spin = gtk_spin_button_new_with_range (-MAX_DIM_X, MAX_DIM_X, 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (translatex_spin), 5);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (translatex_spin), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), translatex_spin, 1, 2, row, row+1);
  row++;

  label = gtk_label_new ("Translate(Y)");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
  translatey_spin = gtk_spin_button_new_with_range (-MAX_DIM_Y, MAX_DIM_Y, 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (translatey_spin), 5);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (translatey_spin), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), translatey_spin, 1, 2, row, row+1);
  row++;

  label = gtk_label_new ("Rotate About(X)");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
  rotate_aboutx_spin = gtk_spin_button_new_with_range (-MAX_DIM_X, MAX_DIM_X, 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (rotate_aboutx_spin), 5);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (rotate_aboutx_spin), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), rotate_aboutx_spin, 1, 2, row, row+1);
  row++;

  label = gtk_label_new ("Rotate About(Y)");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
  rotate_abouty_spin = gtk_spin_button_new_with_range (-MAX_DIM_Y, MAX_DIM_Y, 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (rotate_abouty_spin), 5);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (rotate_abouty_spin), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), rotate_abouty_spin, 1, 2, row, row+1);
  row++;

  label = gtk_label_new ("Rotation");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
  rotation_spin = gtk_spin_button_new_with_range (-180.0, 180.0, 0.1);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (rotation_spin), 5);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (rotation_spin), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), rotation_spin, 1, 2, row, row+1);
  row++;

  wlist[0] = (GtkWidget *) gui;
  wlist[1] = window;
  wlist[2] = iterations_spin;
  wlist[3] = translatex_spin;
  wlist[4] = translatey_spin;
  wlist[5] = rotate_aboutx_spin;
  wlist[6] = rotate_abouty_spin;
  wlist[7] = rotation_spin;

  generate_button = gtk_button_new_with_label ("Generate");
  gtk_table_attach_defaults (GTK_TABLE (table), generate_button, 0, 2, row, row+1);
  g_signal_connect (G_OBJECT (generate_button), "clicked", G_CALLBACK (generate_pattern_callback), wlist);
  gtk_widget_show_all (window);
}


void
gui_menu_edit_flip_direction_menuitem_callback (GtkWidget *widget, gpointer data)
{
  gcode_block_t *selected_block;
  GtkTreeIter iter;
  gui_t *gui;

  gui = (gui_t *) data;

  get_selected_block (gui, &selected_block, &iter);

  if (selected_block->type == GCODE_TYPE_LINE)
  {
    gcode_line_flip_direction (selected_block);
  }
  else if (selected_block->type == GCODE_TYPE_ARC)
  {
    gcode_arc_flip_direction (selected_block);
  }

  update_menu_options (gui, selected_block);
  gui_tab_display (gui, selected_block, 1);

  gui->opengl.rebuild_view_display_list = 1;
  gui_opengl_context_redraw (&gui->opengl, selected_block);

  gui_menu_util_modified (gui, 1);
}


static void
update_project_callback (GtkWidget *widget, gpointer data)
{
  GtkWidget **wlist;
  GtkTreeIter iter;
  gcode_block_t *selected_block;
  gui_t *gui;
  gfloat_t material_size[3], material_origin[3];
  uint8_t units, material_type;
  char *sp, name[32];


  wlist = (GtkWidget **) data;

  gui = (gui_t *) wlist[0];

  strcpy (name, gtk_entry_get_text (GTK_ENTRY (wlist[2])));

  sp = gtk_combo_box_get_active_text (GTK_COMBO_BOX (wlist[3]));
  if (!strcmp (sp, "inch"))
  {
    units = GCODE_UNITS_INCH;
  }
  else
  {
    units = GCODE_UNITS_MILLIMETER;
  }

  sp = gtk_combo_box_get_active_text (GTK_COMBO_BOX (wlist[4]));
  if (!strcmp (sp, "aluminum"))
  {
    material_type = GCODE_MATERIAL_ALUMINUM;
  }
  else if (!strcmp (sp, "foam"))
  {
    material_type = GCODE_MATERIAL_FOAM;
  }
  else if (!strcmp (sp, "plastic"))
  {
    material_type = GCODE_MATERIAL_PLASTIC;
  }
  else if (!strcmp (sp, "steel"))
  {
    material_type = GCODE_MATERIAL_STEEL;
  }
  else if (!strcmp (sp, "wood"))
  {
    material_type = GCODE_MATERIAL_WOOD;
  }
  else
  {
    /* Set default to steel since it's the safest choice */
    material_type = GCODE_MATERIAL_STEEL;
  }

  material_size[0] = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[5]));
  material_size[1] = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[6]));
  material_size[2] = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[7]));

  material_origin[0] = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[8]));
  material_origin[1] = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[9]));
  material_origin[2] = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[10]));


  /* Machine Name */
  sp = gtk_combo_box_get_active_text (GTK_COMBO_BOX (wlist[11]));
  {
    gui_machine_list_t machine_list;
    int i;

    gui_machines_init (&machine_list);
    gui_machines_read (&machine_list);

    for (i = 0; i < machine_list.num; i++)
    {
      if (!strcmp (sp, machine_list.machine[i].name))
      {
        strcpy (gui->gcode.machine_name, machine_list.machine[i].name);
        gui->gcode.machine_options = machine_list.machine[i].options;
      }
    }

    gui_machines_free (&machine_list);
  }

  gui->gcode.ztraverse = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[12]));

  {
    GtkTextIter start_iter, end_iter;

    gtk_text_buffer_get_start_iter (gtk_text_view_get_buffer (GTK_TEXT_VIEW (wlist[13])), &start_iter);
    gtk_text_buffer_get_end_iter (gtk_text_view_get_buffer (GTK_TEXT_VIEW (wlist[13])), &end_iter);

    strcpy (gui->gcode.notes, gtk_text_buffer_get_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (wlist[13])), &start_iter, &end_iter, FALSE));
  }

  /* Destroy the window along with all of its widgets */
  gtk_widget_destroy (wlist[1]);

  free (wlist);


  /*
  * Update G-Code Settings.
  */
  strcpy (gui->gcode.name, name);
  gui->gcode.units = units;
  gui->gcode.material_type = material_type;
  gui->gcode.material_size[0] = material_size[0];
  gui->gcode.material_size[1] = material_size[1];
  gui->gcode.material_size[2] = material_size[2];

  gui->gcode.material_origin[0] = material_origin[0];
  gui->gcode.material_origin[1] = material_origin[1];
  gui->gcode.material_origin[2] = material_origin[2];

  /*
  * Update OpenGL
  */
  gui_opengl_build_gridxy_display_list (&gui->opengl);
  gui_opengl_build_gridxz_display_list (&gui->opengl);

  gui->opengl.view[GUI_OPENGL_VIEW_REGULAR].zoom = gui->gcode.material_size[0] * 0.66 * (gfloat_t) gui->opengl.context_w / (gfloat_t) gui->opengl.context_h;
  gui->opengl.view[GUI_OPENGL_VIEW_REGULAR].grid = gui->opengl.view[GUI_OPENGL_VIEW_REGULAR].zoom;

  gcode_prep (&gui->gcode);

  gui->opengl.view[GUI_OPENGL_VIEW_REGULAR].zoom = gui->gcode.material_size[0] * 0.66 * (gfloat_t) gui->opengl.context_w / (gfloat_t) gui->opengl.context_h;
  gui->opengl.view[GUI_OPENGL_VIEW_REGULAR].grid = gui->opengl.view[GUI_OPENGL_VIEW_REGULAR].zoom;
  gui_opengl_build_gridxy_display_list (&gui->opengl);
  gui_opengl_build_gridxz_display_list (&gui->opengl);

  gui->opengl.rebuild_view_display_list = 1;
  gui_opengl_context_redraw (&gui->opengl, NULL);

  gui_menu_util_modified (gui, 1);

  get_selected_block (gui, &selected_block, &iter);
  gui_tab_display (gui, selected_block, 1);
}


void
gui_menu_edit_project_settings_menuitem_callback (GtkWidget *widget, gpointer data)
{
  GtkWidget *window;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *name_entry;
  GtkWidget *base_unit_combo;
  GtkWidget *material_type_combo;
  GtkWidget *material_sizex_spin;
  GtkWidget *material_sizey_spin;
  GtkWidget *material_sizez_spin;
  GtkWidget *material_originx_spin;
  GtkWidget *material_originy_spin;
  GtkWidget *material_originz_spin;
  GtkWidget *machine_combo;
  GtkWidget *ztraverse_spin;
  GtkWidget *notes_sw;
  GtkWidget *notes_textview;
  GtkWidget *update_button;
  GtkWidget **wlist;
  gui_t *gui;

  gui = (gui_t *) data;

  wlist = (GtkWidget **) malloc (14 * sizeof (GtkWidget *));

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gui->window));
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
  gtk_window_set_title (GTK_WINDOW (window), "Project Settings");
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  
  table = gtk_table_new (8, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), TABLE_SPACING);
  gtk_table_set_row_spacings (GTK_TABLE (table), TABLE_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (window), table);

  label = gtk_label_new ("Project Name");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  name_entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (name_entry), 32);
  gtk_table_attach_defaults (GTK_TABLE (table), name_entry, 1, 4, 0, 1);
  gtk_entry_set_text (GTK_ENTRY (name_entry), gui->gcode.name);

  label = gtk_label_new ("Base Unit");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  base_unit_combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (base_unit_combo), "inch");
  gtk_combo_box_append_text (GTK_COMBO_BOX (base_unit_combo), "millimeter");
  gtk_combo_box_set_active (GTK_COMBO_BOX (base_unit_combo), gui->gcode.units);
  g_signal_connect_swapped (base_unit_combo, "changed", G_CALLBACK (base_unit_changed_callback), wlist);
  gtk_table_attach_defaults (GTK_TABLE (table), base_unit_combo, 1, 4, 1, 2);

  label = gtk_label_new ("Material Type");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  material_type_combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (material_type_combo), "aluminum");
  gtk_combo_box_append_text (GTK_COMBO_BOX (material_type_combo), "foam");
  gtk_combo_box_append_text (GTK_COMBO_BOX (material_type_combo), "plastic");
  gtk_combo_box_append_text (GTK_COMBO_BOX (material_type_combo), "steel");
  gtk_combo_box_append_text (GTK_COMBO_BOX (material_type_combo), "wood");
  gtk_combo_box_set_active (GTK_COMBO_BOX (material_type_combo), gui->gcode.material_type);
  gtk_table_attach_defaults (GTK_TABLE (table), material_type_combo, 1, 4, 2, 3);

  label = gtk_label_new ("Material Size (XYZ)");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);

  material_sizex_spin = gtk_spin_button_new_with_range (0.01, MAX_DIM_X, 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (material_sizex_spin), 3);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (material_sizex_spin), gui->gcode.material_size[0]);
  gtk_table_attach_defaults (GTK_TABLE (table), material_sizex_spin, 1, 2, 3, 4);

  material_sizey_spin = gtk_spin_button_new_with_range (0.01, MAX_DIM_Y, 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (material_sizey_spin), 3);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (material_sizey_spin), gui->gcode.material_size[1]);
  gtk_table_attach_defaults (GTK_TABLE (table), material_sizey_spin, 2, 3, 3, 4);

  material_sizez_spin = gtk_spin_button_new_with_range (0.01, MAX_DIM_Z, 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (material_sizez_spin), 3);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (material_sizez_spin), gui->gcode.material_size[2]);
  gtk_table_attach_defaults (GTK_TABLE (table), material_sizez_spin, 3, 4, 3, 4);

  label = gtk_label_new ("Material Origin (XYZ)");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);

  material_originx_spin = gtk_spin_button_new_with_range (0.0, MAX_DIM_X, 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (material_originx_spin), 3);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (material_originx_spin), gui->gcode.material_origin[0]);
  gtk_table_attach_defaults (GTK_TABLE (table), material_originx_spin, 1, 2, 4, 5);

  material_originy_spin = gtk_spin_button_new_with_range (0.0, MAX_DIM_Y, 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (material_originy_spin), 3);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (material_originy_spin), gui->gcode.material_origin[1]);
  gtk_table_attach_defaults (GTK_TABLE (table), material_originy_spin, 2, 3, 4, 5);

  material_originz_spin = gtk_spin_button_new_with_range (-MAX_DIM_Z, MAX_DIM_Z, 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (material_originz_spin), 3);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (material_originz_spin), gui->gcode.material_origin[2]);
  gtk_table_attach_defaults (GTK_TABLE (table), material_originz_spin, 3, 4, 4, 5);

  {
    gui_machine_list_t machine_list;
    int i;

    gui_machines_init (&machine_list);
    gui_machines_read (&machine_list);

    label = gtk_label_new ("Machine");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 5, 6);
    machine_combo = gtk_combo_box_new_text ();

    /* Current name is first if one exists */
    if (strcmp (gui->gcode.machine_name, ""))
      gtk_combo_box_append_text (GTK_COMBO_BOX (machine_combo), gui->gcode.machine_name);

    for (i = 0; i < machine_list.num; i++)
      if (strcmp (gui->gcode.machine_name, machine_list.machine[i].name))
        gtk_combo_box_append_text (GTK_COMBO_BOX (machine_combo), machine_list.machine[i].name);

    gtk_combo_box_set_active (GTK_COMBO_BOX (machine_combo), 0);
    gtk_table_attach_defaults (GTK_TABLE (table), machine_combo, 1, 4, 5, 6);

    gui_machines_free (&machine_list);
  }

  label = gtk_label_new ("Traverse(Z)");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 6, 7);
  ztraverse_spin = gtk_spin_button_new_with_range (0.0, GCODE_UNITS ((&gui->gcode), 1.0), 0.01);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (ztraverse_spin), 2);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (ztraverse_spin), gui->gcode.ztraverse);
  gtk_table_attach_defaults (GTK_TABLE (table), ztraverse_spin, 1, 4, 6, 7);

  label = gtk_label_new ("Notes");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 7, 8);

  notes_sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (notes_sw), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (notes_sw), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_table_attach_defaults (GTK_TABLE (table), notes_sw, 1, 4, 7, 8);

  notes_textview = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (notes_textview), GTK_WRAP_WORD_CHAR);

  gtk_text_buffer_insert_at_cursor (gtk_text_view_get_buffer (GTK_TEXT_VIEW (notes_textview)), gui->gcode.notes, -1);

  gtk_container_add (GTK_CONTAINER (notes_sw), notes_textview);

  wlist[0] = (GtkWidget *) gui;
  wlist[1] = window;
  wlist[2] = name_entry;
  wlist[3] = base_unit_combo;
  wlist[4] = material_type_combo;
  wlist[5] = material_sizex_spin;
  wlist[6] = material_sizey_spin;
  wlist[7] = material_sizez_spin;
  wlist[8] = material_originx_spin;
  wlist[9] = material_originy_spin;
  wlist[10] = material_originz_spin;
  wlist[11] = machine_combo;
  wlist[12] = ztraverse_spin;
  wlist[13] = notes_textview;

  /* Because metric may already be selected and the material sizes do not reflect this as of yet */
  base_unit_changed_callback (wlist, 0);

  update_button = gtk_button_new_with_label ("Update Settings");
  gtk_table_attach_defaults (GTK_TABLE (table), update_button, 1, 4, 8, 9);
  g_signal_connect (G_OBJECT (update_button), "clicked", G_CALLBACK (update_project_callback), wlist);
  gtk_widget_show_all (window);
}


void
gui_menu_edit_duplicate_menuitem_callback (GtkWidget *widget, gpointer data)
{
  gcode_block_t *selected_block, *duplicate_block;
  GtkTreeIter iter;
  gui_t *gui;

  gui = (gui_t *) data;

  get_selected_block (gui, &selected_block, &iter);
  if (selected_block)
    if (selected_block->duplicate)
      selected_block->duplicate (selected_block, &duplicate_block);

  insert_primitive (gui, duplicate_block, selected_block, &iter, GUI_INSERT_AFTER);

  gui_menu_util_modified (gui, 1);
#if 0
  refresh_gcode_block_tree ();

  gui_opengl_context_redraw (&gui->opengl, duplicate_block);
#endif
}


static void
scale_on_assistant_close_cancel (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy (widget);
}


static void
scale_on_assistant_prepare (GtkWidget *widget, GtkWidget *page, gpointer data)
{
  gint current_page, n_pages;
  gchar *title;

  current_page = gtk_assistant_get_current_page (GTK_ASSISTANT (widget));

  gtk_assistant_set_page_complete (GTK_ASSISTANT (widget),  gtk_assistant_get_nth_page (GTK_ASSISTANT (widget), 0), TRUE);

  n_pages = gtk_assistant_get_n_pages (GTK_ASSISTANT (widget));

  title = g_strdup_printf ("Scale");
  gtk_window_set_title (GTK_WINDOW (widget), title);
  g_free (title);
}


static void
scale_on_assistant_apply (GtkWidget *widget, gpointer data)
{
  gfloat_t factor;
  GtkTreeIter iter;
  gcode_block_t *selected_block;
  gui_t *gui;

  gui = (gui_t *) data;

  get_selected_block (gui, &selected_block, &iter);
  factor = gtk_spin_button_get_value (GTK_SPIN_BUTTON (((GtkWidget **) gui->generic_ptr)[0]));

  selected_block->scale (selected_block, factor);

  gui->opengl.rebuild_view_display_list = 1;
  gui_opengl_context_redraw (&gui->opengl, selected_block);

  gui_menu_util_modified (gui, 1);

  free (gui->generic_ptr);

  gui_tab_display (gui, selected_block, 1);
}


static void
scale_create_page1 (gui_t *gui, GtkWidget *assistant)
{
  gcode_block_t *selected_block;
  GtkWidget *hbox, *label, *factor_spin;
  GdkPixbuf *pixbuf;
  GtkTreeIter iter;

  get_selected_block (gui, &selected_block, &iter);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);

  label = gtk_label_new ("Factor");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  factor_spin = gtk_spin_button_new_with_range (0.0001, GCODE_UNITS ((&gui->gcode), 10.0), 0.001);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (factor_spin), MANTISSA);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (factor_spin), 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), factor_spin, FALSE, FALSE, 0);
  ((GtkWidget **) gui->generic_ptr)[0] = factor_spin;

  gtk_widget_show_all (hbox);

  gtk_assistant_append_page (GTK_ASSISTANT (assistant), hbox);
  gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), hbox, "Scale");
  gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), hbox, GTK_ASSISTANT_PAGE_CONFIRM);

  pixbuf = gtk_widget_render_icon (assistant, GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG, NULL);
  gtk_assistant_set_page_header_image (GTK_ASSISTANT (assistant), hbox, pixbuf);
  g_object_unref (pixbuf);
}


void
gui_menu_edit_scale_menuitem_callback (GtkWidget *widget, gpointer data)
{
  GtkWidget *assistant;
  gui_t *gui;

  gui = (gui_t *) data;

  assistant = gtk_assistant_new ();
  gtk_window_set_default_size (GTK_WINDOW (assistant), -1, -1);
  gtk_window_set_screen (GTK_WINDOW (assistant), gtk_widget_get_screen (gui->window));
  gtk_window_set_transient_for (GTK_WINDOW (assistant), GTK_WINDOW (gui->window));

  /* Setup Global Widgets */
  gui->generic_ptr = malloc (sizeof (GtkWidget *));

  scale_create_page1 (gui, assistant);

  g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (scale_on_assistant_close_cancel), &assistant);
  g_signal_connect (G_OBJECT (assistant), "close", G_CALLBACK (scale_on_assistant_close_cancel), &assistant);
  g_signal_connect (G_OBJECT (assistant), "prepare", G_CALLBACK (scale_on_assistant_prepare), NULL);
  g_signal_connect (G_OBJECT (assistant), "apply", G_CALLBACK (scale_on_assistant_apply), gui);

  gtk_widget_show (assistant);
}


void
gui_menu_edit_join_previous_menuitem_callback (GtkWidget *widget, gpointer data)
{
  gcode_block_t *selected_block;
  GtkTreeIter iter;
  gui_t *gui;

  gui = (gui_t *) data;

  get_selected_block (gui, &selected_block, &iter);
  if (selected_block)
  {
    gfloat_t p0[2], p1[2], t0[2], t1[2];

    if (!selected_block->ends)
      return;
        
    selected_block->ends (selected_block, p0, p1, GCODE_GET);
    if (selected_block->prev)
    {
      if (selected_block->prev->ends)
      {
        selected_block->prev->ends (selected_block->prev, t0, t1, GCODE_GET);
        selected_block->prev->ends (selected_block->prev, t0, p0, GCODE_SET);
      }
    }
  }

  gui->opengl.rebuild_view_display_list = 1;
  gui_opengl_context_redraw (&gui->opengl, selected_block);

  gui_menu_util_modified (gui, 1);
}


void
gui_menu_edit_join_next_menuitem_callback (GtkWidget *widget, gpointer data)
{
  gcode_block_t *selected_block;
  GtkTreeIter iter;
  gui_t *gui;

  gui = (gui_t *) data;

  get_selected_block (gui, &selected_block, &iter);
  if (selected_block)
  {
    gfloat_t p0[2], p1[2], t0[2], t1[2];

    if (!selected_block->ends)
      return;
        
    selected_block->ends (selected_block, p0, p1, GCODE_GET);
    if (selected_block->next)
    {
      if (selected_block->next->ends)
      {
        selected_block->next->ends (selected_block->next, t0, t1, GCODE_GET);
        selected_block->next->ends (selected_block->next, p1, t1, GCODE_SET);
      }
    }
  }

  gui->opengl.rebuild_view_display_list = 1;
  gui_opengl_context_redraw (&gui->opengl, selected_block);
}


static void
fillet_on_assistant_close_cancel (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy (widget);
}


static void
fillet_on_assistant_prepare (GtkWidget *widget, GtkWidget *page, gpointer data)
{
  gint current_page, n_pages;
  gchar *title;

  current_page = gtk_assistant_get_current_page (GTK_ASSISTANT (widget));

  gtk_assistant_set_page_complete (GTK_ASSISTANT (widget),  gtk_assistant_get_nth_page (GTK_ASSISTANT (widget), 0), TRUE);

  n_pages = gtk_assistant_get_n_pages (GTK_ASSISTANT (widget));

  title = g_strdup_printf ("Fillet");
  gtk_window_set_title (GTK_WINDOW (widget), title);
  g_free (title);
}


static void
fillet_previous_on_assistant_apply (GtkWidget *widget, gpointer data)
{
  gfloat_t radius;
  gcode_block_t *selected_block, *prev_connected_block, *fillet_block;
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreeModel *model;
  gui_t *gui;

  gui = (gui_t *) data;

  get_selected_block (gui, &selected_block, &iter);
  radius = gtk_spin_button_get_value (GTK_SPIN_BUTTON (((GtkWidget **) gui->generic_ptr)[0]));

  prev_connected_block = gcode_sketch_prev_connected (selected_block);

  gcode_arc_init (&gui->gcode, &fillet_block, selected_block->parent);

  gcode_util_fillet (prev_connected_block, selected_block, fillet_block, radius);

  insert_primitive (gui, fillet_block, prev_connected_block, &iter, GUI_INSERT_AFTER);

  fillet_block->offset = selected_block->offset;

  if (selected_block->prev)
  {
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (gui->gcode_block_treeview));
    path = gtk_tree_model_get_path (model, &iter);
    gtk_tree_path_prev (path);
    gtk_tree_path_prev (path);
    gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (gui->gcode_block_treeview)), path);

    get_selected_block (gui, &selected_block, &iter);
    update_menu_options (gui, selected_block);

    gui->opengl.rebuild_view_display_list = 1;
    gui_opengl_context_redraw (&gui->opengl, selected_block);
  }

  gui_menu_util_modified (gui, 1);

  free (gui->generic_ptr);
}


static void
fillet_next_on_assistant_apply (GtkWidget *widget, gpointer data)
{
  gfloat_t radius;
  gcode_block_t *selected_block, *next_connected_block, *fillet_block;
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreeModel *model;
  gui_t *gui;

  gui = (gui_t *) data;

  get_selected_block (gui, &selected_block, &iter);
  radius = gtk_spin_button_get_value (GTK_SPIN_BUTTON (((GtkWidget **) gui->generic_ptr)[0]));

  next_connected_block = gcode_sketch_next_connected (selected_block);

  gcode_arc_init (&gui->gcode, &fillet_block, selected_block->parent);

  gcode_util_fillet (selected_block, next_connected_block, fillet_block, radius);

  insert_primitive (gui, fillet_block, selected_block, &iter, GUI_INSERT_AFTER);

  fillet_block->offset = selected_block->offset;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (gui->gcode_block_treeview));
  path = gtk_tree_model_get_path (model, &iter);
  gtk_tree_path_next (path);
  gtk_tree_path_next (path);
  gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (gui->gcode_block_treeview)), path);

  get_selected_block (gui, &selected_block, &iter);
  update_menu_options (gui, selected_block);

  gui->opengl.rebuild_view_display_list = 1;
  gui_opengl_context_redraw (&gui->opengl, selected_block);

  gui_menu_util_modified (gui, 1);

  free (gui->generic_ptr);
}


static void
fillet_create_page1 (gui_t *gui, GtkWidget *assistant)
{
  gcode_block_t *selected_block;
  GtkWidget *hbox, *label, *radius_spin;
  GdkPixbuf *pixbuf;
  gcode_tool_t *tool;
  GtkTreeIter iter;

  get_selected_block (gui, &selected_block, &iter);
  tool = gcode_tool_find (selected_block);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);

  label = gtk_label_new ("Radius");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  radius_spin = gtk_spin_button_new_with_range (0.0001, GCODE_UNITS ((&gui->gcode), 10.0), 0.001);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (radius_spin), MANTISSA);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (radius_spin), GCODE_UNITS ((&gui->gcode), 0.5*tool->diam));
  gtk_box_pack_start (GTK_BOX (hbox), radius_spin, FALSE, FALSE, 0);
  ((GtkWidget **) gui->generic_ptr)[0] = radius_spin;

  gtk_widget_show_all (hbox);

  gtk_assistant_append_page (GTK_ASSISTANT (assistant), hbox);
  gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), hbox, "Fillet");
  gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), hbox, GTK_ASSISTANT_PAGE_CONFIRM);

  pixbuf = gtk_widget_render_icon (assistant, GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG, NULL);
  gtk_assistant_set_page_header_image (GTK_ASSISTANT (assistant), hbox, pixbuf);
  g_object_unref (pixbuf);
}


void
gui_menu_edit_fillet_previous_menuitem_callback (GtkWidget *widget, gpointer data)
{
  GtkWidget *assistant;
  gui_t *gui;

  gui = (gui_t *) data;

  assistant = gtk_assistant_new ();
  gtk_window_set_default_size (GTK_WINDOW (assistant), -1, -1);
  gtk_window_set_screen (GTK_WINDOW (assistant), gtk_widget_get_screen (gui->window));
  gtk_window_set_transient_for (GTK_WINDOW (assistant), GTK_WINDOW (gui->window));

  /* Setup Global Widgets */
  gui->generic_ptr = malloc (sizeof (GtkWidget *));

  fillet_create_page1 (gui, assistant);

  g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (fillet_on_assistant_close_cancel), &assistant);
  g_signal_connect (G_OBJECT (assistant), "close", G_CALLBACK (fillet_on_assistant_close_cancel), &assistant);
  g_signal_connect (G_OBJECT (assistant), "prepare", G_CALLBACK (fillet_on_assistant_prepare), NULL);
  g_signal_connect (G_OBJECT (assistant), "apply", G_CALLBACK (fillet_previous_on_assistant_apply), gui);

  gtk_widget_show (assistant);
}


void
gui_menu_edit_fillet_next_menuitem_callback (GtkWidget *widget, gpointer data)
{
  GtkWidget *assistant;
  gui_t *gui;

  gui = (gui_t *) data;

  assistant = gtk_assistant_new ();
  gtk_window_set_default_size (GTK_WINDOW (assistant), -1, -1);
  gtk_window_set_screen (GTK_WINDOW (assistant), gtk_widget_get_screen (gui->window));
  gtk_window_set_transient_for (GTK_WINDOW (assistant), GTK_WINDOW (gui->window));

  /* Setup Global Widgets */
  gui->generic_ptr = malloc (sizeof (GtkWidget *));

  fillet_create_page1 (gui, assistant);

  g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (fillet_on_assistant_close_cancel), &assistant);
  g_signal_connect (G_OBJECT (assistant), "close", G_CALLBACK (fillet_on_assistant_close_cancel), &assistant);
  g_signal_connect (G_OBJECT (assistant), "prepare", G_CALLBACK (fillet_on_assistant_prepare), NULL);
  g_signal_connect (G_OBJECT (assistant), "apply", G_CALLBACK (fillet_next_on_assistant_apply), gui);

  gtk_widget_show (assistant);
}
