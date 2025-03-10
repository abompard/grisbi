/* *******************************************************************************/
/*                                 GRISBI                                        */
/*              Programme de gestion financière personnelle                      */
/*                              license : GPLv2                                  */
/*                                                                               */
/*     Copyright (C)    2000-2008 Cédric Auger (cedric@grisbi.org)               */
/*                      2003-2008 Benjamin Drieu (bdrieu@april.org)              */
/*          2008-2020 Pierre Biava (grisbi@pierre.biava.name)                    */
/*          http://www.grisbi.org                                                */
/*                                                                               */
/*     This program is free software; you can redistribute it and/or modify      */
/*     it under the terms of the GNU General Public License as published by      */
/*     the Free Software Foundation; either version 2 of the License, or         */
/*     (at your option) any later version.                                       */
/*                                                                               */
/*     This program is distributed in the hope that it will be useful,           */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/*     GNU General Public License for more details.                              */
/*                                                                               */
/*     You should have received a copy of the GNU General Public License         */
/*     along with this program; if not, write to the Free Software               */
/*     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*                                                                               */
/* *******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "include.h"
#include <glib/gi18n.h>

/*START_INCLUDE*/
#include "utils_widgets.h"
#include "bet_data.h"
#include "bet_hist.h"
#include "grisbi_win.h"
#include "gsb_account.h"
#include "gsb_combo_box.h"
#include "gsb_data_account.h"
#include "gsb_file.h"
#include "gsb_fyear.h"
#include "import_csv.h"
#include "navigation.h"
#include "prefs_page_bet_account.h"
#include "structures.h"
#include "utils.h"
#include "utils_buttons.h"
#include "utils_dates.h"
#include "utils_str.h"
#include "erreur.h"
/*END_INCLUDE*/

/*START_STATIC*/
static const gchar* bet_duration_array[] = {N_("Month"), N_("Year"), NULL};
static const gchar *import_format_date[] = {
	N_("dd/mm/yyyy"),
	N_("mm/dd/yyyy"),
	N_("yyyy/mm/dd"),
	N_("yyyy/dd/mm"),
	N_("dd/yyyy/mm"),
	N_("mm/yyyy/dd"),
	NULL
};
/*END_STATIC*/

/*START_EXTERN*/
/*END_EXTERN*/

/******************************************************************************/
/* Private functions                                                          */
/******************************************************************************/
/* ORIGIN_DATA WIDGET */
/**
 *
 *
 * \param
 * \param
 * \param
 *
 * \return
 **/
static gboolean utils_widget_origin_data_clicked (GtkWidget *togglebutton,
                                                     GdkEventButton *event,
                                                     gpointer data)
{
    GtkTreeViewColumn *column;
    const gchar *name;
    gchar *title;
    gint account_number;
    gint origin;

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (togglebutton), TRUE);

    if ((origin = GPOINTER_TO_INT (data)) == 0)
        account_number = gsb_account_get_account_from_combo ();
    else
        account_number = gsb_gui_navigation_get_current_account ();

    name = gtk_widget_get_name (GTK_WIDGET (togglebutton));

    if (g_strcmp0 (name, "bet_hist_button_1") == 0)
    {
        gsb_data_account_set_bet_hist_data (account_number, 0);
        bet_data_set_div_ptr (0);
        title = g_strdup (_("Category"));
        if (origin == 0)
        {
            GtkWidget *button;

            button = g_object_get_data (G_OBJECT (grisbi_win_get_account_page ()), "bet_hist_button_1");
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
        }
    }
    else
    {
        gsb_data_account_set_bet_hist_data (account_number, 1);
        bet_data_set_div_ptr (1);
        title = g_strdup (_("Budgetary line"));
        if (origin == 0)
        {
            GtkWidget *button;

            button = g_object_get_data (G_OBJECT (grisbi_win_get_account_page ()), "bet_hist_button_2");
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
        }
    }

    column = g_object_get_data (G_OBJECT (grisbi_win_get_account_page ()),  "hist_column_source");
    gtk_tree_view_column_set_title (GTK_TREE_VIEW_COLUMN (column), title);
    g_free (title);

    gsb_file_set_modified (TRUE);

    gsb_data_account_set_bet_maj (account_number, BET_MAJ_ALL);
    bet_data_update_bet_module (account_number, -1);

	return FALSE;
}

/* IMPORT CSV_OPTIONS WIDGET */
/**
 *
 *
 * \param
 * \param
 *
 * \return
 **/
static void utils_widget_import_csv_options_combo_changed (GtkComboBox *combo,
														   gpointer null)
{
	gint order = 0;
	GrisbiWinRun *w_run;

	w_run = (GrisbiWinRun *) grisbi_win_get_w_run ();
	order = gtk_combo_box_get_active (combo);
	w_run->import_format_order = order;

	g_free (w_run->import_format_date);
	switch (order)
	{
		case 0:
			w_run->import_format_date = g_strdup ("%d/%m/%Y");
			break;
		case 1:
			w_run->import_format_date = g_strdup ("%m/%d/%Y");
			break;
		case 2:
			w_run->import_format_date = g_strdup ("%Y/%m/%d");
			break;
		case 3:
			w_run->import_format_date = g_strdup ("%Y/%d/%m");
			break;
		case 4:
			w_run->import_format_date = g_strdup ("%d/%Y/%m");
			break;
		case 5:
			w_run->import_format_date = g_strdup ("%m/%Y/%d");
			break;
	}

}

/**
 * callback pour forcer le chox du format de la date
 *
 * \param
 * \param
 *
 * \return
 **/
static void utils_widget_import_csv_options_toggled (GtkToggleButton *togglebutton,
													 GtkWidget *assistant)
{
	GrisbiWinRun *w_run;

	w_run = (GrisbiWinRun *) grisbi_win_get_w_run ();
	w_run->import_force_date = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
	if (w_run->import_format_date == NULL)
	{
		GtkWidget *combo;

		combo = g_object_get_data (G_OBJECT (assistant), "combo-format-date");
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
		w_run->import_format_order = 0;
		w_run->import_format_date = g_strdup ("%d/%m/%Y");
	}
}

/******************************************************************************/
/* Public functions                                                           */
/******************************************************************************/
/*COMMON_FUNCTIONS*/
/**
 * fixes error [-Werror=cast-function-type]
 *
 * \param
 * \param
 *
 * \return
 **/
void utils_widget_gtk_callback (GtkWidget *widget,
								gpointer null)
{
	gtk_widget_destroy (widget);
}

/* DURATION WIDGET */
/**
 * create the widget
 *
 * \param origin 		SPP_ORIGIN_CONFIG or SPP_ORIGIN_ARRAY
 *
 * \return				the widget
 **/
GtkWidget *utils_widget_get_duration_widget (gint origin)
{
    GtkWidget *box = NULL;
    GtkWidget *label;
    GtkWidget *spin_button = NULL;
    GtkWidget *widget = NULL;
    GtkWidget *hbox;
    GtkWidget *previous = NULL;
    GtkWidget *account_page;
    gpointer pointeur = NULL;
    gint iduration;

    if (origin == SPP_ORIGIN_CONFIG)
    {
        box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        pointeur = GINT_TO_POINTER (0);
    }
    else if (origin == SPP_ORIGIN_ARRAY)
    {
        box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        pointeur = GINT_TO_POINTER (1);
    }
    account_page = grisbi_win_get_account_page ();

    /* partie mensuelle */
    label = gtk_label_new (_("Duration: "));
    utils_labels_set_alignment (GTK_LABEL (label), GSB_LEFT, GSB_CENTER);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, MARGIN_BOX) ;

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0) ;

    spin_button = gtk_spin_button_new_with_range (1.0, PREV_MONTH_MAX, 1.0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_button), 1.0);
    gtk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);

    for (iduration = 0; bet_duration_array[iduration] != NULL; iduration++)
    {
        if (previous == NULL)
        {
            widget = gtk_radio_button_new_with_label (NULL, _(bet_duration_array[iduration]));
            previous = widget;
        }
        else
        {
            widget = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (previous),
                                                                  _(bet_duration_array[iduration]));
        }
        gtk_widget_set_name (widget, bet_duration_array[iduration]);
        gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
        g_signal_connect (G_OBJECT (widget),
                          "button-release-event",
                          G_CALLBACK (utils_widget_duration_button_released),
                          spin_button);
    }

    if (origin == SPP_ORIGIN_CONFIG)
    {
        g_object_set_data (G_OBJECT (spin_button), "bet_origin_signal", pointeur);
        g_object_set_data (G_OBJECT (account_page), "bet_config_account_previous",  previous);
        g_object_set_data (G_OBJECT (account_page), "bet_config_account_widget", widget);
        g_object_set_data (G_OBJECT (account_page), "bet_config_account_spin_button", spin_button);
    }
    else if (origin == SPP_ORIGIN_ARRAY)
    {
        g_object_set_data (G_OBJECT (spin_button), "bet_origin_signal", pointeur);
        g_object_set_data (G_OBJECT (account_page), "bet_account_previous", previous);
        g_object_set_data (G_OBJECT (account_page), "bet_account_widget", widget);
        g_object_set_data (G_OBJECT (account_page), "bet_account_spin_button", spin_button);
    }

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (previous), TRUE);

    g_signal_connect (G_OBJECT (spin_button),
                      "value-changed",
                      G_CALLBACK (utils_widget_duration_number_changed),
                      pointeur);
    g_object_set_data (G_OBJECT (spin_button), "pointer", pointeur);

    gtk_widget_show_all (box);

    return box;
}

/*
 * This function is called when a radio button is called to change the estimate duration.
 * It copies the new durations from the data parameter (of the radio button) into
 * the bet_months property of the bet container
 *
 * \param
 * \param
 * \param
 *
 * \return
 **/
gboolean utils_widget_duration_button_released (GtkWidget *togglebutton,
                                                GdkEventButton *event,
                                                GtkWidget *spin_button)
{
    const gchar *name;
    gpointer data;
    gint account_number;
    gint months;
    gint origin;

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (togglebutton), TRUE);

    name = gtk_widget_get_name (GTK_WIDGET (togglebutton));
    data = g_object_get_data (G_OBJECT (spin_button), "bet_origin_signal");

    if ((origin = GPOINTER_TO_INT (data)) == 0)
        account_number = gsb_account_get_account_from_combo ();
    else
        account_number = gsb_gui_navigation_get_current_account ();

    g_signal_handlers_block_by_func (G_OBJECT (spin_button),
                                     G_CALLBACK (utils_widget_duration_number_changed),
                                     data);

    if (g_strcmp0 (name, "Year") == 0)
    {
        gsb_data_account_set_bet_spin_range (account_number, 1);
        gtk_spin_button_set_range (GTK_SPIN_BUTTON (spin_button), 1.0, PREV_MONTH_MAX / 12.0);
        months = gsb_data_account_get_bet_months (account_number);
        if (months > (PREV_MONTH_MAX / 12.0))
        {
            gint number;

            number = months / 12;
            if (number < 1)
                number = 1;
            else if (months % 12)
                number ++;
            gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_button), number);
            gsb_data_account_set_bet_months (account_number, number * 12);
        }
        else
            gsb_data_account_set_bet_months (account_number, months * 12);
    }
    else
    {
        gsb_data_account_set_bet_spin_range (account_number, 0);
        months = gsb_data_account_get_bet_months (account_number);
        gsb_data_account_set_bet_months (account_number, months);
        gtk_spin_button_set_range (GTK_SPIN_BUTTON (spin_button), 1.0, PREV_MONTH_MAX);
        if (origin == 0)
            gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_button), months);
    }

    g_signal_handlers_unblock_by_func (G_OBJECT (spin_button),
                                       G_CALLBACK (utils_widget_duration_button_released),
                                       data);

    gsb_file_set_modified (TRUE);

    gsb_data_account_set_bet_maj (account_number, BET_MAJ_ESTIMATE);
    bet_data_update_bet_module (account_number, -1);

	return FALSE;
}

/*
 * bet_config_duration_button changed
 * This function is called when a spin button is changed.
 * It copies the new duration from the spin_button into the bet_months property of
 * the bet container
 *
 * \param
 * \param
 *
 * \return
 **/
void utils_widget_duration_number_changed (GtkWidget *spin_button,
										   gpointer data)
{
    gint account_number;
    gint months;

    if (GPOINTER_TO_INT (data) == 0)
        account_number = gsb_account_get_account_from_combo ();
    else
        account_number = gsb_gui_navigation_get_current_account ();

    months = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin_button));
    if (gsb_data_account_get_bet_spin_range (account_number) == 1)
        months *= 12;

    gsb_data_account_set_bet_months (account_number, months);

    gsb_file_set_modified (TRUE);

    gsb_data_account_set_bet_maj (account_number, BET_MAJ_ESTIMATE);
    bet_data_update_bet_module (account_number, -1);
}

/* ORIGIN_DATA WIDGET */
/**
 * creation du widget origine data
 *
 * \param	notebook 	account page
 * \param	type		SPP_ORIGIN_HISTORICAL ou SPP_ORIGIN_CONFIG
 *
 * \return
 **/
GtkWidget *utils_widget_origin_data_new (GtkWidget *notebook,
                                         gint type)
{
    GtkWidget *button_1;
    GtkWidget *button_2;
    GtkWidget *hbox;
    GtkWidget *widget;
    gpointer pointeur;

    /* Choix des données sources */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, MARGIN_BOX);

    button_1 = gtk_radio_button_new_with_label (NULL, _("Categories"));
    gtk_widget_set_name (button_1, "bet_hist_button_1");
    button_2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (button_1),
                        									_("Budgetary lines"));
	if (type  == SPP_ORIGIN_CONFIG)
	{
		g_object_set_data (G_OBJECT (notebook), "bet_config_hist_button_1", button_1);
		g_object_set_data (G_OBJECT (notebook), "bet_config_hist_button_2", button_2);
		pointeur = GINT_TO_POINTER (0);
	}
	else		/* SPP_ORIGIN_HISTORICAL */
	{
		g_object_set_data (G_OBJECT (notebook), "bet_hist_button_1", button_1);
		g_object_set_data (G_OBJECT (notebook), "bet_hist_button_2", button_2);
		pointeur = GINT_TO_POINTER (1);
	}

	gtk_box_pack_start (GTK_BOX (hbox), button_1, FALSE, FALSE, MARGIN_BOX) ;
	gtk_box_pack_start (GTK_BOX (hbox), button_2, FALSE, FALSE, MARGIN_BOX) ;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button_1), TRUE);

	g_signal_connect (G_OBJECT (button_1),
					  "button-release-event",
					  G_CALLBACK (utils_widget_origin_data_clicked),
					  pointeur);
	g_signal_connect (G_OBJECT (button_2),
					  "button-release-event",
					  G_CALLBACK (utils_widget_origin_data_clicked),
					  pointeur);


    /* création du sélecteur de périod */
	if ((type == SPP_ORIGIN_HISTORICAL && bet_hist_fyear_create_combobox_store ())
	    || type == SPP_ORIGIN_CONFIG)
	{
		GtkTreeModel *bet_fyear_model_filter;

		bet_fyear_model_filter = bet_hist_get_bet_fyear_model_filter ();
		widget = gsb_fyear_make_combobox_new (bet_fyear_model_filter, TRUE);
		gtk_widget_set_name (GTK_WIDGET (widget), "bet_hist_fyear_combo");
		gtk_widget_set_tooltip_text (GTK_WIDGET (widget),
									 _("Choose the financial year or 12 months rolling"));
		gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, MARGIN_BOX);

		if (type == SPP_ORIGIN_CONFIG)
		{
			g_object_set_data (G_OBJECT (notebook), "bet_config_hist_fyear_combo", widget);
			pointeur = GINT_TO_POINTER (0);
		}
		else
		{
			g_object_set_data (G_OBJECT (notebook), "bet_hist_fyear_combo", widget);
			pointeur = GINT_TO_POINTER (1);
		}
		g_object_set_data (G_OBJECT (widget), "pointer", pointeur);

        /* hide the present and futures financial years */
        bet_hist_fyear_hide_present_futures_fyears ();

		/* set the signal */
		g_signal_connect (G_OBJECT (widget),
						  "changed",
						  G_CALLBACK (utils_widget_origin_fyear_clicked),
						  pointeur);
	}
	gtk_widget_show_all (hbox);

    return hbox;
}

/**
 *
 *
 * \param
 * \param
 *
 * \return
 **/
void utils_widget_origin_fyear_clicked (GtkWidget *combo,
                                 gpointer data)
{
    gint account_number;

	if (GPOINTER_TO_INT (data) == 0)
		account_number = gsb_account_get_account_from_combo ();
	else
		account_number = gsb_gui_navigation_get_current_account ();

    gsb_data_account_set_bet_hist_fyear (account_number,
                                         bet_hist_get_fyear_from_combobox (combo));

    gsb_file_set_modified (TRUE);

    gsb_data_account_set_bet_maj (account_number, BET_MAJ_ALL);
    bet_data_update_bet_module (account_number, -1);
}


/* IMPORT CSV_OPTIONS WIDGET */
/**
 *
 *
 * \param
 * \param
 * \param
 *
 * \return
 **/
void utils_widget_import_csv_options_widget_new (GtkWidget *parent,
												 gboolean fill,
												 GtkWidget *assistant)
{
	GtkWidget *grid;
	GtkWidget *label;
	GtkWidget *hbox_sep;
	GtkWidget *check_button;
	GtkWidget *check_button_label;
	GtkWidget *combo_format_date;
	GtkWidget *hbox_format_date;
	GtkSizeGroup *size_group;
	gchar *title;
	gchar* tmp_str;

	/* set hbox options for csv separator and force dates data */
	grid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (grid), MARGIN_BOX);
	gtk_grid_set_row_spacing (GTK_GRID (grid), MARGIN_BOX);
	gtk_box_pack_start (GTK_BOX (parent), grid, FALSE, FALSE, 0);

    /* on crée le size_group pour l'alignement des hbox */
    size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/* set choose csv separators */
    label = gtk_label_new (NULL);
    utils_labels_set_alignment (GTK_LABEL (label), 0, 1);
	gtk_widget_set_margin_start (label, MARGIN_BOX);
	title = g_strdup (_("Choose CSV separator"));
    tmp_str = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>", title);
    gtk_label_set_markup (GTK_LABEL (label), tmp_str);
	g_free (title);
    g_free(tmp_str);
	gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 2 , 1);

	label = gtk_label_new ("    ");
    gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1 , 1);

	hbox_sep = utils_widget_csv_separators_new (NULL,
												G_CALLBACK (csv_import_change_separator),
												assistant);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (size_group), hbox_sep);
    gtk_grid_attach (GTK_GRID (grid), hbox_sep, 1, 1, 1 , 1);

	/* set force date option */
	/* set check button */
	title = g_strdup (_("Force date format"));
	tmp_str = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>", title);
	check_button = gtk_check_button_new_with_label (tmp_str);
	check_button_label = gtk_bin_get_child (GTK_BIN (check_button));
	gtk_label_set_markup (GTK_LABEL (check_button_label), tmp_str);
	g_free (title);
    g_free(tmp_str);
	gtk_grid_attach (GTK_GRID (grid), check_button, 2, 0, 2 , 1);

	label = gtk_label_new ("    ");
    gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1 , 1);

	/* set hbox date format */
	hbox_format_date = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, MARGIN_BOX);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (size_group), hbox_format_date);
	gtk_widget_set_sensitive (hbox_format_date, FALSE);
	gtk_grid_attach (GTK_GRID (grid), hbox_format_date, 3, 1, 1 , 1);

	/* set label */
	label = gtk_label_new ("Select new date format : ");
	gtk_box_pack_start (GTK_BOX (hbox_format_date), label, TRUE, TRUE, 0);

	/* set combo date format */
	combo_format_date = gsb_combo_box_new_with_index (import_format_date, NULL, NULL);
	g_object_set_data (G_OBJECT (assistant), "combo-format-date", combo_format_date);
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo_format_date), 0);
    gtk_box_pack_start (GTK_BOX (hbox_format_date), combo_format_date, TRUE, TRUE, 0);

	/* set signals */
	g_signal_connect (G_OBJECT (check_button),
                      "toggled",
                      G_CALLBACK (utils_buttons_sensitive_by_checkbutton),
                      hbox_format_date);

	g_signal_connect_after (G_OBJECT (check_button),
						    "toggled",
						    G_CALLBACK (utils_widget_import_csv_options_toggled),
						    assistant);
	g_signal_connect (G_OBJECT (combo_format_date),
                      "changed",
                      G_CALLBACK (utils_widget_import_csv_options_combo_changed),
                      NULL);

	gtk_widget_show_all (grid);
}

/**
 *
 *
 * \param
 * \param
 *
 * \return
 **/
void utils_widget_import_csv_options_set_combo_order (GtkWidget *assistant,
													  gint import_format_order)
{
	GtkWidget *combo;

	combo = g_object_get_data (G_OBJECT (assistant), "combo-format-date");
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), import_format_order);
}

/**
 *
 *
 * \param
 *
 * \return
 **/
/* Local Variables: */
/* c-basic-offset: 4 */
/* End: */
