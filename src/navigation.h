#ifndef NAVIGATION_H
#define NAVIGATION_H


/** Holds data for the navigation tree.  */
enum navigation_cols { 
    NAVIGATION_PIX, NAVIGATION_PIX_VISIBLE,
    NAVIGATION_TEXT, NAVIGATION_FONT,
    NAVIGATION_PAGE, NAVIGATION_ACCOUNT,
    NAVIGATION_REPORT, NAVIGATION_SENSITIVE,
    NAVIGATION_TOTAL,
};


/* START_INCLUDE_H */
#include "navigation.h"
/* END_INCLUDE_H */


/* START_DECLARATION */
GtkWidget * create_navigation_pane ( void );
void gsb_gui_navigation_add_account ( gint account_nb );
void gsb_gui_navigation_add_report ( gint report_number );
gint gsb_gui_navigation_get_current_account ( void );
gint gsb_gui_navigation_get_current_page ( void );
gint gsb_gui_navigation_get_current_report ( void );
void gsb_gui_navigation_remove_account ( gint account_nb );
void gsb_gui_navigation_remove_report ( gint report_number );
gboolean gsb_gui_navigation_select_next ();
gboolean gsb_gui_navigation_select_prev ();
gboolean gsb_gui_navigation_set_selection ( gint page, gint account_nb, gpointer report );
void gsb_gui_navigation_update_account ( gint account_nb );
void gsb_gui_navigation_update_report ( gint report_number ) ;
gboolean navigation_drag_data_received ( GtkTreeDragDest * drag_dest,
					 GtkTreePath * dest_path,
					 GtkSelectionData * selection_data );
gboolean navigation_row_drop_possible ( GtkTreeDragDest * drag_dest, 
					GtkTreePath * dest_path,
					GtkSelectionData * selection_data );
/* END_DECLARATION */

#endif
