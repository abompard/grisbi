#ifndef STRUCTURES_H
#define STRUCTURES_H (1)

/* fichier d'en tête structures.h */
/* contient toutes les structures du prog */

#include "print_config.h"
#include "utils_str.h"

#define VERSION_FICHIER "0.6.0"
#define VERSION_FICHIER_ETAT "0.6.0"
#define VERSION_FICHIER_CATEG "0.6.0"
#define VERSION_FICHIER_IB "0.6.0"

/* constante de debug originale, obsolete il faudrait utiliser maintenant */
/* la variable globale debugging_grisbi, voir erreur.c */
#define DEBUG utils_str_atoi (getenv ("DEBUG_GRISBI"))

/* constantes definissant le niveau de debug */
#define	DEBUG_LEVEL_ALERT			1		/* grave probleme */
#define DEBUG_LEVEL_IMPORTANT			2		/* probleme moins grave */
#define DEBUG_LEVEL_NOTICE			3		/* probleme encore moins grave :) */
#define DEBUG_LEVEL_INFO			4		/* autre information */
#define DEBUG_LEVEL_DEBUG			5		/* information de debug */
#define MAX_DEBUG_LEVEL				5		/* ignore higher debug levels */
/* constante indiquant dans quelle fichier on se trouve */
/* c'est un alias pratique pour la fonction debug */
#define DEBUG_WHERE_AM_I g_strdup_printf("%s line %05d",__FILE__,__LINE__)

/* initialisation des couleurs */

#define COULEUR1_RED  55000
#define COULEUR1_GREEN  55000
#define COULEUR1_BLUE  65535
#define COULEUR2_RED  65535
#define COULEUR2_GREEN  65535
#define COULEUR2_BLUE  65535
#define COULEUR_SELECTION_RED  63000
#define COULEUR_SELECTION_GREEN  40000
#define COULEUR_SELECTION_BLUE  40000
#define COULEUR_ROUGE_RED  65535
#define COULEUR_ROUGE_GREEN  0
#define COULEUR_ROUGE_BLUE  0
#define COULEUR_NOIRE_RED 0
#define COULEUR_NOIRE_GREEN 0
#define COULEUR_NOIRE_BLUE 0
#define COULEUR_GRISE_RED 50000
#define COULEUR_GRISE_GREEN 50000
#define COULEUR_GRISE_BLUE 50000

#define COLON(s) (g_strconcat ( s, " : ", NULL ))
#define SPACIFY(s) (g_strconcat ( " ", s, " ", NULL ))
#define PRESPACIFY(s) (g_strconcat ( " ", s, NULL ))
#define POSTSPACIFY(s) (g_strconcat ( s, " ", NULL ))


/* variables contenant juste 0 ou 1 */

struct {
    gint modification_fichier;
    gint ctrl;
    gint equilibrage;
    gint r_modifiable;
    gint dernier_fichier_auto;
    gint sauvegarde_auto;             /* utilisé pour enregistrer le fichier automatiquementà la fermeture */
    gint sauvegarde_demarrage;        /* utilisé pour enregistrer le fichier s'il s'est bien ouvert */
    gint entree;   			 /* si etat.entree = 1, la touche entrée finit l'opération */ 
    gint alerte_mini;
    gint formulaire_toujours_affiche;
    gint alerte_permission;   /* à un si le message d'alerte s'affiche */
    gint fichier_deja_ouvert;   /* à un si lors de l'ouverture, le fichier semblait déjà ouvert */
    gint force_enregistrement;    /* à un si on force l'enregistrement */
    gint affichage_exercice_automatique;   /* à 1 si exercice automatique selon la date, 0 si affiche le dernier choisi */
    gint crypt_file;   /* TRUE if we want to crypt the file */
    gint compress_file;  /* TRUE if we want to compress the grisbi file */
    gint compress_backup;  /* TRUE if we want to compress the backup */

    gint formulaire_distinct_par_compte;  /* à 1 si le formulaire est différent pour chaque compte */
    gint affiche_boutons_valider_annuler;
    gint affiche_nb_ecritures_listes;
    gint classement_par_date;   /* à 1 si le classement de la liste d'opé se fait par date */
    gint largeur_auto_colonnes;
    gint retient_affichage_par_compte;   /* à 1 si les caractéristiques de l'affichage (R, non R ...) diffèrent par compte */
    gint en_train_de_sauvegarder;
    gint en_train_de_charger;
    gint utilise_logo;
    gint utilise_fonte_listes;
    gint affichage_grille;         /* à 1 si la grille est affichée */
    gint display_toolbar;
    gint show_closed_accounts;

    /* Various display message stuff  */
    gint display_message_lock_active;
    gint display_message_file_readable;
    gint display_message_minimum_alert;
    gint display_message_no_budgetary_line;
    gint display_message_qif_export_currency;
    gint display_message_ofx_security;

    /* Various commands */
    gchar * browser_command;
    gchar * latex_command;
    gchar * dvips_command;

    /*     chemin de l'image affichée quand on patiente */

    gchar *fichier_animation_attente;

    /* Print stuff */
    struct print_config print_config;
    
    /*     largeur des panned */

    gint largeur_colonne_comptes_operation;
    gint largeur_colonne_echeancier;
    gint largeur_colonne_comptes_comptes;
    gint largeur_colonne_etat;

    /* variables sur l'échéancier */

    gint formulaire_echeancier_toujours_affiche;
    gint formulaire_echeance_dans_fenetre;        /* à 1 lorsque le formulaire de l'échéancier est dans une fenetre à part */
    gint affichage_commentaire_echeancier;     /* à 1 si le commentaire est affiché */

    /* Tips */
    gint last_tip; 
    gint show_tip; 
} etat;


/* devises */

struct struct_devise
{
    gint no_devise;
    gchar *nom_devise;
    gchar *code_iso4217_devise;
    gchar *code_devise;

    /*     tous ces champs sont obsolètes, mais gardés pour les anciennes devises qui sont passées à l'euro */
    /* 	ils ne doivent plus être utilisés sauf dans ce cas */

    gint passage_euro;                                  /* à 1 si cette devise doit passer à l'euro */
    GDate *date_dernier_change;                     /*   dernière mise à jour du change, NULL si aucun change */
    gint une_devise_1_egale_x_devise_2;       /* si = 1 : 1 nom_devise = "change" devise_en_rapport */
    gint no_devise_en_rapport;
    gdouble change;                                          /* taux de change */
};

#define NOM_DEVISE_ASSOCIEE GTK_LABEL (GTK_BIN ( GTK_BUTTON ( &(GTK_OPTION_MENU (option_menu_devises) -> button ))) -> child ) -> label


/* banques */

struct struct_banque
{
    gint no_banque;
    gchar *code_banque;
    gchar *nom_banque;
    gchar *adr_banque;
    gchar *tel_banque;
    gchar *email_banque;
    gchar *web_banque;
    gchar *remarque_banque;
    gchar *nom_correspondant;
    gchar *tel_correspondant;
    gchar *email_correspondant;
    gchar *fax_correspondant;
};



struct struct_exercice
{
    gint no_exercice;
    gchar *nom_exercice;
    GDate *date_debut;
    GDate *date_fin;
    gint affiche_dans_formulaire;
};

struct struct_type_ope
{
    gint no_type;
    gchar *nom_type;
    gint signe_type;   /* 0=neutre, 1=débit, 2=crédit */
    gint affiche_entree;    /* par ex pour virement, chèques ... */
    gint numerotation_auto;        /* pour les chèques */
    gint no_en_cours;
    gint no_compte;
};



/* la struct_ope_liee est utilisée pour le passage de la 0.3.1 à la 0.3.2 */
/* pour transformer les no d'opés */
/* à virer ensuite */

struct struct_ope_liee
{
    gint ancien_no_ope;
    gint compte_ope;
    gint nouveau_no_ope;
    gpointer ope_liee;
    gint ope_ventilee;
};

/* si en 2 parties, séparé d'un - */
/* si la 2ème est un numéro, elle est incrémentée à chaque équilibrage */

struct struct_no_rapprochement
{
    gint no_rapprochement;
    gchar *nom_rapprochement;
};



enum alignement {
    LEFT, CENTER, RIGHT,
};


/* struture d'une opé de ventil */

struct struct_ope_ventil
{
    gint no_operation;

    gdouble montant;

    gint categorie;
    gint sous_categorie;

    gchar *notes;

    gint imputation;
    gint sous_imputation;
    gchar *no_piece_comptable;

    gint relation_no_operation;      /* contient le no de l'opé associée quand elle est déjà créée, -1 si c'est un virement non créé */
    gint relation_no_compte;
    gint no_type_associe;

    gint pointe;

    gint no_exercice;

    gint supprime;   /* à 1 quand cette opé doit être supprimée */
    gint par_completion;    /*à 1 si cette opé a été ajoutée suite à une complétion de tiers*/
};


/* Handle an ISO 4217 currency.  Not specific to Grisbi. */
struct iso_4217_currency 
{
    gchar * continent;
    gchar * currency_name;
    gchar * country_name;
    gchar * currency_code;
    gchar * currency_nickname;
    gboolean active;
    gchar *flag_filename;
};


#define STANDARD_WIDTH 6
#define STANDARD_DIALOG_WIDTH 12

/* organisation du formulaire */

struct organisation_formulaire
{
    /*     nombre de colonnes : de 3 à 6 */

    gint nb_colonnes;

    /*     nombre de lignes : de 1 à 4 */

    gint nb_lignes;

    /*     remplissage du formulaire */
    /* 	1: date (obligatoire) */
    /* 	2: débit (obligatoire) */
    /* 	3: crédit (obligatoire) */
    /* 	4: date de val */
    /* 	5: exo */
    /* 	6: tiers */
    /* 	7: categ */
    /* 	8: bouton de ventil */
    /* 	9: ib */
    /* 	10: notes */
    /* 	11: mode de paiement */
    /* 	12: no de chq */
    /* 	13: devise */
    /* 	14: bouton change */
    /* 	15: pièce comptable */
    /* 	16: info b/g */
    /* 	17: mode paiement contre opé */
	
    gint tab_remplissage_formulaire[4][6];

    /*     rapport de taille entre les colonnes */

    gint taille_colonne_pourcent[6];

};
#endif
