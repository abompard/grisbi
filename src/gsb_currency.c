/* ************************************************************************** */
/*                                                                            */
/*     Copyright (C)    2000-2007 Cédric Auger (cedric@grisbi.org)            */
/*          2003-2009 Benjamin Drieu (bdrieu@april.org)                       */
/*                      2009 Pierre Biava (grisbi@pierre.biava.name)          */
/*          https://www.grisbi.org/                                           */
/*                                                                            */
/*  This program is free software; you can redistribute it and/or modify      */
/*  it under the terms of the GNU General Public License as published by      */
/*  the Free Software Foundation; either version 2 of the License, or         */
/*  (at your option) any later version.                                       */
/*                                                                            */
/*  This program is distributed in the hope that it will be useful,           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/*  GNU General Public License for more details.                              */
/*                                                                            */
/*  You should have received a copy of the GNU General Public License         */
/*  along with this program; if not, write to the Free Software               */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*                                                                            */
/* ************************************************************************** */

/**
 * \file gsb_currency.c
 * contains tools to work with the currencies
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "include.h"
#include <glib/gi18n.h>

/*START_INCLUDE*/
#include "gsb_currency.h"
#include "dialog.h"
#include "grisbi_app.h"
#include "gsb_autofunc.h"
#include "gsb_currency_popup.h"
#include "gsb_data_account.h"
#include "gsb_data_currency.h"
#include "gsb_data_currency_link.h"
#include "gsb_data_form.h"
#include "gsb_data_transaction.h"
#include "gsb_dirs.h"
#include "gsb_form_widget.h"
#include "gsb_real.h"
#include "prefs_page_metatree.h"
#include "structures.h"
#include "utils.h"
#include "utils_files.h"
#include "utils_prefs.h"
#include "utils_real.h"
#include "erreur.h"
#include "widget_account_property.h"
#include "widget_currency_details.h"
/*END_INCLUDE*/

 /** This structure holds information needed for exchange rates cache. */
typedef struct	_CachedExchangeRate		CachedExchangeRate;
struct _CachedExchangeRate {
    gint 		currency1_number;	/** First currency */
    gint 		currency2_number;	/** Second currency */
    GsbReal		rate;				/** Exchange rate between currency1 and currency 2 */
    GsbReal		fees;				/** Fees associated with exchange rate */
};

/*START_STATIC*/
static GtkWidget *gsb_currency_make_combobox_exchange_dialog (gint transaction_currency_number,
                        gint account_currency_number,
                        gint set_index);
/*END_STATIC*/

/**
 * the currency list store, contains 3 columns :
 * 1 : the code of the currency
 * 2 : the name(code) of the currency
 * 3 : the number of the currency
 * used to be set in the combobox */
static GtkListStore *combobox_currency_store;

enum CurrencyComboColumns
{
    CURRENCY_COL_CODE = 0,
    CURRENCY_COL_NAME,
    CURRENCY_COL_NUMBER,
    CURRENCY_COL_FLAG,
    CURRENCY_NUM_COL		 /** Number of columns */
};

/** Exchange rates cache, used by
 * gsb_currency_config_set_cached_exchange
 * and
 * gsb_currency_get_cached_exchange */
static GSList *cached_exchange_rates = NULL;

/**
 * the 2 next variables are filled by gsb_currency_exchange_dialog
 * and permit to the form to get the result by the functions
 * gsb_currency_get_current_exchange
 * gsb_currency_get_current_exchange_fees
 * */
static GsbReal current_exchange;
static GsbReal current_exchange_fees;

/*START_EXTERN*/
/*END_EXTERN*/

/******************************************************************************/
/* Private functions                                                          */
/******************************************************************************/
/**
 * met à jour value à chaque changement du check_button
 *
 * \param
 * \param
 *
 * \return
 **/
static void gsb_currency_checkbutton_link_changed (GtkWidget *checkbutton,
												   gboolean *value)
{
    if (value)
        *value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton));
}

/**
 * Find whether echange rate between two currencies is known.  If so,
 * returns a cached_exchange_rate structure with exchange rate
 * information.
 *
 * \param currency1 		First currency
 * \param currency2 		Second currency
 *
 * \return NULL on failure	a pointer to a cached_exchange_rate structure on success.
 **/
static CachedExchangeRate *gsb_currency_get_cached_exchange (gint currency1_number,
															 gint currency2_number)
{
    GSList *tmp_list;

    tmp_list = cached_exchange_rates;
    while (tmp_list)
    {
		CachedExchangeRate *tmp;

		tmp = tmp_list->data;
		if (currency1_number == tmp->currency1_number && currency2_number == tmp->currency2_number)
			return tmp;

		tmp_list = tmp_list->next;
    }

	return NULL;
}

/**
 * Update exchange rate cache according to arguments.
 *
 * \param currency1 	First currency.
 * \param currency2 	Second currency.
 * \param change    	Exchange rate between two currencies.
 * \param fees      	Fees of transaction.
 *
 * \return
 **/
static void gsb_currency_config_set_cached_exchange (gint currency1_number,
													 gint currency2_number,
													 GsbReal change,
													 GsbReal fees)
{
    CachedExchangeRate *tmp;

    tmp = (CachedExchangeRate *) g_malloc0 (sizeof(CachedExchangeRate));

    tmp->currency1_number = currency1_number;
    tmp->currency2_number = currency2_number;
    tmp->rate = change;
    tmp->fees = fees;

    cached_exchange_rates = g_slist_append (cached_exchange_rates, tmp);
}

/**
 * create and fill the list store of the currency
 * come here mean that combobox_currency_store is NULL
 *
 * \param
 *
 * \return TRUE ok, FALSE problem
 **/
static gboolean gsb_currency_create_combobox_store (void)
{
    combobox_currency_store = gtk_list_store_new (CURRENCY_NUM_COL,
											      G_TYPE_STRING,		/* CURRENCY_COL_CODE */
											      G_TYPE_STRING,		/* CURRENCY_COL_NAME */
											      G_TYPE_INT,			/* CURRENCY_COL_NUMBER */
											      GDK_TYPE_PIXBUF);		/* CURRENCY_COL_FLAG */
    gsb_currency_update_combobox_currency_list ();

	return TRUE;
}

/**
 * Handler that change the entries and calculate the exchange_rate
 *
 *
 * \param entry_1 	the entry which receive the signal
 * \param entry_2 	the other entry
 *
 * \return FALSE
 **/
static void gsb_currency_select_double_amount_changed (GtkWidget *entry_1,
													   GtkWidget *entry_2)
{
    GtkWidget *entry;
    GtkWidget *entry_3;
    GtkWidget *entry_4;
    GsbReal amount_1;
    GsbReal amount_2;
	GsbReal taux;
    gboolean link_currency;
    gboolean valide;

    entry = g_object_get_data (G_OBJECT (entry_1), "exchange_rate");
    link_currency = GPOINTER_TO_INT (g_object_get_data ( G_OBJECT (entry_1), "link_currency"));

    if (link_currency)
    {
        entry_3 = entry_2;
        entry_4 = entry_1;
    }
    else
    {
        entry_3 = entry_1;
        entry_4 = entry_2;
    }

    valide = gsb_form_widget_get_valide_amout_entry (
                gtk_entry_get_text (GTK_ENTRY (entry_1)));
    if (valide)
    {
        /* the entry is valid, make it normal */
        gtk_widget_set_name (entry_1, "form_entry");
    }
    else
    {
        /* the entry is not valid, make it red */
        gtk_widget_set_name (entry_1, "form_entry_error");
        return;
    }

    valide = gsb_form_widget_get_valide_amout_entry (
                gtk_entry_get_text (GTK_ENTRY (entry_2)));
    if (valide)
    {
        /* the entry is valid, make it normal */
        gtk_widget_set_name (entry_2, "form_entry");
    }
    else
    {
        /* the entry is not valid, make it red */
        gtk_widget_set_name (entry_2, "form_entry_error");
        return;
    }

    if (strlen (gtk_entry_get_text (GTK_ENTRY (entry_3))) > 0)
    {
        if (!strlen (gtk_entry_get_text (GTK_ENTRY (entry_4))))
		{
            gtk_entry_set_text (GTK_ENTRY (entry), "");
			gtk_widget_set_sensitive (GTK_WIDGET (entry), TRUE);
		}
		else
        {
            gtk_widget_set_sensitive (GTK_WIDGET (entry), FALSE);
            amount_1 = utils_real_get_from_string (gtk_entry_get_text (GTK_ENTRY (entry_1)));
            amount_2 = utils_real_get_from_string (gtk_entry_get_text (GTK_ENTRY (entry_2)));
            taux = gsb_real_div (amount_2, amount_1);
            gtk_entry_set_text (GTK_ENTRY (entry), utils_real_get_string (taux));
        }
    }
}

/**
 * Handler that change the second combobox of a window that ask for
 * change.
 *
 * \param combobox_1 	the combobox which receive the signal
 * \param combobox_2 	the combobox we want to change
 *
 * \return
 **/
static void gsb_currency_select_change_currency (GtkWidget *combobox_1,
												 GtkWidget *combobox_2)
{
    GtkWidget *entry_1;
    GtkWidget *entry_2;
    gchar *string = NULL;

    /* we just need to set the same active menu on the second combobox */
    g_signal_handlers_block_by_func (G_OBJECT (combobox_1),
									 G_CALLBACK (gsb_currency_select_change_currency),
									 combobox_2);
    g_signal_handlers_block_by_func (G_OBJECT (combobox_2),
									 G_CALLBACK (gsb_currency_select_change_currency),
									 combobox_1);

    gtk_combo_box_set_active (GTK_COMBO_BOX (combobox_2),
							  !gtk_combo_box_get_active (GTK_COMBO_BOX (combobox_1)));

    entry_1 = g_object_get_data (G_OBJECT (combobox_1), "amount_1_entry");
    entry_2 = g_object_get_data (G_OBJECT (combobox_1), "amount_2_entry");

    g_signal_handlers_block_by_func (G_OBJECT (entry_1),
									 G_CALLBACK (gsb_currency_select_double_amount_changed),
									 entry_2);
    g_signal_handlers_block_by_func (G_OBJECT (entry_2),
									 G_CALLBACK (gsb_currency_select_double_amount_changed),
									 entry_1);

    string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry_1)));
    if (string && strlen (string) > 0)
    {
        gtk_entry_set_text (GTK_ENTRY (entry_1), "");
        gtk_entry_set_text (GTK_ENTRY (entry_2), string);
        g_free (string);
    }
    else
    {
        string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry_2)));
        if (string && strlen (string) > 0)
        {
            gtk_entry_set_text (GTK_ENTRY (entry_2), "");
            gtk_entry_set_text (GTK_ENTRY (entry_1), string);
            g_free (string);
        }
    }

    g_signal_handlers_unblock_by_func (G_OBJECT (entry_1),
									   G_CALLBACK (gsb_currency_select_double_amount_changed),
                    				   entry_2);
    g_signal_handlers_unblock_by_func (G_OBJECT (entry_2),
									   G_CALLBACK (gsb_currency_select_double_amount_changed),
                    				   entry_1);

    g_signal_handlers_unblock_by_func (G_OBJECT (combobox_1),
									   G_CALLBACK (gsb_currency_select_change_currency),
                        			   combobox_2);
    g_signal_handlers_unblock_by_func (G_OBJECT (combobox_2),
									   G_CALLBACK (gsb_currency_select_change_currency),
                        			   combobox_1);
}

/*
 * create the exchange rate dialog
 *
 * \param
 * \param
 *
 * \return
 **/
static GtkWidget *gsb_currency_make_combobox_exchange_dialog (gint transaction_currency_number,
															  gint account_currency_number,
															  gint set_index)
{
    GtkWidget *combo_box = NULL;
    GtkListStore *combobox_store;
    GtkCellRenderer *text_renderer, *flag_renderer;
    GtkTreeIter iter;
    GdkPixbuf *pixbuf;
    gchar *string;
    gchar *tmp_dir;
    gint xpad;
    gint ypad;

    combobox_store = gtk_list_store_new (3, G_TYPE_INT, GDK_TYPE_PIXBUF,
						G_TYPE_STRING);

    string = g_strconcat (gsb_data_currency_get_code_iso4217 (transaction_currency_number), ".png", NULL);

    tmp_dir = g_build_filename (gsb_dirs_get_pixmaps_dir (), "flags", string, NULL);
    pixbuf = gdk_pixbuf_new_from_file (tmp_dir, NULL);
    g_free (string);
    g_free (tmp_dir);

    gtk_list_store_append (GTK_LIST_STORE (combobox_store), &iter);
    gtk_list_store_set (combobox_store, &iter,
                    0, transaction_currency_number,
                    1, pixbuf,
                    2, gsb_data_currency_get_name (transaction_currency_number),
                    -1);

    string = g_strconcat(gsb_data_currency_get_code_iso4217 (account_currency_number), ".png", NULL);

    tmp_dir = g_build_filename (gsb_dirs_get_pixmaps_dir (), "flags", string, NULL);
    pixbuf = gdk_pixbuf_new_from_file (tmp_dir, NULL);
    g_free (string);
    g_free (tmp_dir);

    gtk_list_store_append (GTK_LIST_STORE (combobox_store), &iter);
    gtk_list_store_set (combobox_store, &iter,
                    0, account_currency_number,
                    1, pixbuf,
                    2, gsb_data_currency_get_name (account_currency_number),
                    -1);

    combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL
                        (combobox_store));

    /* Flag renderer */
    flag_renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), flag_renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), flag_renderer,
				    "pixbuf", 1, NULL);

    gtk_cell_renderer_get_padding (GTK_CELL_RENDERER (flag_renderer), &xpad, &ypad);
    gtk_cell_renderer_set_padding (GTK_CELL_RENDERER (flag_renderer), 3, ypad);

    text_renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), text_renderer, FALSE);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), text_renderer,
					"text", 2, NULL);

    gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), set_index);

    return (combo_box);
}

/**
 * Set an integer to the value of a menu.  Normally called via a GTK
 * "changed" signal handler.
 *
 * \param menu 		a pointer to a menu widget.
 * \param dummy 	unused
 *
 * \return
 **/
static gboolean gsb_currency_combobox_value_changed (GtkWidget *combobox,
													 gint *dummy)
{
    gint *data;

    data = g_object_get_data (G_OBJECT(combobox), "pointer");

    if (data)
    {
		*data = gsb_currency_get_currency_from_combobox (combobox);
    }

    /* Mark file as modified */
    utils_prefs_gsb_file_set_modified ();

    return (FALSE);
}

/******************************************************************************/
/* Public functions                                                           */
/******************************************************************************/
/**
 * set to NULL the static variables
 *
 * \param
 *
 * \return
 * */
void gsb_currency_init_variables (void)
{
    if (combobox_currency_store
    &&
    GTK_IS_LIST_STORE (combobox_currency_store))
    gtk_list_store_clear (combobox_currency_store);

    combobox_currency_store = NULL;
    current_exchange = null_real;
    current_exchange_fees = null_real;
}

/**
 * create and return a combobox with the currencies
 * for automatic value changed in memory, see gsb_autofunc_currency_new
 *
 * \param set_name if TRUE, the currencies in the combobox will be name(code), else it will be only the code
 *
 * \return a widget combobox or NULL
 * */
GtkWidget *gsb_currency_make_combobox (gboolean set_name)
{
    GtkCellRenderer *text_renderer, *flag_renderer;
    GtkWidget *combo_box;
    gint xpad;
    gint ypad;

    if (!combobox_currency_store)
        gsb_currency_create_combobox_store ();

    combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL
                        (combobox_currency_store));

    /* Flag renderer */
    flag_renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), flag_renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), flag_renderer,
				    "pixbuf", CURRENCY_COL_FLAG, NULL);

    gtk_cell_renderer_get_padding (GTK_CELL_RENDERER (flag_renderer), &xpad, &ypad);
    gtk_cell_renderer_set_padding (GTK_CELL_RENDERER (flag_renderer), 3, ypad);

    text_renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), text_renderer, FALSE);

    if (set_name)
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), text_renderer,
					"text", CURRENCY_COL_NAME,
					NULL);
    else
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), text_renderer,
					"text", CURRENCY_COL_CODE,
					NULL);

    gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box),
			       0);

    return (combo_box);
}

/**
 * create and return a combobox with the currencies
 * for automatic value changed in memory, see gsb_autofunc_currency_new
 *
 * \param combo		a combobox
 * \param set_name if TRUE, the currencies in the combobox will be name(code), else it will be only the code
 *
 * \return a widget combobox or NULL
 **/
void gsb_currency_make_combobox_from_ui (GtkWidget *combo_box,
										 gboolean set_name)
{
    GtkCellRenderer *text_renderer, *flag_renderer;
    gint xpad;
    gint ypad;

    if (!combobox_currency_store)
        gsb_currency_create_combobox_store ();

     gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (combobox_currency_store));

    /* Flag renderer */
    flag_renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), flag_renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box),
									flag_renderer,
									"pixbuf", CURRENCY_COL_FLAG,
									NULL);

    gtk_cell_renderer_get_padding (GTK_CELL_RENDERER (flag_renderer), &xpad, &ypad);
    gtk_cell_renderer_set_padding (GTK_CELL_RENDERER (flag_renderer), 3, ypad);

    text_renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), text_renderer, FALSE);

    if (set_name)
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box),
										text_renderer,
										"text", CURRENCY_COL_NAME,
										NULL);
    else
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box),
										text_renderer,
										"text", CURRENCY_COL_CODE,
										NULL);

    gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);
}

/**
 * set the combobox on the currency given in param
 *
 * \combo_box the combo-box to set
 * \currency_number the currency we want to set on the combo-box
 *
 * \return TRUE currency found, FALSE currency not found, nothing change
 * */
gboolean gsb_currency_set_combobox_history (GtkWidget *combo_box,
                        gint currency_number)
{
    GtkTreeIter iter;
    gint result;

    if (!combobox_currency_store)
	gsb_currency_create_combobox_store ();

    result = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (combobox_currency_store),
					     &iter);
    while (result)
    {
	gint value;

	gtk_tree_model_get (GTK_TREE_MODEL (combobox_currency_store),
			     &iter,
			     CURRENCY_COL_NUMBER, &value,
			     -1);

	if (value == currency_number)
	{
	    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box),
					    &iter);
	    return TRUE;
	}
	result = gtk_tree_model_iter_next (GTK_TREE_MODEL (combobox_currency_store),
					    &iter);
    }
    return FALSE;
}


/**
 * Get and return the number of the currency in the option_menu given
 * in param
 *
 * \param currency_option_menu an option menu with the currencies
 *
 * \return the number of currency
 * */
gint gsb_currency_get_currency_from_combobox (GtkWidget *combo_box)
{
    gint currency_number = 0;
    GtkTreeIter iter;

    if (!combobox_currency_store)
	gsb_currency_create_combobox_store ();

    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box),
					&iter))
	gtk_tree_model_get (GTK_TREE_MODEL (combobox_currency_store),
			     &iter,
			     CURRENCY_COL_NUMBER, &currency_number,
			     -1);
    return currency_number;
}



/**
 * update the list of the currencies for combobox, which change all
 * the current combobox content
 *
 * \param
 *
 * \return FALSE
 */
gboolean gsb_currency_update_combobox_currency_list (void)
{
	GtkWidget *detail_devise_compte;
	GtkWidget *combo_devise_totaux_categ;
	GtkWidget *combo_devise_totaux_ib;
	GtkWidget *combo_devise_totaux_tiers;
    GSList *list_tmp;
    gulong handler_id;
    gint old_currency_number = -1;
	gchar* tmpstr;
	GrisbiWinRun *w_run;

	if (!combobox_currency_store || !gsb_data_currency_get_currency_list ())
		return FALSE;

	combo_devise_totaux_categ = prefs_page_metatree_get_currency_combobox ("combo_totaux_categ");
	combo_devise_totaux_ib = prefs_page_metatree_get_currency_combobox ("combo_totaux_ib");
	combo_devise_totaux_tiers = prefs_page_metatree_get_currency_combobox ("combo_totaux_tiers");

	/* XXX still buggy, very slow on the gtk_list_store_clear() call, try to find why. */
	detail_devise_compte = widget_account_property_get_combo_account_currency ();
    if (detail_devise_compte && G_IS_OBJECT (detail_devise_compte))
    {
        handler_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (detail_devise_compte), "changed-hook"));
        if (handler_id > 0)
        {
            g_signal_handler_block ((gpointer *) detail_devise_compte, handler_id);
            old_currency_number = gtk_combo_box_get_active (GTK_COMBO_BOX (detail_devise_compte));
        }
    }
    if (combo_devise_totaux_tiers && G_IS_OBJECT (combo_devise_totaux_tiers))
    {
        handler_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_tiers), "changed-hook"));
        if (handler_id > 0)
            g_signal_handler_block ((gpointer *) combo_devise_totaux_tiers,
									GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_tiers),
																		 "changed-hook")));
    }
    if (combo_devise_totaux_categ  && G_IS_OBJECT (combo_devise_totaux_categ))
    {
        handler_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_categ), "changed-hook"));
        if (handler_id > 0)
            g_signal_handler_block ((gpointer *) combo_devise_totaux_categ,
									GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_categ),
																		 "changed-hook")));
    }
    if (combo_devise_totaux_ib  && G_IS_OBJECT (combo_devise_totaux_ib))
    {
        handler_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_ib), "changed-hook"));
        if (handler_id > 0)
            g_signal_handler_block ((gpointer *) combo_devise_totaux_ib,
									GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_ib),
																		 "changed-hook")));
    }

    gtk_list_store_clear (GTK_LIST_STORE (combobox_currency_store));
    list_tmp = gsb_data_currency_get_currency_list ();

    while (list_tmp)
    {
        GtkTreeIter iter;
        GdkPixbuf * pixbuf;
        gchar * string;
        gint currency_number;

        currency_number = gsb_data_currency_get_no_currency (list_tmp->data);
        string = g_strconcat (gsb_dirs_get_pixmaps_dir (),
							  G_DIR_SEPARATOR_S,
							  "flags",
							  G_DIR_SEPARATOR_S,
							  gsb_data_currency_get_code_iso4217 (currency_number),
							  ".png",
							  NULL);
        pixbuf = gdk_pixbuf_new_from_file (string, NULL);
        g_free (string);


        gtk_list_store_append (GTK_LIST_STORE (combobox_currency_store), &iter);
        tmpstr = g_strconcat (gsb_data_currency_get_name (currency_number),
							  " (",
							  gsb_data_currency_get_nickname_or_code_iso (currency_number),
							  ")",
							  NULL);
        gtk_list_store_set (combobox_currency_store,
							&iter,
							CURRENCY_COL_FLAG, pixbuf,
							CURRENCY_COL_CODE, gsb_data_currency_get_nickname_or_code_iso (currency_number),
							CURRENCY_COL_NAME, tmpstr,
							CURRENCY_COL_NUMBER, currency_number,
							-1);
        g_free (tmpstr);
        list_tmp = list_tmp->next;
    }

	w_run = (GrisbiWinRun *) grisbi_win_get_w_run ();
    w_run->mise_a_jour_liste_comptes_accueil = TRUE;
    w_run->mise_a_jour_liste_echeances_manuelles_accueil = TRUE;
    w_run->mise_a_jour_liste_echeances_auto_accueil = TRUE;

    if (detail_devise_compte && G_IS_OBJECT (detail_devise_compte))
    {
        handler_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (detail_devise_compte), "changed-hook"));
        if (handler_id > 0)
        {
            gtk_combo_box_set_active (GTK_COMBO_BOX (detail_devise_compte), old_currency_number);
            g_signal_handler_unblock (detail_devise_compte,
									  GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (detail_devise_compte),
																		   "changed-hook")));
        }
    }
    if (combo_devise_totaux_tiers && G_IS_OBJECT (combo_devise_totaux_tiers))
    {
        handler_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_tiers), "changed-hook"));
        if (handler_id > 0)
        {
            gtk_combo_box_set_active (GTK_COMBO_BOX (combo_devise_totaux_tiers), old_currency_number);
            g_signal_handler_unblock ((gpointer *) combo_devise_totaux_tiers,
									  GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_tiers),
																		   "changed-hook")));
        }
    }
    if (combo_devise_totaux_categ  && G_IS_OBJECT (combo_devise_totaux_categ))
    {
        handler_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_categ), "changed-hook"));
        if (handler_id > 0)
        {
            gtk_combo_box_set_active (GTK_COMBO_BOX (combo_devise_totaux_categ), old_currency_number);
            g_signal_handler_unblock ((gpointer *) combo_devise_totaux_categ,
									  GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_categ),
																		   "changed-hook")));
        }
    }
    if (combo_devise_totaux_ib  && G_IS_OBJECT (combo_devise_totaux_ib))
    {
        handler_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_ib), "changed-hook"));
        if (handler_id > 0)
        {
            gtk_combo_box_set_active (GTK_COMBO_BOX (combo_devise_totaux_ib), old_currency_number);
            g_signal_handler_unblock ((gpointer *) combo_devise_totaux_ib,
									  GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (combo_devise_totaux_ib),
																		   "changed-hook")));
        }
    }

    return FALSE;
}


/**
 * Check if a transaction need an exchange rate and fees with its
 * account
 * if yes, ask for that and set the in the transaction.
 *
 * \param transaction_number
 */
void gsb_currency_check_for_change (gint transaction_number)
{
    gint transaction_currency_number;
    gint account_currency_number;
    gint link_number;

    account_currency_number = gsb_data_account_get_currency (
                        gsb_data_transaction_get_account_number (transaction_number));
    transaction_currency_number = gsb_data_transaction_get_currency_number (
                        transaction_number);

	if (transaction_currency_number == account_currency_number)
	{
		gsb_data_transaction_set_exchange_rate (transaction_number, null_real);
		gsb_data_transaction_set_change_between (transaction_number, 0);
		gsb_data_transaction_set_exchange_fees (transaction_number, null_real);
		return;
	}

    link_number = gsb_data_currency_link_search (account_currency_number,
                        transaction_currency_number);

    if (link_number)
    {
        if (current_exchange_fees.mantissa)
            gsb_data_transaction_set_exchange_fees (transaction_number,
                        current_exchange_fees);
        else
            gsb_data_transaction_set_exchange_fees (transaction_number,
                        null_real);

        if (current_exchange.mantissa == 0)
            gsb_data_transaction_set_exchange_rate (transaction_number,
                    gsb_real_abs (
                    gsb_data_currency_link_get_change_rate (
                    link_number)));
        else
            gsb_data_transaction_set_exchange_rate (transaction_number,
                    current_exchange);

        if (gsb_data_currency_link_get_first_currency (
         link_number) == account_currency_number)
            gsb_data_transaction_set_change_between (transaction_number, 1);
        else
            gsb_data_transaction_set_change_between (transaction_number, 0);

        return;
    }

    if (current_exchange.mantissa == 0)
        gsb_currency_exchange_dialog (account_currency_number,
                        transaction_currency_number,
                        0,
                        null_real,
                        null_real,
                        TRUE);

    gsb_data_transaction_set_exchange_rate (transaction_number,
                        gsb_real_abs (current_exchange));
    if (current_exchange_fees.mantissa)
        gsb_data_transaction_set_exchange_fees (transaction_number,
                        current_exchange_fees);
    else
        gsb_data_transaction_set_exchange_fees (transaction_number,
                        null_real);

    gsb_data_transaction_set_change_between (transaction_number, 0);
}


/**
 * ask to the user the exchange and the fees for a transaction
 * fill the variables current_exchange and current_exchange_fees with
 * the data of the user
 * if this was asked before, will use the buffer cached exchange rate, except
 * if force is set to TRUE
 *
 * \param account_currency_number
 * \param transaction_currency_number
 * \param link_currency si = TRUE : 1 nom_devise = "change" devise_en_rapport
 * \param exchange_rate
 * \param exchange_fees
 * \param force if TRUE will not get the cached exchange rate and will really ask to the user
 *
 * \return
 * */
void gsb_currency_exchange_dialog (gint account_currency_number,
                        gint transaction_currency_number ,
                        gboolean link_currency,
                        GsbReal exchange_rate,
                        GsbReal exchange_fees,
                        gboolean force)
{
    GtkWidget *dialog, *label, *hbox, *paddingbox, *table, *widget;
    GtkWidget *entry, *amount_entry, *amount_1_entry, *amount_2_entry, *fees_entry;
	GtkWidget *button_cancel;
	GtkWidget *button_OK;
    GtkWidget *combobox_1;
    GtkWidget *combobox_2;
    CachedExchangeRate *cache;
    gchar* tmpstr;
    gint row = 0;
    gint result;
    gint link_number;
    gint change_link_currency = 1;

    if (account_currency_number == 0 || transaction_currency_number == 0)
        return;

    if (!force
     &&
     (cache = gsb_currency_get_cached_exchange (
     account_currency_number, transaction_currency_number)))
    {
        current_exchange = cache->rate;
        current_exchange_fees = cache->fees;
        return;
    }

    dialog = gtk_dialog_new_with_buttons (_("Enter exchange rate"),
										  GTK_WINDOW (grisbi_app_get_active_window (NULL)),
										  GTK_DIALOG_MODAL,
										  NULL, NULL,
										  NULL);

	button_cancel = gtk_button_new_with_label (_("Cancel"));
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button_cancel, GTK_RESPONSE_CANCEL);
	gtk_widget_set_can_default (button_cancel, TRUE);

	button_OK = gtk_button_new_with_label (_("Validate"));
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button_OK, GTK_RESPONSE_OK);
	gtk_widget_set_can_default (button_OK, TRUE);

	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    /* text for paddingbox */
    tmpstr = g_strdup_printf (_("Please enter data for the transaction"));

    /* Ugly dance to avoid side effects on dialog's vbox. */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (dialog_get_content_area (dialog)), hbox, FALSE, FALSE, 0);
    paddingbox = new_paddingbox_with_title (hbox, TRUE, tmpstr);
    gtk_container_set_border_width (GTK_CONTAINER(hbox), 6);
    gtk_container_set_border_width (GTK_CONTAINER(paddingbox), 6);
    g_free (tmpstr);

    /* table for layout */
    table = gtk_grid_new ();
    gtk_box_pack_start (GTK_BOX (paddingbox), table, FALSE, FALSE, 6);
    gtk_grid_set_column_spacing (GTK_GRID (table), 6);
    gtk_grid_set_row_spacing (GTK_GRID (table), 6);

    /* echange line label */
    label = gtk_label_new (_("Currencies"));
    utils_labels_set_alignment (GTK_LABEL (label), 0.0, 0.0);
    gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

    /* echange line currency 1 */
    combobox_1 = gsb_currency_make_combobox_exchange_dialog (
                        transaction_currency_number,
                        account_currency_number,
                        link_currency);
    gtk_grid_attach (GTK_GRID (table), combobox_1, 1, row, 1, 1);

    /* echange line label */
    label = gtk_label_new (_("Exchange rate"));
    utils_labels_set_alignment (GTK_LABEL (label), 0.5, 0.0);
    gtk_grid_attach (GTK_GRID (table), label, 2, row, 1, 1);

    /* echange line currency 2 */
    combobox_2 = gsb_currency_make_combobox_exchange_dialog (
                        transaction_currency_number,
                        account_currency_number,
                        !link_currency);
    gtk_grid_attach (GTK_GRID (table), combobox_2, 3, row, 1, 1);
    row++;

    link_number = gsb_data_currency_link_search (account_currency_number,
                        transaction_currency_number);
    if (link_number)
    {
        gtk_widget_set_sensitive (combobox_1, FALSE);
        gtk_widget_set_sensitive (combobox_2, FALSE);
    }

    /* amount line */
    label = gtk_label_new (_("Amounts: "));
    utils_labels_set_alignment (GTK_LABEL (label), 0.0, 0.0);
    gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

    amount_1_entry = gtk_entry_new ();
    gtk_entry_set_activates_default (GTK_ENTRY (amount_1_entry), TRUE);
    gtk_grid_attach (GTK_GRID (table), amount_1_entry, 1, row, 1, 1);

    /* echange line input field */
    entry = gtk_entry_new ();
    gtk_widget_set_size_request (entry, 100, -1);
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
    gtk_grid_attach (GTK_GRID (table), entry, 2, row, 1, 1);

    amount_2_entry = gtk_entry_new ();
    gtk_entry_set_activates_default (GTK_ENTRY (amount_2_entry), TRUE);
    gtk_grid_attach (GTK_GRID (table), amount_2_entry, 3, row, 1, 1);

    /* if amount exist already, fill them here */
    if (link_currency)
        amount_entry = amount_2_entry;
    else
        amount_entry = amount_1_entry;

    widget = gsb_form_widget_get_widget (TRANSACTION_FORM_DEBIT);
    if (!gsb_form_widget_check_empty (widget))
        gtk_entry_set_text (GTK_ENTRY (amount_entry),
                        gtk_entry_get_text (GTK_ENTRY (widget)));
    else
    {
        widget = gsb_form_widget_get_widget (TRANSACTION_FORM_CREDIT);
        if (!gsb_form_widget_check_empty (widget))
            gtk_entry_set_text (GTK_ENTRY (amount_entry),
                        gtk_entry_get_text (GTK_ENTRY (widget)));
    }

    /* set the connections */
    g_signal_connect (G_OBJECT (combobox_1),
                        "changed",
                        G_CALLBACK (gsb_currency_select_change_currency),
                        combobox_2);
    g_signal_connect (G_OBJECT (combobox_2),
                        "changed",
                        G_CALLBACK (gsb_currency_select_change_currency),
                        combobox_1);
    g_object_set_data (G_OBJECT (combobox_1),
                        "amount_1_entry", amount_1_entry);
    g_object_set_data (G_OBJECT (combobox_1),
                        "amount_2_entry", amount_2_entry);

    g_object_set_data (G_OBJECT (combobox_2),
                        "amount_1_entry", amount_1_entry);
    g_object_set_data (G_OBJECT (combobox_2),
                        "amount_2_entry", amount_2_entry);

    g_signal_connect (G_OBJECT (amount_1_entry),
                        "changed",
                        G_CALLBACK (gsb_currency_select_double_amount_changed),
                        amount_2_entry);
    g_signal_connect_swapped (G_OBJECT (amount_2_entry),
                        "changed",
                        G_CALLBACK (gsb_currency_select_double_amount_changed),
                        amount_1_entry);
    g_object_set_data (G_OBJECT (amount_1_entry), "exchange_rate", entry);
    g_object_set_data (G_OBJECT (amount_1_entry), "link_currency",
                        GINT_TO_POINTER (link_currency));
    row++;

    /* exchange fees line label */
    label = gtk_label_new (_("Exchange fees: "));
    utils_labels_set_alignment (GTK_LABEL (label), 0.0, 0.0);
    gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

    /* exchange fees line input field */
    fees_entry = gtk_entry_new ();
    gtk_entry_set_activates_default (GTK_ENTRY (fees_entry), TRUE);
    gtk_grid_attach (GTK_GRID (table), fees_entry, 1, row, 1, 1);

    /* exchange fees currency for fees */
    label = gtk_label_new (gsb_data_currency_get_name (account_currency_number));
    utils_labels_set_alignment (GTK_LABEL (label), 0.0, 0.0);
    gtk_grid_attach (GTK_GRID (table), label, 2, row, 1, 1);

    if (link_number)
    {
        GtkWidget *checkbox;

        change_link_currency = !gsb_data_currency_link_get_fixed_link (link_number);
        checkbox = gtk_check_button_new_with_label (_("Change the link"));
        if (change_link_currency)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox), TRUE);
        gtk_widget_set_sensitive (checkbox, TRUE);
        g_signal_connect (G_OBJECT (checkbox),
                        "toggled",
                        G_CALLBACK (gsb_currency_checkbutton_link_changed),
                        &change_link_currency);

        gtk_grid_attach (GTK_GRID (table), checkbox, 3, row, 1, 1);
    }

    /* if the rate or fees exist already, fill them here */
    if (exchange_rate.mantissa)
    {
        tmpstr = utils_real_get_string (exchange_rate);
        gtk_entry_set_text (GTK_ENTRY (entry), tmpstr);
        g_free (tmpstr);
    }

    if (exchange_fees.mantissa)
    {
        tmpstr = utils_real_get_string (gsb_real_abs (exchange_fees));
        gtk_entry_set_text (GTK_ENTRY (fees_entry), tmpstr);
        g_free (tmpstr);
    }

    gtk_widget_show_all (dialog);

    /* show the dialog */
dialog_return:
    result = gtk_dialog_run (GTK_DIALOG (dialog));

    if (result == GTK_RESPONSE_OK)
    {
        gint new_link_number;
        gint new_link_currency;

        current_exchange = utils_real_get_from_string (
                        gtk_entry_get_text (GTK_ENTRY (entry)));

        if (strlen (gtk_entry_get_text (GTK_ENTRY (fees_entry))) > 0
         ||
         strcmp (gtk_entry_get_text (GTK_ENTRY (fees_entry)), "0") != 0)
        {
            current_exchange_fees = utils_real_get_from_string (
                        gtk_entry_get_text (GTK_ENTRY (fees_entry)));
        }
        else
            current_exchange_fees = null_real ;

        if (current_exchange.mantissa == 0)
        {
            tmpstr = g_strdup_printf (_("The exchange rate or the transaction amount in "
                        "%s must be filled."),
                        gsb_data_currency_get_name (account_currency_number));
            dialogue_warning_hint (tmpstr, _("One field is not filled in"));

            goto dialog_return;
        }

        gsb_currency_config_set_cached_exchange (account_currency_number,
                        transaction_currency_number,
                        current_exchange, current_exchange_fees);

        if (link_number)
        {
            if (change_link_currency
             &&
             gsb_real_cmp (current_exchange,
             gsb_data_currency_link_get_change_rate (link_number)) != 0)
                gsb_data_currency_link_set_change_rate (link_number,
                        current_exchange);
        }
        else
        {
            new_link_number = gsb_data_currency_link_new (0);
            new_link_currency = gtk_combo_box_get_active (
                        GTK_COMBO_BOX (combobox_1));
            if (new_link_currency)
            {
                gsb_data_currency_link_set_first_currency (new_link_number,
                        account_currency_number);
                gsb_data_currency_link_set_second_currency (new_link_number,
                        transaction_currency_number);
            }
            else
            {
                gsb_data_currency_link_set_first_currency (new_link_number,
                        transaction_currency_number);
                gsb_data_currency_link_set_second_currency (new_link_number,
                        account_currency_number);
            }
            gsb_data_currency_link_set_change_rate (new_link_number,
                        current_exchange);
        }
    }
    else
        gsb_currency_init_exchanges ();

    gtk_widget_destroy (GTK_WIDGET (dialog));
}

/*
 * initialize current_exchange and current_exchange_fees
 *
 */
void gsb_currency_init_exchanges (void)
{
    current_exchange = null_real;
    current_exchange_fees = null_real;
}




/**
 *
 *
**/
GsbReal gsb_currency_get_current_exchange (void)
{
    return current_exchange;
}


/**
 *
 *
**/
gboolean gsb_currency_set_current_exchange (GsbReal exchange)
{
    current_exchange.mantissa = exchange.mantissa;
    current_exchange.exponent = exchange.exponent;

    return FALSE;
}


/**
 *
 *
**/
GsbReal gsb_currency_get_current_exchange_fees (void)
{
    return current_exchange_fees;
}


/**
 *
 *
**/
gboolean gsb_currency_set_current_exchange_fees (GsbReal fees)
{
    current_exchange_fees.mantissa = fees.mantissa;
    current_exchange_fees.exponent = fees.exponent;

    return FALSE;
}



/**
 *
 *
 * \param
 * \param
 *
 * \return
 **/
gboolean gsb_currency_add_currency_set_combobox (GtkWidget *button,
												 GtkWidget *combobox)
{
    gsb_currency_dialog_list_iso_4217_new (NULL, TRUE);
    gsb_currency_set_combobox_history (combobox, gsb_data_currency_max_number ());

    return FALSE;
}

/**
 * Create a new GtkComboBox with a pointer to an integer that will be
 * modified according to the entry's value.
 *
 * \param value		A pointer to a gint which contains the currency number
 * \param hook 		An optional function to execute as a handler
 *
 * \return A newly allocated option menu.
 **/
GtkWidget *gsb_currency_combobox_new (gint *value,
									  GCallback hook)
{
    GtkWidget *combo_box;

    combo_box = gsb_currency_make_combobox (FALSE);

    if (value && *value)
		gsb_currency_set_combobox_history (combo_box, *value);

    g_signal_connect (G_OBJECT (combo_box),
					  "changed",
					  (GCallback) gsb_currency_combobox_value_changed,
					  value);
    g_object_set_data (G_OBJECT (combo_box), "pointer", value);

    if (hook)
		g_object_set_data (G_OBJECT (combo_box),
						   "changed-hook",
						   GUINT_TO_POINTER (g_signal_connect_after (G_OBJECT(combo_box),
																	 "changed",
																	 G_CALLBACK (hook),
																	 value)));
    return combo_box;
}

/**
 *
 *
 * \param
 * \param
 *
 * \return
 **/
gint gsb_currency_dialog_list_iso_4217_new (GtkWidget *page_currency,
											gboolean no_callback)
{
    GtkWidget *dialog;
	GtkWidget *button_close;
    GtkWidget *content_area;
    GtkWidget *details;
    GtkWidget *popup;
	GtkWidget *tree_view;
    GtkWidget *vbox;
    GtkTreeSelection *selection;
    gint currency_number = 0;
    gint result;

	dialog = gtk_dialog_new_with_buttons (_("Add a currency"),
										  GTK_WINDOW (grisbi_app_get_active_window (NULL)),
										  GTK_DIALOG_MODAL,
										  NULL, NULL,
										  NULL);

	button_close = gtk_button_new_with_label (_("Close"));
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button_close, GTK_RESPONSE_ACCEPT);
	gtk_widget_set_can_default (button_close, TRUE);

    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_widget_set_size_request (dialog, 650, 500);

	content_area = dialog_get_content_area (dialog);
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (content_area), vbox, TRUE, TRUE, 0);
	utils_widget_set_padding (vbox, MARGIN_BOX, MARGIN_BOX);

	/* get currency_popup */
	popup = gsb_popup_list_iso_4217_new (NULL, NULL);
	tree_view = g_object_get_data (G_OBJECT (popup), "tree_view" );
    gtk_box_pack_start (GTK_BOX (vbox), popup, TRUE, TRUE, MARGIN_BOX);

	/* get currentcy_details */
	details = GTK_WIDGET (widget_currency_details_new (page_currency, no_callback));
	widget_currency_details_set_entry_editable (details, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), details, FALSE, FALSE, MARGIN_BOX);

	/* set selection signal to update first currency */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
	g_signal_connect (G_OBJECT (selection),
					  "changed",
					  G_CALLBACK (gsb_popup_list_selection_changed),
					  details);

	/* Select default currency. */
    gtk_tree_model_foreach (GTK_TREE_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view))),
										   (GtkTreeModelForeachFunc) gsb_popup_list_select_default,
										   tree_view);

	gtk_widget_show_all (dialog);
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

    result = gtk_dialog_run (GTK_DIALOG (dialog));

    if (result)
    {
		currency_number = gsb_popup_list_selected_currency_new (selection);
		if (currency_number)
		{
			/* update the currencies list in account properties */
			gsb_currency_update_combobox_currency_list ();
		}
    }
    gtk_widget_destroy (GTK_WIDGET (dialog));

	return currency_number;
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
