/* $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include <xview/text.h>

#if (defined(TEXT))
#undef TEXT
#endif

#if (defined(TEXT_TYPE))
#undef TEXT_TYPE
#endif

#include "genC.h"
#include "ri.h"
#include "ri-util.h"
#include "makefile.h"
#include "database.h"

#include "misc.h"
#include "pipsdbm.h"
#include "pipsmake.h"
#include "top-level.h"
#include "wpips.h"

#include "resources.h"


/* Maximum size of the module menu of the main frame: */
enum {
   WPIPS_MAX_MODULE_MENU_SIZE = 50
};


Menu_item directory_menu_item;

static char * workspace_name_to_create;

/* To enable or disable the menu items generated by generate_module_menu(): */
bool wpips_close_workspace_menu_inactive,
wpips_close_workspace_menu_inactive_old,
wpips_create_workspace_menu_inactive,
wpips_open_workspace_menu_inactive,
wpips_delete_workspace_menu_inactive,
wpips_change_directory_inactive,
wpips_change_directory_inactive_old;


/* Try to select a main module (that is the PROGRAM in the Fortran
   stuff) if no one is selected: */
void
select_a_module_by_default()
{
    char *module_name = db_get_current_module_name();

    if (module_name == NULL) {
	/* Ok, no current module, then find a main module (PROGRAM): */
	string main_module_name = get_first_main_module();
      
	if (!string_undefined_p(main_module_name)) {
	    /* Ok, we got it ! Now we select it: */
	    module_name = main_module_name;
	    user_log("Main module PROGRAM \"%s\" found.\n", module_name);
	    end_select_module_notify(module_name);
	    /* GO: show_module() has already been called so return now */
	    return;
	}
    }

    /* Refresh the module name on the status window: */
    show_module();
}


success
end_directory_notify(char * dir)
{
   char *s;

/*   if (dir != NULL) {*/
   if ((s = pips_change_directory(dir)) == NULL) {
      user_log("Directory \"%s\" does not exist\n", dir);
      prompt_user("Directory \"%s\" does not exist\n", dir);
      show_directory();
      return FALSE;
   }
   else {
      user_log("Directory \"%s\" selected\n", dir);
      show_directory();
      return TRUE;
   }
}


void
end_directory_text_notify(Panel_item text_item,
                          Event * event)
{
   (void) end_directory_notify((char *) xv_get(text_item, PANEL_VALUE));
}

/* FC: uses an external wish script 
 * I do not know how to activate this function safely...
 */
void
direct_change_directory()
{
    char newdir[MAXPATHLEN];
    char * tmp = strdup("/tmp/wpips.dir.XXXXXX");
    int i=0, c;
    FILE * tmph;

    (void) mktemp(tmp);

    if (wpips_change_directory_inactive)
	return; /* no cd in this state! */

    safe_system(concatenate("Wchangedir -- ", get_cwd(), " > ", tmp, 0));

    tmph = safe_fopen(tmp, "r");
    while ((c=getc(tmph))!=EOF && i<MAXPATHLEN)
	newdir[i++]=c;
    newdir[i-1]='\0'; /* last was \n */
    safe_fclose(tmph, tmp);

    end_directory_notify(newdir);

    unlink(tmp); free(tmp);

    return /* generate_workspace_menu(); */ ;
}

Menu
generate_directory_menu()
{
    return generate_a_directory_menu(get_cwd());
}


void
prompt_user_not_allowed_to_change_directory(Panel_item text_item,
                                            Event * event)
{
   /* First untype whatever the user typed: */
   show_directory();
   prompt_user("You have to close the current workspace"
	       " before changing directory.");
}

                                            
void start_directory_notify(menu, menu_item)
     Menu menu;
     Menu_item menu_item;
{
  if (db_get_current_workspace_name())
     prompt_user_not_allowed_to_change_directory(
	 (Panel_item)NULL, (Event*)NULL);
  else
    start_query("Change Directory",
		"Enter directory path: ", 
		"ChangeDirectory",
		end_directory_notify,
		cancel_query_notify);
}


void
enable_change_directory()
{
   xv_set(directory_menu_item, MENU_INACTIVE, FALSE, NULL);

   /* Enable the normal notify mode: notify when return and so is
      typed: */
   xv_set(directory_name_panel_item,
          PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
          PANEL_NOTIFY_PROC, end_directory_text_notify,
          PANEL_INACTIVE, FALSE,
          NULL);
   wpips_change_directory_inactive_old = wpips_change_directory_inactive;
   wpips_change_directory_inactive = FALSE;
}


void
disable_change_directory()
{
   xv_set(directory_menu_item, MENU_INACTIVE, TRUE, NULL);

   /* In order to warn the user as soon as possible that (s)he can't
      change the directory, notify for whatever character: */
   xv_set(directory_name_panel_item,
          PANEL_NOTIFY_LEVEL, PANEL_ALL,
          PANEL_NOTIFY_PROC, prompt_user_not_allowed_to_change_directory,
          PANEL_INACTIVE, TRUE,
          NULL);
   wpips_change_directory_inactive_old = wpips_change_directory_inactive;
   wpips_change_directory_inactive = TRUE;
}


void static
restore_enable_change_directory_state()
{
   if (wpips_change_directory_inactive_old)
      disable_change_directory();
   else
      enable_change_directory();
}


static Menu_item create_pgm, open_pgm, close_pgm, module_item;


void
disable_workspace_create_or_open()
{
   xv_set(create_pgm,
          MENU_INACTIVE, TRUE,
          NULL);
   wpips_create_workspace_menu_inactive = TRUE;
   xv_set(open_pgm,
          MENU_INACTIVE, TRUE,
          NULL);
   xv_set(workspace_name_panel_item,
          PANEL_INACTIVE, TRUE,
          NULL);
}


void
enable_workspace_create_or_open()
{
   xv_set(create_pgm,
          MENU_INACTIVE, FALSE,
          NULL);
   wpips_create_workspace_menu_inactive = FALSE;
   xv_set(open_pgm,
          MENU_INACTIVE, FALSE,
          NULL);
   xv_set(workspace_name_panel_item,
          PANEL_INACTIVE, FALSE,
          NULL);
}


void
disable_workspace_delete_or_open()
{
   /* Avoid also to delete a workspace during a creation in case the
      name of the workspace is the same... */
   wpips_delete_workspace_menu_inactive = TRUE;
   wpips_open_workspace_menu_inactive = TRUE;
   xv_set(workspace_name_panel_item,
          PANEL_INACTIVE, TRUE,
          NULL);
}


void
enable_workspace_delete_or_open()
{
   wpips_delete_workspace_menu_inactive = FALSE;
   wpips_open_workspace_menu_inactive = FALSE;
   xv_set(workspace_name_panel_item,
          PANEL_INACTIVE, FALSE,
          NULL);
}


void
disable_workspace_close()
{
   xv_set(close_pgm, MENU_INACTIVE, TRUE, NULL);
   wpips_close_workspace_menu_inactive_old = wpips_close_workspace_menu_inactive;
   /* For generate_module_menu(): */
   wpips_close_workspace_menu_inactive = TRUE;
}


void
enable_workspace_close()
{
   xv_set(close_pgm, MENU_INACTIVE, FALSE, NULL);
   /* For generate_module_menu(): */
   wpips_close_workspace_menu_inactive = FALSE;
}


void
restore_enable_workspace_close_state()
{
   wpips_close_workspace_menu_inactive = wpips_close_workspace_menu_inactive_old;
}


void
disable_module_selection()
{
   xv_set(module_item, MENU_INACTIVE, TRUE, NULL);
   xv_set(module_name_panel_item, PANEL_INACTIVE, TRUE, NULL);
   disable_view_selection();
   disable_transform_selection();
   disable_compile_selection();
   disable_option_selection();
}


void
enable_module_selection()
{
   xv_set(module_item, MENU_INACTIVE, FALSE, NULL);
   xv_set(module_name_panel_item, PANEL_INACTIVE, FALSE, NULL);
   /* Well, after a workspace creation without automatic module
      selection successful, there is no module selected and thus the
      following menus are not very interestiong, except for the option
      part of the option selection. Anyway, there is a guard in each
      of these menu, so, just do nothing... :-) */
   enable_view_selection();
   enable_transform_selection();
   enable_compile_selection();
   enable_option_selection();
}


void
end_delete_workspace_notify(char * name)
{
   schoose_close();

   if (db_get_current_workspace_name() != NULL
       && strcmp(db_get_current_workspace_name(), name) == 0)
   {
      int result;
      /* Send to emacs if we are in the emacs mode: */
      if (wpips_emacs_mode) 
         send_notice_prompt_to_emacs("The workspace",
                                     name,
                                     "is currently opened!",
                                     "Do you really want to close and remove it ?",
                                     NULL);
      result = notice_prompt(xv_find(main_frame, WINDOW, 0),
                             NULL,
                             NOTICE_MESSAGE_STRINGS,
                             "The workspace",
                             name,
                             "is currently opened!",
                             "Do you really want to close and remove it ?",
                             NULL,
                             NOTICE_BUTTON_YES,  "Yes, remove the database",
                             NOTICE_BUTTON_NO,   "No, cancel",
                             NULL);
      if (result == NOTICE_NO)
         goto do_not_delete_the_workspace;
      
      /* First close the workspace: */
      if (!close_workspace_notify((Menu)NULL, (Menu_item)NULL))
         /* Hmm... This may put WPips in a strange state if close
            fails... */
         goto do_not_delete_the_workspace;
   }

   (void) delete_workspace(name);

  do_not_delete_the_workspace:
   enable_workspace_delete_or_open();
   enable_workspace_create_or_open();
   restore_enable_workspace_close_state();
   restore_enable_change_directory_state();
   
   display_memory_usage();
}


void
cancel_delete_workspace_notify()
{
   /* Nothing to do. */
   enable_workspace_delete_or_open();
   enable_workspace_create_or_open();
   restore_enable_workspace_close_state();
   restore_enable_change_directory_state();
}


void
start_delete_workspace_notify(Menu menu,
                              Menu_item menu_item)
{
    gen_array_t workspace_list;
    int  workspace_list_length = 0;

    workspace_list = gen_array_make(0);
    pips_get_workspace_list(workspace_list);
    workspace_list_length = gen_array_nitems(workspace_list);

    if (workspace_list_length == 0) {
	prompt_user("No workspace available in this directory.");
    }
    else {
	disable_workspace_delete_or_open();
	disable_workspace_create_or_open();
	disable_workspace_close();
	disable_change_directory();
	
	schoose("Select the Workspace to Delete", 
		workspace_list,
		/* Choix initial sur le workspace courant si
		   possible : */
		db_get_current_workspace_name(),
		end_delete_workspace_notify,
		cancel_delete_workspace_notify);
    }
    gen_array_full_free(workspace_list);
}


void
start_create_workspace_notify(Menu menu,
                            Menu_item menu_item)
{
   disable_workspace_delete_or_open();
   disable_workspace_create_or_open();
   disable_change_directory();

   if (db_get_current_workspace_name() != NULL)
      /* There is an open workspace: close it first: */
      if (!close_workspace_notify((Menu) NULL, (Menu_item) NULL)) {
         /* If it fails: */
         cancel_create_workspace_notify((Panel_item) NULL, (Event*)NULL);
         return;
      }

   start_query("Create Workspace", 
               "Enter workspace name: ", 
               "CreateWorkspace",
               continue_create_workspace_notify,
               /* Pas la peine de faire quelque chose si on appuie
                  sur cancel : */
               cancel_create_workspace_notify);
}


void
cancel_create_workspace_notify(Panel_item item,
                             Event * event)
{
   /* Re'tablit le droit d'ouvrir ou de cre'er un autre worspace : */
   enable_workspace_create_or_open();
   enable_workspace_delete_or_open();
   enable_change_directory();
   cancel_query_notify(item, event);
   show_workspace();
}


success
continue_create_workspace_notify(char * name)
{
   gen_array_t fortran_list;
   int fortran_list_length = 0;

   /* Is the name a valid workspace name? */
   if (! workspace_name_p(name)) {
      user_prompt_not_a_valid_workspace_name(name);
   }
   else 
   {
       fortran_list = gen_array_make(0);
       pips_get_fortran_list(fortran_list);
       fortran_list_length = gen_array_nitems(fortran_list);

      if (fortran_list_length == 0) {
         prompt_user("No Fortran files in this directory");
      }
      else {
         /* Code added to confirm for a database destruction before
            opening a database with the same name.
            RK 18/05/1993. */
         if (workspace_exists_p(name))
         {
            int result;
            /* Send to emacs if we are in the emacs mode: */
            if (wpips_emacs_mode) 
               send_notice_prompt_to_emacs("The database",
                                           name,
                                           "already exists!",
                                           "Do you really want to remove it?",
                                           NULL);
            result = notice_prompt(xv_find(main_frame, WINDOW, 0),
                                   NULL,
                                   NOTICE_MESSAGE_STRINGS,
                                   "The database", name, "already exists!",
                                   "Do you really want to remove it?",
                                   NULL,
                                   NOTICE_BUTTON_YES,  
				   "Yes, remove the database",
                                   NOTICE_BUTTON_NO,   "No, cancel",
                                   NULL);
            if (result == NOTICE_NO)
               goto continue_create_workspace_notify_failed;
         }

         disable_workspace_create_or_open();

         /* To avoid passing the name through mchoose(): */
         workspace_name_to_create = name;
         
         if (fortran_list_length == 1) {
            /* Only one Fortran program: use it without user
               confirmation. */
      user_log("There is only one Fortran program in the current directory.\n"
	       "\tCreating the workspace \"%s\" from the file \"%s\"...\n",
                     name, 
		     gen_array_item(fortran_list, 0));
            end_create_workspace_notify(fortran_list);
         }
         else {
            mchoose("Create Workspace", 
                    fortran_list, 
                    end_create_workspace_notify,
                    (void (*)(void)) cancel_create_workspace_notify);
         }
         /* Memory leak if mchoose exit... */
	 gen_array_full_free(fortran_list);
         
         return(TRUE);
      }
   }

   /* If it failed, cancel the creation: */
  continue_create_workspace_notify_failed:
   cancel_create_workspace_notify((Panel_item)NULL, (Event*)NULL);
   
   return FALSE;
}


void
user_prompt_not_a_valid_workspace_name(char * workspace_name)
{
   prompt_user("The name \"%s\" is not a valid workspace name!\n",
               workspace_name);
   enable_workspace_create_or_open();
   disable_workspace_close();      
   enable_change_directory();
}


void
end_create_workspace_notify(gen_array_t files)
{
    /* If the user click quickly on OK, be sure
       end_create_workspace_notify() is not reentrant by verifying
       something as not been opened already: */
    if (db_get_current_workspace_name() == NULL) {
	/* Is the name a valid workspace name? */
	if (workspace_name_p(workspace_name_to_create)) {
	    if (db_create_workspace(workspace_name_to_create)) {
		/* The create workspace has been successful: */
		/* open_log_file(); */
		display_memory_usage();
         
		if (create_workspace(files)) {
		    /* The processing of user files has been successful: */
		    enable_workspace_close();

		    show_workspace();
		    select_a_module_by_default();
		    enable_module_selection();
		    disable_change_directory();
            
		    enable_workspace_create_or_open();
		    enable_workspace_delete_or_open();

		    /* Tell Emacs the new module list: */
		    send_the_names_of_the_available_modules_to_emacs();

		    display_memory_usage();
         
		    return;
		}
		else
		    /* close_log_file(); */
		    ;
	    }
	}
	else
	    user_prompt_not_a_valid_workspace_name(workspace_name_to_create);

	/* The creation failed: */
	enable_change_directory();
	enable_workspace_create_or_open();
	enable_workspace_delete_or_open();
   
	display_memory_usage();
    }
}


void
end_open_workspace_notify(string name)
{
   schoose_close();

   if ( open_workspace(name) ) {
       /* open_log_file(); */
      enable_workspace_close();
      show_workspace();
      select_a_module_by_default();
      enable_module_selection();
      disable_change_directory();
      /* Tell Emacs the new module list: */
      send_the_names_of_the_available_modules_to_emacs();
   }

   enable_workspace_create_or_open();

   display_memory_usage();
}


void
cancel_open_workspace_notify()
{
   enable_workspace_create_or_open();
   enable_change_directory();
   show_workspace();
}


void
open_workspace_notify(Menu menu,
                      Menu_item menu_item)
{
    gen_array_t workspace_list;
    int  workspace_list_length = 0;

    workspace_list = gen_array_make(0);
    pips_get_workspace_list(workspace_list);
    workspace_list_length = gen_array_nitems(workspace_list);
    
    if (workspace_list_length == 0) {
	prompt_user("No workspace available in this directory.");
    }
    else if (workspace_list_length == 1) {
	/* There is only workspace: open it without asking confirmation
	   to the user: */
	user_log("There is only one workspace in the current directory.\n"
		 "\tOpening the workspace \"%s\"...\n",
		 gen_array_item(workspace_list, 0)); 
	end_open_workspace_notify(gen_array_item(workspace_list, 0));
    }
    else {
	disable_workspace_create_or_open();
	
	schoose("Select Workspace", 
		workspace_list,
		/* Choix initial sur le workspace courant si
		   possible : */
		db_get_current_workspace_name(),
		end_open_workspace_notify,
		cancel_open_workspace_notify);
    }
    gen_array_full_free(workspace_list);
}


success
close_workspace_notify(Menu menu,
                       Menu_item menu_item)
{
   success return_value;
   
   return_value = close_workspace();
   debug(1, "close_workspace_notify", "return_value = %d\n", return_value);

   if (return_value ) {
      /* The close has been successful: */
       /* close_log_file(); */
      initialize_wpips_hpfc_hack_for_fabien_and_from_fabien();

      edit_close_notify(menu, menu_item);

      enable_workspace_create_or_open();
      disable_workspace_close();
   
      /* It is the only place to enable a directory change, after a close
         workspace: */
      enable_change_directory();

      disable_module_selection();
      /* Tell Emacs the new module list, that is nothing in fact: */
      send_the_names_of_the_available_modules_to_emacs();
   }
  
   hide_window(schoose_frame);
   show_workspace();
   show_module();
   display_memory_usage();

   return return_value;
}


/* To be used with schoose_create_abbrev_menu_with_text from the main
   panel: */
void
open_or_create_workspace(char * workspace_name_original)
{
   int i;
   gen_array_t workspace_list;
   int workspace_list_length = 0;
   char workspace_name[SMALL_BUFFER_LENGTH];

   /* If close_workspace_notify() is called below, show_workspace() will
      set the name to "(* none *)" in the panel and
      workspace_name_original is directly a pointer to it ! */
   (void) strncpy(workspace_name,
                  workspace_name_original,
                  sizeof(workspace_name) - 1);

   if (! workspace_name_p(workspace_name)) {
      /* Prompt the warning and restore the menu enable state: */
      user_prompt_not_a_valid_workspace_name(workspace_name);
      show_workspace();
      return;
   }
      
   if (db_get_current_workspace_name() != NULL)
      /* There is an open workspace: close it first: */
      if (!close_workspace_notify((Menu) NULL, (Menu_item) NULL))
         return;

   /* To choose between open or create, look for the an existing
      workspace with the same name: */
   workspace_list = gen_array_make(0);
   pips_get_workspace_list(workspace_list);
   workspace_list_length = gen_array_nitems(workspace_list);

   for(i = 0; i < workspace_list_length; i++) {
       string name = gen_array_item(workspace_list, i);
       if (strcmp(workspace_name, name) == 0) {
	   /* OK, the workspace exists, open it: */
	   end_open_workspace_notify(workspace_name);
	   return;
       }
   }
   /* The workspace does not exist, create it: */
   disable_change_directory();
   gen_array_full_free(workspace_list);
   (void) continue_create_workspace_notify(workspace_name);
}


/* To use with schoose_create_abbrev_menu_with_text: */
Menu
generate_workspace_menu()
{
   Menu menu;
   Menu_item delete_menu_item;
   int i;
   gen_array_t workspace_list;
   int  workspace_list_length = 0;

   workspace_list = gen_array_make(0);
   pips_get_workspace_list(workspace_list);
   workspace_list_length = gen_array_nitems(workspace_list);

   menu = xv_create(XV_NULL, MENU,
                    MENU_TITLE_ITEM, " Select in the workspace list: ",
                    NULL);

   /* Replace the Select Workspace menu from the status window with
      the following items: */
   xv_set(menu, MENU_APPEND_ITEM,
          xv_create(XV_NULL, MENUITEM,
                    MENU_STRING,
                    "   Create Workspace...",
                    MENU_NOTIFY_PROC, start_create_workspace_notify,
                    MENU_RELEASE,
                    /* Always active since
                       start_create_workspace_notify() does the close
                       if necessary: */
                    MENU_INACTIVE, wpips_create_workspace_menu_inactive,
                    NULL),
          NULL);
   xv_set(menu, MENU_APPEND_ITEM,
          xv_create(XV_NULL, MENUITEM,
                    MENU_STRING,
                    "   Close Workspace",
                    MENU_NOTIFY_PROC, close_workspace_notify,
                    MENU_RELEASE,
                    /* On can close only if there is something open: */
                    MENU_INACTIVE, wpips_close_workspace_menu_inactive,
                    NULL),
          NULL);
   delete_menu_item = xv_create(XV_NULL, MENUITEM,
                                MENU_STRING,
                                "   Delete Workspace...",
                                MENU_NOTIFY_PROC, start_delete_workspace_notify,
                                MENU_RELEASE,
                                MENU_INACTIVE, wpips_delete_workspace_menu_inactive,
                                NULL);
   
   xv_set(menu, MENU_APPEND_ITEM,
          delete_menu_item,
          NULL);

   /* Now complete with the list of the workspaces: */
   if (workspace_list_length == 0) {
      xv_set(menu, MENU_APPEND_ITEM,
             xv_create(XV_NULL, MENUITEM,
                       MENU_STRING,
                       "* No workspace available in this directory *",
                       MENU_RELEASE,
                       MENU_INACTIVE, TRUE,
                       NULL),
             NULL);
      /* Well, since there is no workspace, there is nothing to
         delete... */
      xv_set(delete_menu_item,
             MENU_INACTIVE, TRUE,
             NULL);
   }
   else {
       for(i = 0; i < workspace_list_length; i++) 
       {
	   string name = gen_array_item(workspace_list, i);
	   xv_set(menu, MENU_APPEND_ITEM,
		  xv_create(XV_NULL, MENUITEM,
			    MENU_STRING, strdup(name),
			    MENU_RELEASE,
			    /* The strdup'ed string will also be
			       freed when the menu is discarded: */
			    MENU_RELEASE_IMAGE,
			    MENU_INACTIVE, wpips_open_workspace_menu_inactive,
			    NULL),
		  NULL);
       }
   }

   gen_array_full_free(workspace_list);
   
   return menu;
}


void
end_select_module_notify(string name)
{
   gen_array_t module_list = db_get_module_list();
   int module_list_length = gen_array_nitems(module_list);

   if (module_list_length == 0)
   {
      prompt_user("No module available in this workspace.");
   }
   else 
   {
      bool module_found = FALSE;
      int i;
      
      for(i = 0; i < module_list_length; i++) {
	  string mn = gen_array_item(module_list, i);
         if (strcmp(name, mn) == 0) {
            module_found = TRUE;
            break;
         }
      }
      if (module_found)
         lazy_open_module(name);
      else
         prompt_user("The module \"%s\" does not exist in this workspace.",
                     name);
   }
   
   show_module();
   display_memory_usage();
   gen_array_full_free(module_list);
}

void cancel_select_module_notify()
{
}

void
select_module_notify(Menu menu,
                     Menu_item menu_item)
{
   gen_array_t module_list = db_get_module_list();
   int  module_list_length = gen_array_nitems(module_list);

   if (module_list_length == 0)
   {
      /* If there is no module... RK, 23/1/1993. */
      prompt_user("No module available in this workspace.");
   }
   else
      schoose("Select Module", 
              module_list,
              /* Affiche comme choix courant le module
                 courant (c'est utile si on ferme la fen�tre
                 module entre temps) : */
              db_get_current_module_name(),
              end_select_module_notify,
              cancel_select_module_notify);

   gen_array_full_free(module_list);
}


/* To use with schoose_create_abbrev_menu_with_text: */
Menu
generate_module_menu()
{
   Menu menu;
   int i;
   menu = xv_create(XV_NULL, MENU,
                    MENU_TITLE_ITEM, " Select in the module list: ",
                    NULL);

   if (db_get_current_workspace_name() == NULL) {
      xv_set(menu, MENU_APPEND_ITEM,
             xv_create(XV_NULL, MENUITEM,
                       MENU_STRING, "* No workspace yet! *",
                       MENU_RELEASE,
                       MENU_INACTIVE, TRUE,
                       NULL),
             NULL);
   }
   else {
       gen_array_t module_list = db_get_module_list();
       int  module_list_length = gen_array_nitems(module_list);
       
       if (module_list_length == 0) {
	   xv_set(menu, MENU_APPEND_ITEM,
		  xv_create(XV_NULL, MENUITEM,
			    MENU_STRING,
                          "* No module available in this workspace *",
			    MENU_RELEASE,
			    MENU_INACTIVE, TRUE,
			    NULL),
                NULL);
       }
       else if (module_list_length > WPIPS_MAX_MODULE_MENU_SIZE) {
	   xv_set(menu,
		  MENU_TITLE_ITEM,
		  " Too many modules. Click on the Module Chooser: ",
		  NULL);
	   xv_set(menu, MENU_APPEND_ITEM,
		  xv_create(XV_NULL, MENUITEM,
			    MENU_STRING, "Module Chooser",
			    MENU_NOTIFY_PROC, select_module_notify,
			    MENU_RELEASE,
			    NULL),
		  NULL);
	   user_warning("generate_a_directory_menu",
			"Too many modules in this workspace. "
			"Click on the Module Chooser\n");
       }
       else {
	   for(i = 0; i < module_list_length; i++) {
	       string mn = gen_array_item(module_list, i);
	       xv_set(menu, MENU_APPEND_ITEM,
		      xv_create(XV_NULL, MENUITEM,
				MENU_STRING, strdup(mn),
				MENU_RELEASE,
				/* The strdup'ed string will also be
				   freed when the menu is discarded: */
				MENU_RELEASE_IMAGE,
				NULL),
		      NULL);
	   }
       }
       gen_array_full_free(module_list);
   }
   
   return menu;
}


void
create_select_menu()
{
   Menu menu, pmenu;

   create_pgm = xv_create(XV_NULL, MENUITEM, 
                          MENU_STRING, "Create",
                          MENU_NOTIFY_PROC, start_create_workspace_notify,
                          MENU_RELEASE,
                          NULL);

   open_pgm = xv_create(XV_NULL, MENUITEM, 
                        MENU_STRING, "Open",
                        MENU_NOTIFY_PROC, open_workspace_notify,
                        MENU_RELEASE,
                        NULL);

   close_pgm = xv_create(XV_NULL, MENUITEM, 
                         MENU_STRING, "Close",
                         MENU_NOTIFY_PROC, close_workspace_notify,
                         MENU_INACTIVE, TRUE,
                         MENU_RELEASE,
                         NULL);

   module_item = xv_create(XV_NULL, MENUITEM, 
                           MENU_STRING, "Module",
                           MENU_NOTIFY_PROC, select_module_notify,
                           MENU_INACTIVE, TRUE,
                           MENU_RELEASE,
                           NULL);

   /* Exchange of the order of create_pgm & open_pgm on the screen
      for ergonomic reasons. :-) RK, 19/02/1993. */
   pmenu = 
      xv_create(XV_NULL, MENU_COMMAND_MENU, 
                MENU_TITLE_ITEM, "Deal with workspaces ",
                MENU_GEN_PIN_WINDOW, main_frame, "Workspace Menu",
                MENU_APPEND_ITEM, open_pgm,
                MENU_APPEND_ITEM, create_pgm,
                MENU_APPEND_ITEM, close_pgm,
                NULL);

   /* Exchange of the order of start_directory_notify &
      module_item on the screen for ergonomic reasons.
      :-) RK, 19/02/1993. */
   directory_menu_item = xv_create(XV_NULL, MENUITEM,
                                   MENU_ACTION_ITEM,
                                   "Directory",
                                   start_directory_notify,
                                   NULL);
   
   menu = 
      xv_create(XV_NULL, MENU_COMMAND_MENU, 
                MENU_TITLE_ITEM, "Selecting workspace & Fortran module ",
                MENU_GEN_PIN_WINDOW, main_frame, "Selection Menu",
                MENU_APPEND_ITEM, module_item,
                MENU_PULLRIGHT_ITEM, "Workspace", pmenu,
                MENU_APPEND_ITEM, directory_menu_item,
                NULL);

   /* Now all the functionality of the select menu are on the status
      panel but I could reinstall the select menu later. Since I do
      not want do change the code, put the select menu in a dummy
      spare frame: */
   /*
      (void) xv_create(main_panel, PANEL_BUTTON,
                    PANEL_LABEL_STRING, "Select",
                    PANEL_ITEM_MENU, menu,
                    NULL);
                    */
   {
      Frame select_button_frame;
      Panel select_button_panel;
      
      select_button_frame = xv_create(main_frame, FRAME, 
                                      XV_SHOW, FALSE,
                                      NULL);
      select_button_panel = xv_create(select_button_frame, PANEL,
                                      NULL);
      
      (void) xv_create(select_button_panel, PANEL_BUTTON,
                       PANEL_LABEL_STRING, "Select",
                       PANEL_ITEM_MENU, menu,
                       NULL);
   }
}
