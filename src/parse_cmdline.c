/* *******************************************************************************/
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

#include "include.h"

#include <stdio.h>
#include <getopt.h>

/*START_INCLUDE*/
#include "parse_cmdline.h"
#include "./main.h"
#include "./utils_str.h"
#include "./include.h"
/*END_INCLUDE*/

/*START_STATIC*/
static gboolean is_valid_window_number(gint w);
static CMDLINE_ERRNO parse_tab_parameters(char *tab_parameters, cmdline_options* pOpt);
static void show_errstr(FILE* output, gint errval, gchar* extra);
static void show_help(FILE* output);
static void show_synoptic(FILE* output);
static void   show_usage(FILE* output, gint errval, gchar* extra);
static void show_version(FILE* output);
/*END_STATIC*/


static char*         short_options  = "t:vh"; /*!< configure the list of 'short' options using getopt syntax */
static struct option long_options[] =           /*!< configure the list of 'long'  options using getopt syntax */
{
    {"version", no_argument,       0, 'V'},     /*!< '--version   */
    {"help"   , no_argument,       0, 'H'},     /*!< '--help'     */
    {"tab",     required_argument, 0, 'T'},     /*!< '--tab=s' */
    {0, 0, 0, 0} /* end of long arg list DO NOT REMOVE */
};

/* Please make sure to keep consistency between short_options/long_options and USAGE_STRING/HELP_STRING definition */
#define   GET_DETAIL N_("Try grisbi --help for details\n\n")
#define   USAGE_STRING  		        						\
 N_("Usage: gribi [--version | -v] | [--help | -?] |" 				\
	"[--tab=w[,x[,y[,z]]] | -t w[,x[,y[,z]]]] [file]\n"      \
 )

#define   HELP_STRING  				                               \
 N_("gribi [options] [file]\n"                                                 \
    "  -?, --help                    Show this help message and exit\n"        \
    "  -v, --version                 Show version number and exit\n"           \
    "  -t, --tab=w[,x[,y[,z]]]    Open file and show specified tab\n"       \
    "              w=-1              Show configuration window\n"              \
    "              w=[0-6]           Show tab no <w>\n"                        \
    "              w=7,x[,y[,z]]]    Show report pane\n"                       \
    "                                    x: report number to show\n"           \
    "                                    y: show main customization tab\n"     \
    "                                       number <y> of report\n"            \
    "                                    z: show custmization tab number <z>\n"\
    "                                       of report\n\n"                     \
   )

/**
 *
 * Parse command line arguments using getopt feature for options.
 *
 * \param argc main() argc (or argn) parameter.
 * \param argv main() argv parameter.
 * \param pOpt pointer to the options structure to store configuration in.
 * \param errval pointer to put the operation status:
 *
 * \return TRUE when the application can continue, FALSEthe application should stop
 *
 * \note  See short_options and long_options variables to have possible options.
 */
gboolean  parse_options(int argc, char **argv, cmdline_options *pOpt, gint* pErrval)
{
    gboolean still_args_to_treat = TRUE; /*!< used to stop treating the command line args using getopt */ 
    int      option_index        = 0;    /*!< Index of the field of argv to compute */
    gboolean app_must_stop       = FALSE;
	
    *pErrval = CMDLINE_ERROR(CMDLINE_SYNTAX_OK);
	/* init param de retour */
	pOpt->demande_page         = 0;
	pOpt->fichier              = NULL;
	pOpt->report_no            = -1;
	pOpt->customization_tab_no = -1;
	pOpt->subcustom_tab_no     = -1;

    while ((CMDLINE_SYNTAX_OK==CMDLINE_ERROR(*pErrval)) && (still_args_to_treat) && (!app_must_stop))
	{
        int c;
        switch (c = getopt_long(argc, argv, short_options,long_options, &option_index))
		{
            /* special getopt return values */
            case ':':/* missing parameter for one of the  options */
                *pErrval = CMDLINE_ERROR(CMDLINE_MISSING_PARAMETER);
                show_usage(stderr, (gint)(*pErrval), argv[option_index]);
                break;
            default: /* unmanaged options */
                *pErrval = CMDLINE_ERROR(CMDLINE_SYNTAX_ERROR);
                show_usage(stderr, (gint)(*pErrval), argv[option_index]);
                break;
            case '?':/* unknown option character */
                *pErrval = CMDLINE_ERROR(CMDLINE_UNKNOWN_OPTION);
                show_usage(stderr, (gint)(*pErrval), argv[option_index]);
                break;
            case EOF:/* all options parsed */
                still_args_to_treat = FALSE;
                break;
            
            /* configured returned values */
            /* -h : short help = usage */
            case 'v' : /* -v */
            case 'V' : /* --version */
                app_must_stop       = TRUE;// stopping here!
                show_version(stdout);    
                break;
            case 'h':  /* -h */
            case 'H':  /* --help */
                app_must_stop       = TRUE;// stopping here!
                show_help(stdout);
                break; 
            case 't': /* -t <str> */
            case 'T': /* --tab[=]<optarg> */
                if (!optarg)
                {
                    /* Denote a getopt configuration error, arg is required should never happend, but ... */
                    *pErrval = CMDLINE_ERROR(CMDLINE_GETOPT_CONFIGURATION_ERROR);
                    show_usage(stderr,(gint)(*pErrval),"");
			} 
                else
			{
                    /* optarg contains argument parameter */
                    if (CMDLINE_SYNTAX_OK != (*pErrval = parse_tab_parameters(optarg, pOpt)))
                        show_usage(stderr,(gint)(*pErrval),"");
			} 
                break; 
				}
			}
    /* Manual management of command line arguments */
    if(optind >= argc) /* no more arguments */
			{
			}
    else while (optind < argc)
		{
        /* first found arg should be a gsb file */
        /* so if Opt->fichier is not NULL, we have more than one extra arg! */
        if (pOpt->fichier != NULL)
			{
            show_usage(stderr,CMDLINE_ERROR(CMDLINE_TOO_MANY_PARAMETERS),argv[optind]);
            break;
			}
        pOpt->fichier = g_strdup(argv[optind]);
        optind++;
		}
		
    return ((gboolean)((*pErrval)==0)&&(!app_must_stop));
	
}


/**
 * 
 * Display grisbi synoptic
 *
 * \param output to choose between stdout and stderr or another file descriptor
 *
 * \private
 */
void show_synoptic(FILE* output)
{ 
    fprintf(output,_("\nGrisbi\n\n  Personal accounting program under GNU General Public Licence\n\n")); 
}


/**
 * 
 * Display grisbi version after synoptic
 *
 * \param output to choose between stdout and stderr or another file descriptor
 *
 **/
void show_version(FILE* output)
{
    show_synoptic(output);
    g_print(N_("Version %s\n\n"), VERSION);	
}


/**
 *
 * Display complete help message
 *
 * \param output to choose between stdout and stderr or another file descriptor
 *
 * \private
 */
void show_help(FILE* output)
{
    show_synoptic(output);
    show_usage(output,0,NULL);
    fprintf(output,HELP_STRING);
}


/**
 *
 * Display usage line
 * 
 * \param output to choose between stdout and stderr or another file descriptor
 * \param errval  error code to pass to show_errstr
 * \param extra  extra information to pass to show_errstr
 *
 * \private
 */
void   show_usage(FILE* output, gint errval, gchar* extra)
{
    show_errstr(output,errval,extra);
    fprintf(output,USAGE_STRING);
}


/**
 *
 * Display a string corresponding to error code 
 *
 * \param output to choose between stdout and stderr or another file descriptor
 * \param errval  error code to inform the user why this message is displayed
 * \param extra  extra information to complete error code
 *
 * \private
 */
void show_errstr(FILE* output, gint errval, gchar* extra)
{
    switch (CMDLINE_ERROR(errval))
    {
        case CMDLINE_SYNTAX_OK:/* do nothing!*/
            break;
        case CMDLINE_MISSING_PARAMETER:
            fprintf(output,_("Missing parameter for option %s\n\n"),extra);
            break;
        case CMDLINE_SYNTAX_ERROR:
            fprintf(output,_("Syntax error (%s)\n\n"),extra);
            break;
        case CMDLINE_UNKNOWN_OPTION:
             fprintf(output,_("Unknown option (%s)\n\n"),extra);
            break;
        case CMDLINE_TOO_MANY_PARAMETERS:
            fprintf(output,_("Too many parameters (%s)\n\n"),extra);
            break;
        case CMDLINE_TAB_ID_OUT_OF_RANGE:
            fprintf(output,_("Tab is is out of range (%s)\n\n"),extra);
            break;
        case CMDLINE_GETOPT_CONFIGURATION_ERROR:
            fprintf(output,_("Something strange happend (%s)\n\n"),extra);
            break;            
        default:
            fprintf(output,_("Syntax error!\n\n"));
            break;
    }
}


/**
 * Analyse tab argument parameter to extra information.
 *
 * \param tab_parameters string of tab parameters (the one given by getopt!)
 * \param pOpt pointer to the configuration structure to fill
 *
 * \return status of the operation is CMDLINE_ERRNO values
 * \retval CMDLINE_SYNTAX_OK
 * \retval CMDLINE_MISSING_PARAMETER
 * \retval CMDLINE_TAB_ID_OUT_OF_RANGE
 *
 */
CMDLINE_ERRNO parse_tab_parameters(char *tab_parameters, cmdline_options* pOpt)
{
	gchar **split_chiffres;
	gint    w, x, y, z;
	
	w = x = y = z = -1;

    split_chiffres = g_strsplit ( tab_parameters, ",", 0 );
	
	if (split_chiffres[0] == NULL)
	{
		/* on s'attend à avoir au moins un numero d'onglet */
        return CMDLINE_MISSING_PARAMETER;
	}
	
	w = utils_str_atoi(split_chiffres[0]); 
	if ( !is_valid_window_number(w) )
	{
        return CMDLINE_TAB_ID_OUT_OF_RANGE;
	}

	if (w == 7)
	{
		/* Fenetre des etats. Lit parametres restant si il y en a. */
		if (split_chiffres[1]) 
		{
			x = utils_str_atoi(split_chiffres[1]);
		
			if (split_chiffres[2]) 
			{
				y = utils_str_atoi(split_chiffres[2]);
				if (split_chiffres[3]) 
				{
					z = utils_str_atoi(split_chiffres[3]);
				}
			}
		}
	}
	

	/* Tout c'est bien passé, on peut ranger les valeurs dans la var. de retour */
	pOpt->demande_page         = 1;
	
	pOpt->page_w               = w;
	pOpt->report_no            = x;
	pOpt->customization_tab_no = y;
	pOpt->subcustom_tab_no     = z;
	
    return CMDLINE_SYNTAX_OK;

}

	


#define  NB_MAX_ONGLET    8 
/**
 * 
 * Check if a value can be a tab index.
 * 
 * A Tab index is in -1 (config dialog) to NB_MAX_ONGLET-1.
 *
 * \param w value to check 
 *
 * \return TRUE is w can be a tab index, FALSE otherwise.
 */
gboolean is_valid_window_number(gint w)
{
	/* Valeurs valides:
	 * 1)  -1: fenêtre de configuration
	 * 2)  0..NB_MAX_ONGLET-1: onglet
	 */
	return ((w >= -1) && (w < NB_MAX_ONGLET));
}


/* Local Variables: */
/* c-basic-offset: 4 */
/* End: */
