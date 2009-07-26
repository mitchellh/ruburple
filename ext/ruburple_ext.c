// Ruburple - a ruby wrapper for libpurple
// Copyright (C) 2007 Martin Kihlgren <zond at troja dot ath dot cx>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <libpurple/account.h>
#include <libpurple/conversation.h>
#include <libpurple/core.h>
#include <libpurple/debug.h>
#include <libpurple/cipher.h>
#include <libpurple/eventloop.h>
#include <libpurple/ft.h>
#include <libpurple/log.h>
#include <libpurple/notify.h>
#include <libpurple/prefs.h>
#include <libpurple/prpl.h>
#include <libpurple/pounce.h>
#include <libpurple/request.h>
#include <libpurple/savedstatuses.h>
#include <libpurple/sound.h>
#include <libpurple/status.h>
#include <libpurple/util.h>
#include <libpurple/whiteboard.h>

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ruby.h>
#include <stdarg.h>

/*
 * The ruby classes.
 */
static VALUE rb_ruburple;
static VALUE rb_ruburple_plugin;
static VALUE rb_ruburple_protocol;
static VALUE rb_ruburple_protocol_info;
static VALUE rb_ruburple_protocol_account;
static VALUE rb_ruburple_protocol_account_transfer;
static VALUE rb_ruburple_protocol_account_conversation;
static VALUE rb_ruburple_protocol_account_chat;
static VALUE rb_ruburple_protocol_account_savedstatus;
static VALUE rb_ruburple_protocol_account_status;
static VALUE rb_ruburple_protocol_connection;
static VALUE rb_ruburple_blist;
static VALUE rb_ruburple_blist_buddy_icon;
static VALUE rb_ruburple_blist_buddy;
static VALUE rb_ruburple_blist_node;
static VALUE rb_ruburple_blist_contact;
static VALUE rb_ruburple_blist_chat;
static VALUE rb_ruburple_blist_group;
static VALUE rb_ruburple_handle;
static VALUE rb_ruburple_cipher;
static VALUE rb_ruburple_log;
static VALUE rb_ruburple_xmlnode;
static VALUE rb_ruburple_userinfo;
static VALUE rb_ruburple_storedimage;
static VALUE rb_ruburple_subscription;
static VALUE rb_ruburple_pointer;

/*
 * A few globals.
 */
static GMainLoop *main_loop;
static VALUE call_method_id;
static VALUE idle_handler_method_id;
static int purple_event_queue[2];
static gboolean glib_sleeping;
static VALUE gc_save_set;
static VALUE add_method_id;
static VALUE delete_method_id;

/*
 * The UI name for libpurple.
 */
#define UI_ID "ruburple"

/**
 * The following eventloop functions are used in both pidgin and purple-text. If your
 * application uses glib mainloop, you can safely use this verbatim.
 */
#define PURPLE_GLIB_READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define PURPLE_GLIB_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

/* util macros */
#define FATAL(s, ...) fprintf(stderr, "FATAL: "); fprintf(stderr, s, ## __VA_ARGS__); fprintf(stderr, "\n"); exit(1)
#define RETURN_QNIL_IF_NULL(pointer) if (pointer == NULL) return Qnil

/*
 * Conversions from c to ruby.
 */
#define BOOL(gbool) (gbool == FALSE ? Qfalse : Qtrue)
#define RB_RUBURPLE_PROTOCOL_ACCOUNT(purple_account_pointer) Data_Wrap_Struct(rb_ruburple_protocol_account, NULL, NULL, purple_account_pointer)
#define RB_RUBURPLE_PROTOCOL_ACCOUNT_gc(purple_account_pointer) Data_Wrap_Struct(rb_ruburple_protocol_account, NULL, rb_ruburple_protocol_account_free, purple_account_pointer)
#define RB_RUBURPLE_PROTOCOL_ACCOUNT_CONVERSATION(purple_conversation_pointer) Data_Wrap_Struct(rb_ruburple_protocol_account_conversation, NULL, NULL, purple_conversation_pointer)
#define RB_RUBURPLE_PROTOCOL_ACCOUNT_TRANSFER(purple_xfer_pointer) Data_Wrap_Struct(rb_ruburple_protocol_account_transfer, NULL, NULL, purple_xfer_pointer)
#define RB_RUBURPLE_PROTOCOL_ACCOUNT_CHAT(purple_chat_pointer) Data_Wrap_Struct(rb_ruburple_protocol_account_chat, NULL, NULL, purple_chat_pointer)
#define RB_RUBURPLE_PROTOCOL_CONNECTION(purple_connection_pointer) Data_Wrap_Struct(rb_ruburple_protocol_connection, NULL, NULL, purple_connection_pointer)
#define RB_RUBURPLE_PROTOCOL_ACCOUNT_SAVEDSTATUS(purple_savedstatus_pointer) Data_Wrap_Struct(rb_ruburple_protocol_account_savedstatus, NULL, NULL, purple_savedstatus_pointer)
#define RB_RUBURPLE_PROTOCOL(purple_plugin_pointer) Data_Wrap_Struct(rb_ruburple_protocol, NULL, NULL, purple_plugin_pointer)
#define RB_RUBURPLE_PLUGIN(purple_plugin_pointer) Data_Wrap_Struct(rb_ruburple_plugin, NULL, NULL, purple_plugin_pointer)
#define RB_RUBURPLE_PROTOCOL_INFO(purple_plugin_info_pointer) Data_Wrap_Struct(rb_ruburple_protocol_info, NULL, NULL, purple_plugin_info_pointer)
#define RB_RUBURPLE_PROTOCOL_ACCOUNT_STATUS(purple_status_pointer) Data_Wrap_Struct(rb_ruburple_protocol_account_status, NULL, NULL, purple_status_pointer)
#define RB_RUBURPLE_BLIST(purple_blist_pointer) Data_Wrap_Struct(rb_ruburple_blist, NULL, NULL, purple_blist_pointer)
#define RB_RUBURPLE_BLIST_BUDDY(purple_buddy_pointer) Data_Wrap_Struct(rb_ruburple_blist_buddy, NULL, NULL, purple_buddy_pointer)
#define RB_RUBURPLE_BLIST_NODE(purple_blist_node_pointer) Data_Wrap_Struct(rb_ruburple_blist_node, NULL, NULL, purple_blist_node_pointer)
#define RB_RUBURPLE_BLIST_GROUP(purple_blist_group_pointer) Data_Wrap_Struct(rb_ruburple_blist_group, NULL, NULL, purple_blist_group_pointer)
#define RB_RUBURPLE_BLIST_BUDDY_ICON(purple_buddy_icon_pointer) Data_Wrap_Struct(rb_ruburple_blist_buddy_icon, NULL, NULL, purple_buddy_icon_pointer)
#define RB_RUBURPLE_HANDLE(void_pointer) Data_Wrap_Struct(rb_ruburple_handle, NULL, NULL, void_pointer)
#define RB_RUBURPLE_CIPHER(purple_cipher_pointer) Data_Wrap_Struct(rb_ruburple_cipher, NULL, NULL, purple_cipher_pointer)
#define RB_RUBURPLE_LOG(purple_log_pointer) Data_Wrap_Struct(rb_ruburple_log, NULL, NULL, purple_log_pointer)
#define RB_RUBURPLE_XMLNODE(xmlnode_pointer) Data_Wrap_Struct(rb_ruburple_xmlnode, NULL, NULL, xmlnode_pointer)
#define RB_RUBURPLE_USERINFO(purple_notify_user_info_pointer) Data_Wrap_Struct(rb_ruburple_userinfo, NULL, NULL, purple_notify_user_info_pointer)
#define RB_RUBURPLE_STOREDIMAGE(purple_stored_image_pointer) Data_Wrap_Struct(rb_ruburple_storedimage, NULL, NULL, purple_stored_image_pointer)
#define RB_RUBURPLE_SUBSCRIPTION_gc(ruburple_signal_handler_pointer) Data_Wrap_Struct(rb_ruburple_subscription, ruburple_signal_handler_mark, ruburple_signal_handler_free, ruburple_signal_handler_pointer)
#define RB_RUBURPLE_POINTER(gpointer) Data_Wrap_Struct(rb_ruburple_pointer, NULL, NULL, gpointer)

/*
 * Conversions from ruby to c.
 */
#define GBOOL(bool) ((bool == Qnil || bool == Qfalse) ? FALSE : TRUE)
#define PURPLE_ACCOUNT(rb_ruburple_protocol_account,purple_account_pointer) Data_Get_Struct(rb_ruburple_protocol_account, PurpleAccount, purple_account_pointer)
#define PURPLE_PLUGIN(rb_ruburple_protocol,purple_plugin_pointer) Data_Get_Struct(rb_ruburple_protocol, PurplePlugin, purple_plugin_pointer)
#define PURPLE_PLUGIN_INFO(rb_ruburple_protocol_info,purple_plugin_info_pointer) Data_Get_Struct(rb_ruburple_protocol_info, PurplePluginInfo, purple_plugin_info_pointer)
#define PURPLE_SAVEDSTATUS(rb_ruburple_protocol_account_savedstatus,purple_saved_status_pointer) Data_Get_Struct(rb_ruburple_protocol_account_savedstatus, PurpleSavedStatus, purple_saved_status_pointer)
#define RUBURPLE_SIGNAL_HANDLER(rb_ruburple_subscription,ruburple_signal_handler_pointer) Data_Get_Struct(rb_ruburple_subscription, RuburpleSignalHandler, ruburple_signal_handler_pointer)
#define PURPLE_CONNECTION(rb_ruburple_protocol_connection,purple_connection_pointer) Data_Get_Struct(rb_ruburple_protocol_connection, PurpleConnection, purple_connection_pointer)
#define PURPLE_BLIST(rb_ruburple_blist, purple_blist_pointer) Data_Get_Struct(rb_ruburple_blist, PurpleBuddyList, purple_blist)
#define PURPLE_BUDDY(rb_ruburple_blist_buddy,purple_buddy_pointer) Data_Get_Struct(rb_ruburple_blist_buddy, PurpleBuddy, purple_buddy_pointer)
#define PURPLE_GROUP(rb_ruburple_blist_group,purple_group_pointer) Data_Get_Struct(rb_ruburple_blist_group, PurpleGroup, purple_group_pointer)
#define PURPLE_BUDDY_ICON(rb_ruburple_blist_buddy_icon,purple_buddy_icon_pointer) Data_Get_Struct(rb_ruburple_blist_buddy_icon, PurpleBuddyIcon, purple_buddy_icon_pointer)
#define PURPLE_STATUS(rb_ruburple_protocol_account_status,purple_status_pointer) Data_Get_Struct(rb_ruburple_protocol_account_status, PurpleStatus, purple_status_pointer)
#define GPOINTER(rb_ruburple_pointer,gpointer) Data_Get_Struct(rb_ruburple_pointer, void, gpointer)

/*
 * The libpurple signal handler for Ruburple.
 */
typedef struct
{
  gpointer handle;
  VALUE callable;
  char *signal;
} RuburpleSignalHandler;

static RuburpleSignalHandler*
ruburple_signal_handler_new(gpointer handle, char *signal_name, VALUE callable)
{
  RuburpleSignalHandler *handler = g_new(RuburpleSignalHandler, 1);
  handler->handle = handle;
  handler->signal = strdup(signal_name);
  handler->callable = callable;
  return handler;
}

static void
ruburple_signal_handler_free(RuburpleSignalHandler *handler)
{
  g_free(handler->signal);
  g_free(handler);
}

static void
ruburple_signal_handler_mark(RuburpleSignalHandler *handler)
{
  rb_gc_mark(handler->callable);
}

/*
 * A definition of an incoming event for ruby.
 */
typedef struct
{
  VALUE receiver;
  VALUE method;
  int argc;
  VALUE *argv;
  GMutex *lock;
  GCond *flag;
} IncomingRuburpleEvent;

/*
 * Asking the glib main loop to do stuff for us.
 */

/* A shorthand for a generic function */
typedef gpointer (*RuburpleCallFunc)(gpointer);

/* A definition of something we want run once */
typedef struct
{
  RuburpleCallFunc function;
  gpointer data;
  gpointer result;
  GCond *done;
  GMutex *lock;
  gboolean run;
} RuburpleSingleShotCall;

/* Creator of single shot calls */
static RuburpleSingleShotCall*
ruburple_single_shot_call_new(RuburpleCallFunc function, gpointer data)
{
  RuburpleSingleShotCall *return_value = g_new(RuburpleSingleShotCall, 1);
  return_value->function = function;
  return_value->data = data;
  return_value->run = FALSE;
  return_value->done = g_cond_new();
  return_value->lock = g_mutex_new();
  return return_value;
}

/* Destructor of single shot calls */
static void
ruburple_single_shot_call_free(RuburpleSingleShotCall *call)
{
  g_mutex_free(call->lock);
  g_cond_free(call->done);
  g_free(call);
}

/* Call one of our single shots and free the call structure */
static gboolean
call_and_free_single_shot(gpointer data)
{
  RuburpleSingleShotCall *call = (RuburpleSingleShotCall *) data;
  call->function(call->data);
  ruburple_single_shot_call_free(call);
  return FALSE;
}

/* Call one of our single shots */
static gboolean
call_single_shot(gpointer data)
{
  RuburpleSingleShotCall *call = (RuburpleSingleShotCall *) data;

  g_mutex_lock(call->lock);

  call->result = call->function(call->data);
  call->run = TRUE;
  g_cond_broadcast(call->done);

  g_mutex_unlock(call->lock);

  return FALSE;
}

/* Ask glib to run something once */
static RuburpleSingleShotCall*
add_single_shot(RuburpleCallFunc function, gpointer data)
{
  RuburpleSingleShotCall *call = ruburple_single_shot_call_new(function, data);
  g_idle_add(call_single_shot, call);
  return call;
}

/*
 * Ask glib to run something for us once and return immediately and discard the result.
 *
 * If the glib main loop is sleeping due to us we will do it ourselves,
 * if it is NOT sleeping due to us we will register a single shot call.
 *
 */
static void
call_and_discard_result(RuburpleCallFunc function, gpointer data)
{
  if (glib_sleeping) {
    function(data);
  } else {
    RuburpleSingleShotCall *call = ruburple_single_shot_call_new(function, data);
    g_idle_add(call_and_free_single_shot, call);
  }
}

/* 
 * Ask glib to run something for us once and wait for the result to be ready.
 *
 * If the glib main loop is sleeping due to us we will do it ourselves, 
 * if it is NOT sleeping due to us we will register a single shot call.
 */
static gpointer
call_and_get_result(RuburpleCallFunc function, gpointer data)
{
  RuburpleSingleShotCall *call;
  gpointer return_value;

  if (glib_sleeping) {
    return_value = function(data);
  } else {
    call = add_single_shot(function, data);
    
    g_mutex_lock(call->lock);
    
    while (!call->run) {
      g_cond_wait(call->done, call->lock);
    }
    
    return_value = call->result;
    
    g_mutex_unlock(call->lock);
    
    ruburple_single_shot_call_free(call);
  }
  return return_value;
}

/*
 * The magical stuff taken more or less verbatim from nullclient.c
 */

typedef struct _PurpleGLibIOClosure {
	PurpleInputFunction function;
	guint result;
	gpointer data;
} PurpleGLibIOClosure;

static void purple_glib_io_destroy(gpointer data)
{
	g_free(data);
}

static gboolean purple_glib_io_invoke(GIOChannel *source, GIOCondition condition, gpointer data)
{
	PurpleGLibIOClosure *closure = data;
	PurpleInputCondition purple_cond = 0;

	if (condition & PURPLE_GLIB_READ_COND)
		purple_cond |= PURPLE_INPUT_READ;
	if (condition & PURPLE_GLIB_WRITE_COND)
		purple_cond |= PURPLE_INPUT_WRITE;

	closure->function(closure->data, g_io_channel_unix_get_fd(source),
			  purple_cond);

	return TRUE;
}

static guint glib_input_add(gint fd, PurpleInputCondition condition, PurpleInputFunction function,
							   gpointer data)
{
	PurpleGLibIOClosure *closure = g_new0(PurpleGLibIOClosure, 1);
	GIOChannel *channel;
	GIOCondition cond = 0;

	closure->function = function;
	closure->data = data;

	if (condition & PURPLE_INPUT_READ)
		cond |= PURPLE_GLIB_READ_COND;
	if (condition & PURPLE_INPUT_WRITE)
		cond |= PURPLE_GLIB_WRITE_COND;

	channel = g_io_channel_unix_new(fd);
	closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond,
					      purple_glib_io_invoke, closure, purple_glib_io_destroy);

	g_io_channel_unref(channel);
	return closure->result;
}

static PurpleEventLoopUiOps glib_eventloops = 
  {
    g_timeout_add,
    g_source_remove,
    glib_input_add,
    g_source_remove,
    NULL
  };
/*** End of the eventloop functions. ***/


/* this function blindly presses the first button of whatever dialogue box
   libpurple wants us to show! That's crazy!  */
static gpointer
ruburple_request_blindly_accept(const char *title, const char *primary, const char *secondary,
                                                    int default_action,
                                                    PurpleAccount *account, const char *who, PurpleConversation *conv,
                                                    void *user_data, size_t action_count, va_list actions)
{
  GCallback ok_cb;
  char *button_text;
	/* Get the first callback, skipping over the title */
  button_text = va_arg(actions, char *);
  printf("Automatically pressing: %s\nin dialogue%s\n%s\n", button_text, primary, secondary);
	ok_cb = va_arg(actions, GCallback);
  ((PurpleRequestActionCb)ok_cb)(user_data, default_action);
}

static gpointer
ruburple_notify_message(PurpleNotifyMsgType type, 
			const char *title,
			const char *primary, 
			const char *secondary)
{
  switch (type) {
  case PURPLE_NOTIFY_MSG_ERROR:
    printf("Error message: %s\n%s\n%s\n", title, primary, secondary);
    break;
  case PURPLE_NOTIFY_MSG_WARNING:
    printf("Warning message: %s\n%s\n%s\n", title, primary, secondary); 
    break;
 case PURPLE_NOTIFY_MSG_INFO:
    printf("Error message: %s\n%s\n%s\n", title, primary, secondary);
    break;
  default:
    printf("Unknown message: %s\n%s\n%s\n", title, primary, secondary);
    break;
  }
  fflush(NULL);
}

static PurpleNotifyUiOps notify_ops =
  {
    ruburple_notify_message,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
  };

static PurpleRequestUiOps request_ops =
  {
    NULL,
    NULL,
    ruburple_request_blindly_accept,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
  };


static gpointer
run_the_loop(gpointer data)
{
  main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(main_loop);
}

static VALUE
rb_ruburple_init(VALUE self)
{
  if (purple_get_core() == NULL) {
    /* We do not want any debugging for now to keep the noise to a minimum. */
    purple_debug_set_enabled(FALSE);

    /* Set the uiops for the eventloop. If your client is glib-based, you can safely
     * copy this verbatim. */
    purple_eventloop_set_ui_ops(&glib_eventloops);
    
    purple_notify_set_ui_ops(&notify_ops);

    purple_request_set_ui_ops(&request_ops);

    /* Now that all the essential stuff has been set, let's try to init the core. It's
     * necessary to provide a non-NULL name for the current ui to the core. This name
     * is used by stuff that depends on this ui, for example the ui-specific plugins. */
    if (!purple_core_init(UI_ID)) {
      /* Initializing the core failed. Terminate. */
      fprintf(stderr,
	      "libpurple initialization failed. Dumping core.\n"
	      "Please report this!\n");
      abort();
    }

    /* Create and load the buddylist. */
    purple_set_blist(purple_blist_new());
    purple_blist_load();

    /* Load the preferences. */
    purple_prefs_load();

    /* Load the pounces. */
    purple_pounces_load();

    g_thread_create(run_the_loop, NULL, FALSE, NULL);
  }
  return Qnil;
}

static VALUE
rb_ruburple_quit(VALUE self) 
{
  if (purple_get_core() != NULL) {
    g_main_loop_quit(main_loop);
    purple_core_quit();
  }
  return Qnil;
}

static gpointer
ruburple_set_debug(gpointer data)
{
  purple_debug_set_enabled(GBOOL(* ((VALUE *) data)));
  return NULL;
}

static VALUE
rb_ruburple_set_debug(VALUE self, VALUE enabled)
{
  gpointer return_value = call_and_get_result(ruburple_set_debug, (gpointer) (&enabled));
  return Qnil;
}

static VALUE
rb_protocol_get_id(VALUE self)
{
  PurplePlugin *plugin;
  const gchar *id;
  PURPLE_PLUGIN(self, plugin);
  id = purple_plugin_get_id(plugin);
  RETURN_QNIL_IF_NULL(id);
  return rb_str_new2(id);
}

static VALUE
rb_protocol_get_summary(VALUE self)
{
  PurplePlugin *plugin;
  const gchar *summary;
  PURPLE_PLUGIN(self, plugin);
  summary = purple_plugin_get_summary(plugin);
  RETURN_QNIL_IF_NULL(summary);
  return rb_str_new2(summary);
}

static VALUE
rb_protocol_get_version(VALUE self)
{
  PurplePlugin *plugin;
  const gchar *version;
  PURPLE_PLUGIN(self, plugin);
  version = purple_plugin_get_version(plugin);
  RETURN_QNIL_IF_NULL(version);
  return rb_str_new2(version);
}

static VALUE
rb_protocol_get_name(VALUE self)
{
  PurplePlugin *plugin;
  const gchar *name;
  PURPLE_PLUGIN(self, plugin);
  name = purple_plugin_get_name(plugin);
  RETURN_QNIL_IF_NULL(name);
  return rb_str_new2(name);
}

static VALUE
rb_protocol_get_description(VALUE self)
{
  PurplePlugin *plugin;
  const gchar *description;
  PURPLE_PLUGIN(self, plugin);
  description = purple_plugin_get_description(plugin);
  RETURN_QNIL_IF_NULL(description);
  return rb_str_new2(description);
}

static VALUE
rb_protocol_get_author(VALUE self)
{
  PurplePlugin *plugin;
  const gchar *author;
  PURPLE_PLUGIN(self, plugin);
  author = purple_plugin_get_author(plugin);
  RETURN_QNIL_IF_NULL(author);
  return rb_str_new2(author);
}

static VALUE
rb_protocol_get_homepage(VALUE self)
{
  PurplePlugin *plugin;
  const gchar *homepage;
  PURPLE_PLUGIN(self, plugin);
  homepage = purple_plugin_get_homepage(plugin);
  RETURN_QNIL_IF_NULL(homepage);
  return rb_str_new2(homepage);
}

static gpointer
ruburple_get_protocols(gpointer data)
{
  return (gpointer) purple_plugins_get_protocols();
}

static gpointer
ruburple_get_protocol(gpointer data)
{
  return (gpointer) purple_plugins_find_with_name((char *) data);
}

static VALUE
rb_ruburple_get_protocols(VALUE self)
{
  GList *iter;
  VALUE return_value = rb_ary_new();

  for (iter = (GList *) call_and_get_result(ruburple_get_protocols, NULL); iter; iter = iter->next) {
    rb_ary_push(return_value, RB_RUBURPLE_PROTOCOL(iter->data));
  }

  return return_value;
}

static VALUE
rb_ruburple_get_protocol(VALUE self, VALUE name)
{
  PurplePlugin *protocol = (PurplePlugin *) call_and_get_result(ruburple_get_protocol, (gpointer) RSTRING(name)->ptr);
  RETURN_QNIL_IF_NULL(protocol);
  return RB_RUBURPLE_PROTOCOL(protocol);
}

static VALUE
rb_ruburple_has_protocol(VALUE self, VALUE name)
{
  GList *iter;

  for (iter = (GList *) call_and_get_result(ruburple_get_protocols, NULL); iter; iter = iter->next) {
    if (strcmp(purple_plugin_get_name((PurplePlugin *) iter->data), RSTRING(name)->ptr) == 0)
      return Qtrue;
  }

  return Qfalse;
}

static gpointer
ruburple_get_blist()
{
  return (gpointer)purple_get_blist();
}

static VALUE
rb_ruburple_get_blist(VALUE self)
{
  PurpleBuddyList *blist;
  blist = (PurpleBuddyList *) call_and_get_result(ruburple_get_blist, NULL);
  RETURN_QNIL_IF_NULL(blist);
  return RB_RUBURPLE_BLIST(blist);
}

//==Ruburple::Blist==
static gpointer
ruburple_blist_get_group_size(gpointer data)
{
  gpointer *args = (gpointer *) data;
  PurpleGroup *group = purple_find_group((char *)args[0]);
  * (gint *) args[1] = purple_blist_get_group_size(group, 1);
  return NULL;
}

static VALUE
rb_ruburple_blist_get_group_size(VALUE self, VALUE name)
{
  gpointer args[2];
  gint tmp;
  VALUE rval;
  args[0] = (gpointer) RSTRING(name)->ptr;
  args[1] = (gpointer) &tmp;
  call_and_get_result(ruburple_blist_get_group_size, args); 
  rval = rb_int_new(tmp);
  return rval;
}

//==Ruburple::BuddyList::Group==

static gpointer
ruburple_blist_group_new(gpointer data)
{
  return (gpointer) purple_group_new((char *) data);
}

static VALUE
rb_ruburple_blist_group_new(VALUE self, VALUE name)
{
  gpointer data;
  data = (gpointer) RSTRING(name)->ptr;
  PurpleGroup *group;
  group = (PurpleGroup *) call_and_get_result(ruburple_blist_group_new, data);
  RETURN_QNIL_IF_NULL(group);
  return RB_RUBURPLE_BLIST_GROUP(group);
}

static gpointer
ruburple_blist_group_get_name(gpointer data)
{
  return (gpointer) purple_group_get_name((PurpleGroup *) data);
}

static VALUE
rb_ruburple_blist_group_get_name(VALUE self)
{
  gpointer data;
  PURPLE_GROUP(self, data);

  char *rval;
  rval = (char *) call_and_get_result(ruburple_blist_group_get_name, data);
  RETURN_QNIL_IF_NULL(rval);
  return rb_str_new2(rval);
}

static gpointer
ruburple_blist_group_get_accounts(gpointer data)
{
  return (gpointer) purple_group_get_accounts((PurpleGroup *) data);
}

static VALUE
rb_ruburple_blist_group_get_accounts(VALUE self)
{
  gpointer data;
  PURPLE_GROUP(self, data);

  GList *iter;
  VALUE rval = rb_ary_new();
  
	for(iter = (GList *) call_and_get_result(ruburple_blist_group_get_accounts, data); iter; iter = iter->next) {
		rb_ary_push(rval, RB_RUBURPLE_PROTOCOL_ACCOUNT((PurpleAccount *) iter->data));
	}
  
  return rval;
}

//==Ruburple::Protocol==

static gpointer
ruburple_protocol_account_free(gpointer data)
{
  PurpleAccount *account = (PurpleAccount *) data;
  if (!purple_account_is_disconnected(account) && !account->disconnecting)
    purple_account_disconnect(account);
  purple_account_destroy(account);
  return NULL;
}

static void
rb_ruburple_protocol_account_free(PurpleAccount *account)
{
  call_and_get_result(ruburple_protocol_account_free, account);
}

static gpointer
ruburple_protocol_get_account(gpointer data)
{
  gpointer *args = (gpointer *) data;
  return (gpointer) purple_account_new((char *) args[0], (char *) args[1]);
}

static gpointer
ruburple_protocol_get_account_and_set_password(gpointer data)
{
  gpointer *args = (gpointer *) data;
  gpointer return_value;
  return_value = (gpointer) purple_account_new((char *) args[0], (char *) args[1]);
  purple_account_set_password((PurpleAccount *) return_value, (char *) args[2]);
  return return_value;
}

static VALUE
rb_ruburple_protocol_get_account(int argc, VALUE *argv, VALUE self)
{
  PurplePlugin *plugin;
  gpointer args[3];
  VALUE return_value;

  if (argc > 2)
    rb_raise(rb_eRuntimeError, "Ruburple::Protocol#get_account(USERNAME, PASSWORD = nil) takes at most 2 arguments");
  else if (argc < 1)
    rb_raise(rb_eRuntimeError, "Ruburple::Protocol#get_account(USERNAME, PASSWORD = nil) takes at least 1 argument");
  else if (argc > 1)
    Check_Type(argv[1], T_STRING);
  Check_Type(argv[0], T_STRING);

  PURPLE_PLUGIN(self, plugin);

  args[0] = (gpointer) RSTRING(argv[0])->ptr;
  args[1] = (gpointer) plugin->info->id;
  
  if (argc == 1)
    return_value = RB_RUBURPLE_PROTOCOL_ACCOUNT_gc((PurpleAccount *) call_and_get_result(ruburple_protocol_get_account, args));
  else if (argc == 2) {
    args[2] = (gpointer) RSTRING(argv[1])->ptr;
    return_value = RB_RUBURPLE_PROTOCOL_ACCOUNT_gc((PurpleAccount *) call_and_get_result(ruburple_protocol_get_account_and_set_password, args));
  }

  return return_value;
}

static gpointer
ruburple_protocol_account_set_password(gpointer data)
{
  gpointer *args = (gpointer *) data;
  purple_account_set_password((PurpleAccount *) args[0], (char *) args[1]);
  return NULL;
}

static VALUE
rb_ruburple_protocol_account_set_password(VALUE self, VALUE password)
{
  PurpleAccount *account;
  gpointer args[2];

  Check_Type(password, T_STRING);
  PURPLE_ACCOUNT(self, account);
  args[0] = (gpointer) account;
  args[1] = (gpointer) RSTRING(password)->ptr;

  call_and_get_result(ruburple_protocol_account_set_password, args);

  return Qnil;
}

static gpointer
ruburple_protocol_account_set_enabled(gpointer data)
{
  gpointer *args = (gpointer *) data;
  purple_account_set_enabled((PurpleAccount *) args[0], UI_ID, * (gboolean *) args[1]);
  return NULL;
}

static VALUE
rb_ruburple_protocol_account_set_enabled(VALUE self, VALUE enabled_value)
{
  PurpleAccount *account;
  gboolean enabled = GBOOL(enabled_value);
  gpointer args[2];

  PURPLE_ACCOUNT(self, account);
  args[0] = (gpointer) account;
  args[1] = (gpointer) &enabled;
  
  call_and_get_result(ruburple_protocol_account_set_enabled, args);

  return Qnil;
}

static VALUE
rb_ruburple_protocol_account_connect(VALUE self)
{
  return rb_ruburple_protocol_account_set_enabled(self, Qtrue);
}

static gpointer
ruburple_protocol_account_get_username(gpointer data)
{
  PurpleAccount *account = (PurpleAccount *) data;
  return (gpointer) purple_account_get_username(account);
}

static VALUE
rb_ruburple_protocol_account_get_username(VALUE self)
{
  PurpleAccount *account;
  char *rval;
  PURPLE_ACCOUNT(self, account);
  rval = (char *) call_and_get_result(ruburple_protocol_account_get_username, (gpointer) account);
  RETURN_QNIL_IF_NULL(rval);
  return rb_str_new2(rval);
}

static gpointer
ruburple_protocol_account_get_password(gpointer data)
{
  PurpleAccount *account = (PurpleAccount *) data;
  return (gpointer) purple_account_get_password(account);
}

static VALUE
rb_ruburple_protocol_account_get_password(VALUE self)
{
  PurpleAccount *account;
  char *rval;
  PURPLE_ACCOUNT(self, account);
  rval = (char *) call_and_get_result(ruburple_protocol_account_get_password, (gpointer) account);
  RETURN_QNIL_IF_NULL(rval);
  return rb_str_new2(rval);
}

static gpointer
ruburple_protocol_account_get_alias(gpointer data)
{
  PurpleAccount *account = (PurpleAccount *) data;
  return (gpointer) purple_account_get_alias(account);
}

static VALUE
rb_ruburple_protocol_account_get_alias(VALUE self)
{
  PurpleAccount *account;
  char *rval;
  PURPLE_ACCOUNT(self, account);
  rval = (char *) call_and_get_result(ruburple_protocol_account_get_alias, (gpointer) account);
  RETURN_QNIL_IF_NULL(rval);
  return rb_str_new2(rval);
}

static gpointer
ruburple_protocol_account_get_status(gpointer data)
{
  return (gpointer) purple_account_get_active_status((PurpleAccount *) data);
}

static VALUE
rb_ruburple_protocol_account_get_status(VALUE self)
{
  PurpleAccount *account;
  PurpleStatus *status;
  PURPLE_ACCOUNT(self, account);
  status = (PurpleStatus *) call_and_get_result(ruburple_protocol_account_get_status, (gpointer) account);
  RETURN_QNIL_IF_NULL(status);
  return RB_RUBURPLE_PROTOCOL_ACCOUNT_STATUS(status);
}

static gpointer
ruburple_protocol_account_set_savedstatus(gpointer data)
{
  gpointer *args = (gpointer *) data;
  purple_savedstatus_activate_for_account((PurpleSavedStatus *) args[0], (PurpleAccount *) args[1]);
  return NULL;
}

static VALUE
rb_ruburple_protocol_account_set_savedstatus(VALUE self, VALUE savedstatus_value)
{
  PurpleAccount *account;
  PurpleSavedStatus *savedstatus;
  gpointer args[2];

  PURPLE_ACCOUNT(self, account);
  PURPLE_SAVEDSTATUS(savedstatus_value, savedstatus);

  args[0] = (gpointer) savedstatus;
  args[1] = (gpointer) account;

  call_and_get_result(ruburple_protocol_account_set_savedstatus, args);

  return Qnil;
}

static gpointer
ruburple_protocol_account_savedstatus_alloc(gpointer data)
{
  return (gpointer) purple_savedstatus_new(NULL, PURPLE_STATUS_UNSET);
}

static VALUE
rb_ruburple_protocol_account_savedstatus_alloc(VALUE klass)
{
  PurpleSavedStatus *savedstatus = (PurpleSavedStatus *) call_and_get_result(ruburple_protocol_account_savedstatus_alloc, NULL);
  return Data_Wrap_Struct(klass, NULL, free, savedstatus);
}

static gpointer
ruburple_protocol_account_savedstatus_initialize(gpointer data)
{
  gpointer *args = (gpointer *) data;
  purple_savedstatus_set_type((PurpleSavedStatus *) args[0], NUM2INT(*( (VALUE *) args[1])));
}

static VALUE
rb_ruburple_protocol_account_savedstatus_initialize(VALUE self, VALUE status_constant)
{
  PurpleSavedStatus *savedstatus;
  gpointer args[2];

  Check_Type(status_constant, T_FIXNUM);
  PURPLE_SAVEDSTATUS(self, savedstatus);

  args[0] = (gpointer) savedstatus;
  args[1] = (gpointer) (&status_constant);

  call_and_get_result(ruburple_protocol_account_savedstatus_initialize, args);

  return Qnil;
}

static VALUE
get_ruby_instance_from_subtype(PurpleValue *value, gpointer data, int i, char *signal_name)
{
  switch (purple_value_get_subtype(value)) {
  case PURPLE_SUBTYPE_UNKNOWN:
    FATAL("argument %d of callback from signal %s is of PURPLE_TYPE_SUBTYPE:PURPLE_TYPE_UNKNOWN", 
	  i, 
	  signal_name);
    break;
  case PURPLE_SUBTYPE_ACCOUNT:
    return(RB_RUBURPLE_PROTOCOL_ACCOUNT((PurpleAccount *) data));
    break;
  case PURPLE_SUBTYPE_BLIST:
    return(RB_RUBURPLE_BLIST((PurpleBuddyList *) data));
    break;
  case PURPLE_SUBTYPE_BLIST_BUDDY:
    return(RB_RUBURPLE_BLIST_BUDDY((PurpleBuddy *) data));
    break;
  case PURPLE_SUBTYPE_BLIST_GROUP:
    return(RB_RUBURPLE_BLIST_GROUP((PurpleGroup *) data));
    break;
  case PURPLE_SUBTYPE_BLIST_CHAT:
    return(RB_RUBURPLE_PROTOCOL_ACCOUNT_CHAT((PurpleChat *) data));
    break;
  case PURPLE_SUBTYPE_BUDDY_ICON:
    return(RB_RUBURPLE_BLIST_BUDDY_ICON((PurpleBuddyIcon *) data));
    break;
  case PURPLE_SUBTYPE_CONNECTION:
    return(RB_RUBURPLE_PROTOCOL_CONNECTION((PurpleConnection *) data));
    break;
  case PURPLE_SUBTYPE_CONVERSATION:
    return(RB_RUBURPLE_PROTOCOL_ACCOUNT_CONVERSATION((PurpleConversation *) data));
    break;
  case PURPLE_SUBTYPE_PLUGIN:
    return(RB_RUBURPLE_PLUGIN((PurplePlugin *) data));
    break;
  case PURPLE_SUBTYPE_BLIST_NODE:
    return(RB_RUBURPLE_BLIST_NODE((PurpleBlistNode *) data));
    break;
  case PURPLE_SUBTYPE_CIPHER:
    return(RB_RUBURPLE_CIPHER((PurpleCipher *) data));
    break;
  case PURPLE_SUBTYPE_STATUS:
    return(RB_RUBURPLE_PROTOCOL_ACCOUNT_STATUS((PurpleStatus *) data));
    break;
  case PURPLE_SUBTYPE_LOG:
    return(RB_RUBURPLE_LOG((PurpleLog *) data));
    break;
  case PURPLE_SUBTYPE_XFER:
    return(RB_RUBURPLE_PROTOCOL_ACCOUNT_TRANSFER((PurpleXfer *) data));
    break;
  case PURPLE_SUBTYPE_SAVEDSTATUS:
    return(RB_RUBURPLE_PROTOCOL_ACCOUNT_SAVEDSTATUS((PurpleSavedStatus *) data));
    break;
  case PURPLE_SUBTYPE_XMLNODE:
    return(RB_RUBURPLE_XMLNODE((xmlnode *) data));
    break;
  case PURPLE_SUBTYPE_USERINFO:
    return(RB_RUBURPLE_USERINFO((PurpleNotifyUserInfo *) data));
    break;
  case PURPLE_SUBTYPE_STORED_IMAGE:
    return(RB_RUBURPLE_STOREDIMAGE((PurpleStoredImage *) data));
    break;
  default:
    FATAL("argument %d of callback from signal %s is of a completely unknown PURPLE_TYPE_SUBTYPE %d", 
	  i, 
	  signal_name, 
	  purple_value_get_subtype(value));
    break;
  }
}

static void
ruburple_callback(va_list args, void *data)
{
  RuburpleSignalHandler *handler = (RuburpleSignalHandler *) data;
  void *ret_val;
  int value_count;
  int i;
  PurpleValue *ret_value, **values;
  IncomingRuburpleEvent event;
  IncomingRuburpleEvent *event_pointer = &event;
  VALUE *ruby_args;
  gpointer next_arg;

  purple_signal_get_values(handler->handle, handler->signal,
			   &ret_value, &value_count, &values);
  
  ruby_args = g_new(VALUE, value_count);

  for (i = 0; i < value_count; i++) {
    if (purple_value_is_outgoing(values[i])) {
      switch (purple_value_get_type(values[i])) {
      case PURPLE_TYPE_UNKNOWN: /**< Unknown type.                     */
	FATAL("argument %d of callback from signal %s is of PURPLE_TYPE_UNKNOWN", 
	      i, 
	      handler->signal);
	break;
      case PURPLE_TYPE_SUBTYPE:    /**< Subtype.                          */
	if ((next_arg = (gpointer) va_arg(args, gpointer *)) == NULL) {
	  ruby_args[i] = Qnil;
	} else {
	  ruby_args[i] = get_ruby_instance_from_subtype(values[i], *( (gpointer *) next_arg), i, handler->signal);
	}
	break;
      case PURPLE_TYPE_CHAR:       /**< Character.                        */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = INT2NUM(*(char *) next_arg);
	break;
      case PURPLE_TYPE_UCHAR:      /**< Unsigned character.               */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = INT2NUM(*(unsigned char *) next_arg);
	break;
      case PURPLE_TYPE_BOOLEAN:    /**< Boolean.                          */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = (*(gboolean *) next_arg == TRUE ? Qtrue : Qfalse);
	break;
      case PURPLE_TYPE_SHORT:      /**< Short integer.                    */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = INT2NUM(*(short *) next_arg);
	break;
      case PURPLE_TYPE_USHORT:     /**< Unsigned short integer.           */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = INT2NUM(*(unsigned short *) next_arg);
	break;
      case PURPLE_TYPE_INT:        /**< Integer.                          */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = INT2NUM(*(int *) next_arg);
	break;
      case PURPLE_TYPE_UINT:       /**< Unsigned integer.                 */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = INT2NUM(*(unsigned int *) next_arg);
	break;	
      case PURPLE_TYPE_LONG:       /**< Long integer.                     */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = INT2NUM(*(long *) next_arg);
	break;
      case PURPLE_TYPE_ULONG:      /**< Unsigned long integer.            */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = INT2NUM(*(unsigned long *) next_arg);
	break;
      case PURPLE_TYPE_INT64:      /**< 64-bit integer.                   */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = INT2NUM(*(gint64 *) next_arg);
	break;	
      case PURPLE_TYPE_UINT64:     /**< 64-bit unsigned integer.          */
	if ((next_arg = va_arg(args, gpointer)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = INT2NUM(*(guint64 *) next_arg);
	break;
      case PURPLE_TYPE_STRING:     /**< String.                           */
	if ((next_arg = va_arg(args, gpointer *)) == NULL)
	  ruby_args[i] = Qnil;
	else
	  ruby_args[i] = rb_str_new2(*(char **) next_arg);
	break;	
      case PURPLE_TYPE_OBJECT:     /**< Object pointer.                   */
	FATAL("argument %d of callback from signal %s is of type PURPLE_TYPE_OBJECT which I dont know what the hell to do with",
	      i, "");
	break;	
      case PURPLE_TYPE_POINTER:    /**< Generic pointer.                  */
	FATAL("argument %d of callback from signal %s is of type PURPLE_TYPE_POINTER which I dont know what the hell to do with",
	      i, "");
	break;	
      case PURPLE_TYPE_ENUM:       /**< Enum.                             */
	FATAL("argument %d of callback from signal %s is of type PURPLE_TYPE_ENUM which I dont know what the hell to do with",
	      i, "");
	break;
      case PURPLE_TYPE_BOXED:      /**< Boxed pointer with specific type. */
	FATAL("argument %d of callback from signal %s is of type PURPLE_TYPE_BOXED which I dont know what the hell to do with",
	      i, "");
	break;
      default: 
	FATAL("argument %d of callback from signal %s is of a completely unknown type %d", 
	      i, 
	      handler->signal, 
	      purple_value_get_type(values[i]));
	break;
      }
    } else {
      switch (purple_value_get_type(values[i])) {
      case PURPLE_TYPE_UNKNOWN: /**< Unknown type.                     */
	FATAL("argument %d of callback from signal %s is of PURPLE_TYPE_UNKNOWN", 
	      i, 
	      handler->signal);
	break;
      case PURPLE_TYPE_SUBTYPE:    /**< Subtype.                          */
	if ((next_arg = va_arg(args, gpointer)) == NULL) {
	  ruby_args[i] = Qnil;
	} else {
	  ruby_args[i] = get_ruby_instance_from_subtype(values[i], next_arg, i, handler->signal);
	}
	break;
      default:
	switch (purple_value_get_type(values[i])) {
	case PURPLE_TYPE_BOOLEAN:    /**< Boolean.                          */
	  ruby_args[i] = va_arg(args, gboolean) == TRUE ? Qtrue : Qfalse;
	  break;
	case PURPLE_TYPE_INT:        /**< Integer.                          */
	  ruby_args[i] = INT2NUM(va_arg(args, int));
	  break;
	case PURPLE_TYPE_UINT:       /**< Unsigned integer.                 */
	  ruby_args[i] = INT2NUM(va_arg(args, unsigned int));
	  break;	
	case PURPLE_TYPE_LONG:       /**< Long integer.                     */
	  ruby_args[i] = INT2NUM(va_arg(args, long));
	  break;
	case PURPLE_TYPE_ULONG:      /**< Unsigned long integer.            */
	  ruby_args[i] = INT2NUM(va_arg(args, unsigned long));
	  break;
	case PURPLE_TYPE_INT64:      /**< 64-bit integer.                   */
	  ruby_args[i] = INT2NUM(va_arg(args, gint64));
	  break;	
	case PURPLE_TYPE_UINT64:     /**< 64-bit unsigned integer.          */
	  ruby_args[i] = INT2NUM(va_arg(args, guint64));
	  break;
	case PURPLE_TYPE_STRING:     /**< String.                           */
	  ruby_args[i] = rb_str_new2(va_arg(args, char *));
	  break;	
	case PURPLE_TYPE_OBJECT:     /**< Object pointer.                   */
	  FATAL("argument %d of callback from signal %s is of type PURPLE_TYPE_OBJECT which I dont know what the hell to do with",
		i, "");
	  break;	
	case PURPLE_TYPE_POINTER:    /**< Generic pointer.                  */
	  FATAL("argument %d of callback from signal %s is of type PURPLE_TYPE_POINTER which I dont know what the hell to do with",
		i, "");
	  break;	
	case PURPLE_TYPE_ENUM:       /**< Enum.                             */
	  FATAL("argument %d of callback from signal %s is of type PURPLE_TYPE_ENUM which I dont know what the hell to do with",
		i, "");
	  break;
	case PURPLE_TYPE_BOXED:      /**< Boxed pointer with specific type. */
	  FATAL("argument %d of callback from signal %s is of type PURPLE_TYPE_BOXED which I dont know what the hell to do with",
		i, "");
	  break;
	default: 
	  FATAL("argument %d of callback from signal %s is of a completely unknown type %d", 
		i, 
		handler->signal, 
		purple_value_get_type(values[i]));
	  break;
	}
      }
    }
  }
  
  event.receiver = handler->callable;
  event.method = call_method_id;
  event.argc = value_count;
  event.argv = ruby_args;
  event.lock = g_mutex_new();
  event.flag = g_cond_new();
  
  g_mutex_lock(event.lock);
  
  write(purple_event_queue[1], (char *) &event_pointer, sizeof(IncomingRuburpleEvent *));

  glib_sleeping = TRUE;
  g_cond_wait(event.flag, event.lock);
  glib_sleeping = FALSE;

  g_mutex_unlock(event.lock);
  
  g_mutex_free(event.lock);
  g_cond_free(event.flag);
  g_free(event.argv);
}

static VALUE
rb_ruburple_read_event(VALUE self)
{
  IncomingRuburpleEvent *event;
  VALUE return_value;
  int i;

  read(purple_event_queue[0], (char *) &event, sizeof(IncomingRuburpleEvent *));

  return_value = rb_funcall2(event->receiver, event->method, event->argc, event->argv);

  g_mutex_lock(event->lock);
  g_cond_signal(event->flag);
  g_mutex_unlock(event->lock);

  return return_value;
}

static gpointer
ruburple_signal_connect_vargs(gpointer data)
{
  RuburpleSignalHandler *handler = (RuburpleSignalHandler *) data;
  purple_signal_connect_vargs(handler->handle, handler->signal, (gpointer) handler->callable, PURPLE_CALLBACK(ruburple_callback), (gpointer) handler);
  return NULL;
}

static VALUE
rb_ruburple_subscribe(VALUE self, VALUE handle_value, VALUE signal_name, VALUE callable)
{
  VALUE return_value;
  gpointer handle;
  GPOINTER(handle_value, handle);
  RuburpleSignalHandler *handler = ruburple_signal_handler_new(handle,
							       RSTRING(signal_name)->ptr,
							       callable);

  call_and_get_result(ruburple_signal_connect_vargs, (gpointer) handler);

  return_value = RB_RUBURPLE_SUBSCRIPTION_gc(handler);

  rb_funcall(gc_save_set, add_method_id, 1, return_value);

  return return_value;
}

static gpointer
ruburple_signal_disconnect(gpointer data)
{
  RuburpleSignalHandler *handler = (RuburpleSignalHandler *) data;
  purple_signal_disconnect(handler->handle, handler->signal, (gpointer) handler->callable, PURPLE_CALLBACK(ruburple_callback));
  return NULL;
}

static VALUE
rb_ruburple_subscription_unsubscribe(VALUE self)
{
  RuburpleSignalHandler *handler;
  RUBURPLE_SIGNAL_HANDLER(self, handler);

  call_and_get_result(ruburple_signal_disconnect, (gpointer) handler);

  rb_funcall(gc_save_set, delete_method_id, 1, self);

  return Qnil;
}

static VALUE
rb_ruburple_protocol_account_get_connection(VALUE self)
{
  PurpleAccount *account;
  PurpleConnection *connection;
  PURPLE_ACCOUNT(self, account);
  connection = purple_account_get_connection(account);
  RETURN_QNIL_IF_NULL(connection);
  return RB_RUBURPLE_PROTOCOL_CONNECTION(connection);
}

static gpointer
ruburple_serv_send_im(gpointer data)
{
  gpointer *args = (gpointer *) data;
  * (int *) args[4] = serv_send_im((PurpleConnection *) args[0], (char *) args[1], (char *) args[2], * (int *) args[3]);
  return NULL;
}

static VALUE
rb_ruburple_protocol_connection_send_im(int argc, VALUE *argv, VALUE self)
{
  PurpleConnection *connection;
  int flags = 0;
  gpointer args[5];
  int rval;

  if (argc > 3)
    rb_raise(rb_eRuntimeError, "Ruburple::Protocol::Connection#send_im(RECIPIENT, MESSAGE, FLAGS = 0) takes at most 3 arguments");
  if (argc == 3) {
    Check_Type(argv[2], T_FIXNUM);
    flags = NUM2INT(argv[2]);
  }
  if (argc >= 2) {
    Check_Type(argv[1], T_STRING);
    Check_Type(argv[0], T_STRING);
  } else {
    rb_raise(rb_eRuntimeError, "Ruburple::Protocol::Connection#send_im(RECIPIENT, MESSAGE, FLAGS = 0) takes at least 2 arguments");
  }

  PURPLE_CONNECTION(self, connection);

  args[0] = (gpointer) connection;
  args[1] = (gpointer) RSTRING(argv[0])->ptr;
  args[2] = (gpointer) RSTRING(argv[1])->ptr;
  args[3] = (gpointer) &flags;
  args[4] = (gpointer) &rval;

  call_and_get_result(ruburple_serv_send_im, args);

  return INT2NUM(rval);
}

static gpointer
ruburple_protocol_account_is_connected(gpointer data)
{
  gpointer *args = (gpointer *) data;
  * (gboolean *) args[1] = purple_account_is_connected((PurpleAccount *) args[0]);
  return NULL;
}

static VALUE
rb_ruburple_protocol_account_is_connected(VALUE self)
{
  gpointer args[2];
  PurpleAccount *account;
  gboolean tmp;
  VALUE rval;
  PURPLE_ACCOUNT(self, account);
  args[0] = (gpointer) account;
  args[1] = (gpointer) &tmp;
  call_and_get_result(ruburple_protocol_account_is_connected, args);
  rval = BOOL(tmp);
  return rval;
}

static gpointer
ruburple_protocol_account_is_connecting(gpointer data)
{
  gpointer *args = (gpointer *) data;
  * (gboolean *) args[1] = purple_account_is_connecting((PurpleAccount *) args[0]);
  return NULL;
}

static VALUE
rb_ruburple_protocol_account_is_connecting(VALUE self)
{
  gpointer args[2];
  PurpleAccount *account;
  gboolean tmp;
  VALUE rval;
  PURPLE_ACCOUNT(self, account);
  args[0] = (gpointer) account;
  args[1] = (gpointer) &tmp;
  call_and_get_result(ruburple_protocol_account_is_connecting, args);
  rval = BOOL(tmp);
  return rval;
}

static gpointer
ruburple_protocol_connection_get_account(gpointer data)
{
  return (gpointer) purple_connection_get_account((PurpleConnection *) data);
}

static VALUE
rb_ruburple_protocol_connection_get_account(VALUE self)
{
  PurpleConnection *plugin;
  PURPLE_CONNECTION(self, plugin);
  return RB_RUBURPLE_PROTOCOL_ACCOUNT((PurpleAccount *) call_and_get_result(ruburple_protocol_connection_get_account, (gpointer) plugin));
}

static VALUE
rb_ruburple_protocol_connection_close(VALUE self)
{
  VALUE account = rb_ruburple_protocol_connection_get_account(self);
  rb_ruburple_protocol_account_set_enabled(account, Qfalse);
  return Qnil;
}

static gpointer
ruburple_protocol_account_get_buddy(gpointer data)
{
  gpointer *args = (gpointer *) data;
  return purple_find_buddy((PurpleAccount *) args[0], (char *) args[1]);
}

static VALUE
rb_ruburple_protocol_account_has_buddy(VALUE self, VALUE name)
{
  PurpleAccount *account;
  gpointer args[2];
  PurpleBuddy *buddy;
  PURPLE_ACCOUNT(self, account);
  args[0] = (gpointer) account;
  args[1] = (gpointer) RSTRING(name)->ptr;
  buddy = (PurpleBuddy *) call_and_get_result(ruburple_protocol_account_get_buddy, args);
  return buddy == NULL ? Qfalse : Qtrue;
}

static VALUE
rb_ruburple_protocol_account_get_buddy(VALUE self, VALUE name)
{
  PurpleAccount *account;
  gpointer args[2];
  PurpleBuddy *buddy;
  PURPLE_ACCOUNT(self, account);
  args[0] = (gpointer) account;
  args[1] = (gpointer) RSTRING(name)->ptr;
  buddy = (PurpleBuddy *) call_and_get_result(ruburple_protocol_account_get_buddy, args);
  RETURN_QNIL_IF_NULL(buddy);
  return RB_RUBURPLE_BLIST_BUDDY(buddy);
}

static gpointer
ruburple_protocol_account_get_buddies(gpointer data)
{
  gpointer *args = (gpointer *) data;
  return purple_find_buddies((PurpleAccount *) args[0], (char *) args[1]);
}

static VALUE
rb_ruburple_protocol_account_get_buddies(int argc, VALUE *argv, VALUE self)
{
  PurpleAccount *account;
  GList *iter;
  VALUE return_value = rb_ary_new();
  gpointer args[2];

  PURPLE_ACCOUNT(self, account);
  args[0] = (gpointer) account;
  if (argc > 1)
    rb_raise(rb_eRuntimeError, "Ruburple::Protocol::Account#buddies(USERNAME = nil) takes at most 1 argument");
  else if (argc == 1)
    args[1] = RSTRING(argv[0])->ptr;
  else
    args[1] = NULL;

  for (iter = (GList *) call_and_get_result(ruburple_protocol_account_get_buddies, args); iter; iter = iter->next) {
    rb_ary_push(return_value, RB_RUBURPLE_BLIST_BUDDY(iter->data));
  }

  return return_value;
}

static gpointer
ruburple_protocol_account_add_buddy(gpointer data)
{
  gpointer *args = (gpointer *) data;

  gpointer *buddy = (gpointer *) purple_buddy_new((PurpleAccount *) args[0], (char *) args[1], NULL);

  purple_account_add_buddy((PurpleAccount *) args[0], (PurpleBuddy *) buddy); 

  return buddy;
}

static VALUE
rb_ruburple_protocol_account_add_buddy(VALUE self, VALUE buddy)
{
  PurpleAccount *account;

  Check_Type(buddy, T_STRING);
  PURPLE_ACCOUNT(self, account);

  gpointer args[2];

  args[0] = (gpointer) account;
  args[1] = (gpointer) RSTRING(buddy)->ptr;

  PurpleBuddy *rval;
  rval = (PurpleBuddy *) call_and_get_result(ruburple_protocol_account_add_buddy, args);
  RETURN_QNIL_IF_NULL(rval);

  return RB_RUBURPLE_BLIST_BUDDY(rval);
}

static VALUE
ruburple_protocol_account_remove_buddy(gpointer data)
{
  PurpleBuddy *buddy;
  PurpleGroup *group;

  gpointer *args = (gpointer *) data;
  buddy = purple_find_buddy((PurpleAccount *) args[0], (char *) args[1]);
  group = purple_buddy_get_group(buddy);

  purple_account_remove_buddy((PurpleAccount *) args[0], buddy, group); 

  return Qnil;
}

static VALUE
rb_ruburple_protocol_account_remove_buddy(VALUE self, VALUE buddy)
{
  PurpleAccount *account;

  Check_Type(buddy, T_STRING);
  PURPLE_ACCOUNT(self, account);

  gpointer args[2];

  args[0] = (gpointer) account;
  args[1] = (gpointer) RSTRING(buddy)->ptr;

  call_and_get_result(ruburple_protocol_account_remove_buddy, args);

  return Qnil;
}


static gpointer
ruburple_blist_buddy_get_account(gpointer data)
{
  return (gpointer) purple_buddy_get_account((PurpleBuddy *) data);
}

static VALUE
rb_ruburple_blist_buddy_get_account(VALUE self)
{
  PurpleBuddy *buddy;
  PURPLE_BUDDY(self, buddy);
  return RB_RUBURPLE_PROTOCOL_ACCOUNT((PurpleAccount *) call_and_get_result(ruburple_blist_buddy_get_account, (gpointer) buddy));
}

static gpointer
ruburple_blist_buddy_get_name(gpointer data)
{
  return (gpointer) purple_buddy_get_name((PurpleBuddy *) data);
}

static VALUE
rb_ruburple_blist_buddy_get_name(VALUE self)
{
  PurpleBuddy *buddy;
  PURPLE_BUDDY(self, buddy);
  return rb_str_new2((char *) call_and_get_result(ruburple_blist_buddy_get_name, (gpointer) buddy));
}

static gpointer
ruburple_blist_buddy_get_group(gpointer data)
{
  return (gpointer) purple_buddy_get_group((PurpleBuddy *) data);
}

static VALUE
rb_ruburple_blist_buddy_get_group(VALUE self)
{
  PurpleBuddy *buddy;
  PURPLE_BUDDY(self, buddy);
  return RB_RUBURPLE_BLIST_GROUP((PurpleGroup *) call_and_get_result(ruburple_blist_buddy_get_group, (gpointer) buddy));
}

static gpointer
ruburple_protocol_account_status_get_name(gpointer data)
{
  PurpleStatus *status = (PurpleStatus *) data;
  return (gpointer) purple_status_get_name(status);
}

static VALUE
rb_ruburple_protocol_account_status_get_name(VALUE self)
{
  PurpleStatus *status;
  PURPLE_STATUS(self, status);
  return rb_str_new2((char *) call_and_get_result(ruburple_protocol_account_status_get_name, (gpointer) status));
}

static gpointer
ruburple_protocol_account_status_get_primitive(gpointer data)
{
  gpointer *args = (gpointer *) data;
  * (int *) args[1] = purple_status_type_get_primitive((PurpleStatusType *) args[0]);
  return NULL;
}

static VALUE
rb_ruburple_protocol_account_status_get_primitive(VALUE self)
{
  PurpleStatus *status;
  PurpleStatusType *type;
  gpointer args[2];
  int rval;
  PURPLE_STATUS(self, status);
  type = purple_status_get_type(status);
  args[0] = type;
  args[1] = &rval;
  call_and_get_result(ruburple_protocol_account_status_get_primitive, (gpointer) args);
  return INT2NUM(rval);
}

static VALUE
rb_ruburple_protocol_account_get_uid(VALUE self)
{
  PurpleAccount *account;
  PURPLE_ACCOUNT(self, account);
  return INT2NUM(GPOINTER_TO_INT(account));
}

static VALUE
rb_ruburple_protocol_connection_get_protocol(VALUE self)
{
  PurpleConnection *connection;
  PURPLE_CONNECTION(self, connection);
  RETURN_QNIL_IF_NULL(connection->prpl);
  return RB_RUBURPLE_PROTOCOL(connection->prpl);
}

static gpointer
ruburple_blist_buddy_get_status(gpointer data)
{
  return (gpointer) purple_presence_get_active_status(purple_buddy_get_presence((PurpleBuddy *) data));
}

static VALUE
rb_ruburple_blist_buddy_get_status(VALUE self)
{
  PurpleBuddy *buddy;
  PURPLE_BUDDY(self, buddy);
  return RB_RUBURPLE_PROTOCOL_ACCOUNT_STATUS((PurpleStatus *) call_and_get_result(ruburple_blist_buddy_get_status, (gpointer) buddy));
}
static gpointer
ruburple_blist_buddy_get_alias(gpointer data)
{
  return (gpointer) purple_buddy_get_contact_alias((PurpleBuddy *) data);
}

static VALUE
rb_ruburple_blist_buddy_get_alias(VALUE self)
{
  PurpleBuddy *buddy;
  PURPLE_BUDDY(self, buddy);
  return rb_str_new2((char *) call_and_get_result(ruburple_blist_buddy_get_alias, (gpointer) buddy));
}

static VALUE
rb_ruburple_blist_buddy_get_icon(VALUE self)
{
  PurpleBuddy *buddy;
  PURPLE_BUDDY(self, buddy);
  RETURN_QNIL_IF_NULL(buddy->icon);
  return RB_RUBURPLE_BLIST_BUDDY_ICON(buddy->icon);
}

static gpointer
ruburple_blist_buddy_icon_get_extension(gpointer data)
{
  return (gpointer) purple_buddy_icon_get_extension((PurpleBuddyIcon *) data);
}

static VALUE
rb_ruburple_blist_buddy_icon_get_extension(VALUE self)
{
  PurpleBuddyIcon *buddy_icon;
  char *extension;
  PURPLE_BUDDY_ICON(self, buddy_icon);
  extension = (char *) call_and_get_result(ruburple_blist_buddy_icon_get_extension, (gpointer) buddy_icon);
  RETURN_QNIL_IF_NULL(extension);
  return rb_str_new2(extension);
}

static gpointer
ruburple_blist_buddy_icon_get_full_path(gpointer data)
{
  return (gpointer) purple_buddy_icon_get_full_path((PurpleBuddyIcon *) data);
}

static VALUE
rb_ruburple_blist_buddy_icon_get_full_path(VALUE self)
{
  PurpleBuddyIcon *buddy_icon;
  char *rval;
  PURPLE_BUDDY_ICON(self, buddy_icon);
  rval = (char *) call_and_get_result(ruburple_blist_buddy_icon_get_full_path, (gpointer) buddy_icon);
  RETURN_QNIL_IF_NULL(rval);
  return rb_str_new2(rval);
}


#ifdef __cplusplus
extern "C" {
#endif
  void Init_ruburple_ext() {
    VALUE io_class = rb_define_class("IO", rb_cObject);
    VALUE new_method = rb_intern("new");
    VALUE set_class;

    rb_f_require(0, rb_str_new2("set"));

    set_class = rb_define_class("Set", rb_cObject);

    gc_save_set = rb_funcall(set_class, new_method, 0);

    g_thread_init(NULL);

    glib_sleeping = FALSE;

    add_method_id = rb_intern("add");
    delete_method_id = rb_intern("delete");

    if (pipe(purple_event_queue) == -1)
      rb_raise(rb_eRuntimeError, "error creating event pipe from libpurple to ruby: %s", strerror(errno));

    call_method_id = rb_intern("call");
    idle_handler_method_id = rb_intern("_idle_handler");

    rb_ruburple = rb_define_module("Ruburple");
    rb_define_singleton_method(rb_ruburple, "quit", rb_ruburple_quit, 0);
    rb_define_singleton_method(rb_ruburple, "debug=", rb_ruburple_set_debug, 1);
    rb_define_singleton_method(rb_ruburple, "protocols", rb_ruburple_get_protocols, 0);
    rb_define_singleton_method(rb_ruburple, "get_protocol", rb_ruburple_get_protocol, 1);
    rb_define_singleton_method(rb_ruburple, "has_protocol?", rb_ruburple_has_protocol, 1);
    rb_define_singleton_method(rb_ruburple, "get_blist", rb_ruburple_get_blist, 0);
    rb_define_singleton_method(rb_ruburple, "_init", rb_ruburple_init, 0);
    rb_define_singleton_method(rb_ruburple, "_subscribe", rb_ruburple_subscribe, 3);
    rb_define_singleton_method(rb_ruburple, "_read_event", rb_ruburple_read_event, 0);
    rb_define_const(rb_ruburple, "MESSAGE_SEND", INT2NUM(PURPLE_MESSAGE_SEND)); /**< Outgoing message.        */
    rb_define_const(rb_ruburple, "MESSAGE_RECV", INT2NUM(PURPLE_MESSAGE_RECV)); /**< Incoming message.        */
    rb_define_const(rb_ruburple, "MESSAGE_SYSTEM", INT2NUM(PURPLE_MESSAGE_SYSTEM)); /**< System message.          */
    rb_define_const(rb_ruburple, "MESSAGE_AUTO_RESP", INT2NUM(PURPLE_MESSAGE_AUTO_RESP)); /**< Auto response.           */
    rb_define_const(rb_ruburple, "MESSAGE_ACTIVE_ONLY", INT2NUM(PURPLE_MESSAGE_ACTIVE_ONLY)); /**< Hint to the UI that this
												 message should not be
												 shown in conversations
												 which are only open for
												 internal UI purposes
												 (e.g. for contact-aware
												 conversions).           */
    rb_define_const(rb_ruburple, "MESSAGE_NICK", INT2NUM(PURPLE_MESSAGE_NICK)); /**< Contains your nick.      */
    rb_define_const(rb_ruburple, "MESSAGE_NO_LOG", INT2NUM(PURPLE_MESSAGE_NO_LOG)); /**< Do not log.              */
    rb_define_const(rb_ruburple, "MESSAGE_WHISPER", INT2NUM(PURPLE_MESSAGE_WHISPER)); /**< Whispered message.       */
    rb_define_const(rb_ruburple, "MESSAGE_ERROR", INT2NUM(PURPLE_MESSAGE_ERROR)); /**< Error message.           */
    rb_define_const(rb_ruburple, "MESSAGE_DELAYED", INT2NUM(PURPLE_MESSAGE_DELAYED)); /**< Delayed message.         */
    rb_define_const(rb_ruburple, "MESSAGE_RAW", INT2NUM(PURPLE_MESSAGE_RAW)); /**< "Raw" message - don't
										 apply formatting         */
    rb_define_const(rb_ruburple, "MESSAGE_IMAGES", INT2NUM(PURPLE_MESSAGE_IMAGES)); /**< Message contains images  */
    rb_define_const(rb_ruburple, "MESSAGE_NOTIFY", INT2NUM(PURPLE_MESSAGE_NOTIFY)); /**< Message is a notification */
    
    rb_ruburple_pointer = rb_define_class_under(rb_ruburple, "Pointer", rb_cObject);

    rb_define_const(rb_ruburple, "HANDLE_ACCOUNTS", RB_RUBURPLE_POINTER(purple_accounts_get_handle()));    
    rb_define_const(rb_ruburple, "HANDLE_CONNECTIONS", RB_RUBURPLE_POINTER(purple_connections_get_handle()));
    rb_define_const(rb_ruburple, "HANDLE_CONVERSATIONS", RB_RUBURPLE_POINTER(purple_conversations_get_handle()));
    rb_define_const(rb_ruburple, "HANDLE_BLIST", RB_RUBURPLE_POINTER(purple_blist_get_handle()));
    rb_define_const(rb_ruburple, "HANDLE_CIPHERS", RB_RUBURPLE_POINTER(purple_ciphers_get_handle()));
    rb_define_const(rb_ruburple, "HANDLE_CORE", RB_RUBURPLE_POINTER(purple_get_core()));
    rb_define_const(rb_ruburple, "HANDLE_LOG", RB_RUBURPLE_POINTER(purple_log_get_handle()));
    rb_define_const(rb_ruburple, "HANDLE_PLUGINS", RB_RUBURPLE_POINTER(purple_plugins_get_handle()));
    rb_define_const(rb_ruburple, "HANDLE_SAVEDSTATUSES", RB_RUBURPLE_POINTER(purple_savedstatuses_get_handle()));
    rb_define_const(rb_ruburple, "HANDLE_XFERS", RB_RUBURPLE_POINTER(purple_xfers_get_handle()));
    rb_define_const(rb_ruburple, "EVENT_INPUT", rb_funcall(io_class, new_method, 1, INT2NUM(purple_event_queue[0])));
    rb_define_const(rb_ruburple, "SUBSCRIPTIONS", gc_save_set);


    rb_ruburple_plugin = rb_define_class_under(rb_ruburple, "Plugin", rb_cObject);

    rb_ruburple_protocol = rb_define_class_under(rb_ruburple, "Protocol", rb_ruburple_plugin);
    rb_define_method(rb_ruburple_protocol, "get_account", rb_ruburple_protocol_get_account, -1);
    rb_define_method(rb_ruburple_protocol, "id", rb_protocol_get_id, 0);
    rb_define_method(rb_ruburple_protocol, "name", rb_protocol_get_name, 0);
    rb_define_method(rb_ruburple_protocol, "version", rb_protocol_get_version, 0);
    rb_define_method(rb_ruburple_protocol, "summary", rb_protocol_get_summary, 0);
    rb_define_method(rb_ruburple_protocol, "description", rb_protocol_get_description, 0);
    rb_define_method(rb_ruburple_protocol, "author", rb_protocol_get_author, 0);
    rb_define_method(rb_ruburple_protocol, "homepage", rb_protocol_get_homepage, 0);

    rb_ruburple_protocol_account = rb_define_class_under(rb_ruburple_protocol, "Account", rb_cObject);
    rb_define_method(rb_ruburple_protocol_account, "password=", rb_ruburple_protocol_account_set_password, 1);
    rb_define_method(rb_ruburple_protocol_account, "connect", rb_ruburple_protocol_account_connect, 0);
    rb_define_method(rb_ruburple_protocol_account, "enabled=", rb_ruburple_protocol_account_set_enabled, 1);
    rb_define_method(rb_ruburple_protocol_account, "savedstatus=", rb_ruburple_protocol_account_set_savedstatus, 1);
    rb_define_method(rb_ruburple_protocol_account, "connection", rb_ruburple_protocol_account_get_connection, 0);
    rb_define_method(rb_ruburple_protocol_account, "connected?", rb_ruburple_protocol_account_is_connected, 0);
    rb_define_method(rb_ruburple_protocol_account, "connecting?", rb_ruburple_protocol_account_is_connecting, 0);
    rb_define_method(rb_ruburple_protocol_account, "username", rb_ruburple_protocol_account_get_username, 0);
    rb_define_method(rb_ruburple_protocol_account, "password", rb_ruburple_protocol_account_get_password, 0);
    rb_define_method(rb_ruburple_protocol_account, "alias", rb_ruburple_protocol_account_get_alias, 0);
    rb_define_method(rb_ruburple_protocol_account, "has_buddy?", rb_ruburple_protocol_account_has_buddy, 1);
    rb_define_method(rb_ruburple_protocol_account, "get_buddy", rb_ruburple_protocol_account_get_buddy, 1);
    rb_define_method(rb_ruburple_protocol_account, "add_buddy", rb_ruburple_protocol_account_add_buddy, 1);
    rb_define_method(rb_ruburple_protocol_account, "remove_buddy", rb_ruburple_protocol_account_remove_buddy, 1);
    rb_define_method(rb_ruburple_protocol_account, "buddies", rb_ruburple_protocol_account_get_buddies, -1);
    rb_define_method(rb_ruburple_protocol_account, "uid", rb_ruburple_protocol_account_get_uid, 0);
    rb_define_method(rb_ruburple_protocol_account, "status", rb_ruburple_protocol_account_get_status, 0);
    
    rb_ruburple_blist = rb_define_class_under(rb_ruburple, "BuddyList", rb_cObject);
    /*rb_define_method(rb_ruburple_blist, "new", rb_ruburple_blist_new, 0);
    rb_define_method(rb_ruburple_blist, "get_root", rb_ruburple_blist_get_root, 0);    
    rb_define_method(rb_ruburple_blist, "node_next", rb_ruburple_blist_node_next, 2);
    rb_define_method(rb_ruburple_blist, "node_get_parent", rb_ruburple_blist_node_get_parent, 1);
    rb_define_method(rb_ruburple_blist, "node_get_first_child", rb_ruburple_blist_node_get_first_child, 1);
    rb_define_method(rb_ruburple_blist, "node_get_sibling_next", rb_ruburple_blist_node_get_sibling_next, 1);
    rb_define_method(rb_ruburple_blist, "node_get_sibling_prev", rb_ruburple_blist_node_get_sibling_prev, 1);
    rb_define_method(rb_ruburple_blist, "destroy", rb_ruburple_blist_destroy, 0);
    rb_define_method(rb_ruburple_blist, "set_visible", rb_ruburple_blist_set_visible, 1);
    rb_define_method(rb_ruburple_blist, "update_buddy_status", rb_ruburple_blist_update_buddy_status, 2);
		rb_define_method(rb_ruburple_blist, "update_buddy_icon", rb_ruburple_blist_upldate_buddy_icon, 1);
		rb_define_method(rb_ruburple_blist, "rename_buddy", rb_ruburple_blist_rename_buddy, 2);
    rb_define_method(rb_ruburple_blist, "alias_contact", rb_ruburple_blist_alias_contact, 2);
    rb_define_method(rb_ruburple_blist, "server_alias_buddy", rb_ruburple_blist_server_alias_buddy, 2);
		rb_define_method(rb_ruburple_blist, "alias_chat", rb_ruburple_blist_alias_chat, 2);
		rb_define_method(rb_ruburple_blist, "rename_group", rb_ruburple_blist_rename_group, 2);
		rb_define_method(rb_ruburple_blist, "add_chat", rb_ruburple_blist_add_chat, 2);
		rb_define_method(rb_ruburple_blist, "add_buddy", rb_ruburple_blist_add_buddy, 4);
		rb_define_method(rb_ruburple_blist, "add_group", rb_ruburple_blist_add_group, 2);
		rb_define_method(rb_ruburple_blist, "add_contact", rb_ruburple_blist_add_contact, 3);
		rb_define_method(rb_ruburple_blist, "merge_contact", rb_ruburple_blist_merge_contact, 2);
		rb_define_method(rb_ruburple_blist, "remove_buddy", rb_ruburple_blist_remove_buddy, 1);
		rb_define_method(rb_ruburple_blist, "remove_contact", rb_ruburple_blist_remove_contact, 1);
		rb_define_method(rb_ruburple_blist, "remove_chat", rb_ruburple_blist_remove_chat, 1);
		rb_define_method(rb_ruburple_blist, "remove_group", rb_ruburple_blist_remove_group, 1);
		rb_define_method(rb_ruburple_blist, "find_chat", rb_ruburple_blist_find_chat, 2);
		rb_define_method(rb_ruburple_blist, "add_account", rb_ruburple_blist_add_account, 1);
		rb_define_method(rb_ruburple_blist, "remove_account", rb_ruburple_blist_remove_account, 1);
		rb_define_method(rb_ruburple_blist, "get_group_size", rb_ruburple_blist_get_group_size, 2);
		rb_define_method(rb_ruburple_blist, "get_group_online_count", rb_ruburple_blist_get_group_online_count, 1);
    */
		rb_define_method(rb_ruburple_blist, "get_group_size", rb_ruburple_blist_get_group_size, 1);

    rb_ruburple_protocol_account_conversation = rb_define_class_under(rb_ruburple_protocol_account, "Conversation", rb_cObject);

    rb_ruburple_protocol_account_savedstatus = rb_define_class_under(rb_ruburple_protocol_account, "SavedStatus", rb_cObject);
    rb_define_alloc_func(rb_ruburple_protocol_account_savedstatus, rb_ruburple_protocol_account_savedstatus_alloc);
    rb_define_private_method(rb_ruburple_protocol_account_savedstatus, "initialize", rb_ruburple_protocol_account_savedstatus_initialize, 1);

    rb_ruburple_protocol_account_status = rb_define_class_under(rb_ruburple_protocol_account, "Status", rb_cObject);
    rb_define_method(rb_ruburple_protocol_account_status, "name", rb_ruburple_protocol_account_status_get_name, 0);
    rb_define_method(rb_ruburple_protocol_account_status, "primitive", rb_ruburple_protocol_account_status_get_primitive, 0);
    rb_define_const(rb_ruburple_protocol_account_status, "STATUS_UNSET", INT2NUM(PURPLE_STATUS_UNSET));
    rb_define_const(rb_ruburple_protocol_account_status, "STATUS_OFFLINE", INT2NUM(PURPLE_STATUS_OFFLINE));
    rb_define_const(rb_ruburple_protocol_account_status, "STATUS_AVAILABLE", INT2NUM(PURPLE_STATUS_AVAILABLE));
    rb_define_const(rb_ruburple_protocol_account_status, "STATUS_UNAVAILABLE", INT2NUM(PURPLE_STATUS_UNAVAILABLE));
    rb_define_const(rb_ruburple_protocol_account_status, "STATUS_INVISIBLE", INT2NUM(PURPLE_STATUS_INVISIBLE));
    rb_define_const(rb_ruburple_protocol_account_status, "STATUS_AWAY", INT2NUM(PURPLE_STATUS_AWAY));
    rb_define_const(rb_ruburple_protocol_account_status, "STATUS_EXTENDED_AWAY", INT2NUM(PURPLE_STATUS_EXTENDED_AWAY));
    rb_define_const(rb_ruburple_protocol_account_status, "STATUS_MOBILE", INT2NUM(PURPLE_STATUS_MOBILE));

    rb_ruburple_protocol_connection = rb_define_class_under(rb_ruburple_protocol, "Connection", rb_cObject);
    rb_define_method(rb_ruburple_protocol_connection, "send_im", rb_ruburple_protocol_connection_send_im, -1);
    rb_define_method(rb_ruburple_protocol_connection, "protocol", rb_ruburple_protocol_connection_get_protocol, 0);
    rb_define_method(rb_ruburple_protocol_connection, "close", rb_ruburple_protocol_connection_close, 0);
    rb_define_method(rb_ruburple_protocol_connection, "account", rb_ruburple_protocol_connection_get_account, 0);

    rb_ruburple_protocol_account_transfer = rb_define_class_under(rb_ruburple_protocol_account, "Transfer", rb_cObject);

    rb_ruburple_blist_node = rb_define_class_under(rb_ruburple_blist, "Node", rb_cObject);

    rb_ruburple_blist_buddy = rb_define_class_under(rb_ruburple_blist, "Buddy", rb_ruburple_blist_node);
    rb_define_method(rb_ruburple_blist_buddy, "account", rb_ruburple_blist_buddy_get_account, 0);
    rb_define_method(rb_ruburple_blist_buddy, "name", rb_ruburple_blist_buddy_get_name, 0);
    rb_define_method(rb_ruburple_blist_buddy, "status", rb_ruburple_blist_buddy_get_status, 0);
    rb_define_method(rb_ruburple_blist_buddy, "alias", rb_ruburple_blist_buddy_get_alias, 0);
    rb_define_method(rb_ruburple_blist_buddy, "icon", rb_ruburple_blist_buddy_get_icon, 0);
    rb_define_method(rb_ruburple_blist_buddy, "group", rb_ruburple_blist_buddy_get_group, 0);

    rb_ruburple_blist_group = rb_define_class_under(rb_ruburple_blist, "Group", rb_cObject);
    rb_define_singleton_method(rb_ruburple_blist_group, "new", rb_ruburple_blist_group_new, 1);
    rb_define_method(rb_ruburple_blist_group, "name", rb_ruburple_blist_group_get_name, 0);
    rb_define_method(rb_ruburple_blist_group, "accounts", rb_ruburple_blist_group_get_accounts, 0);

    rb_ruburple_blist_contact = rb_define_class_under(rb_ruburple_blist, "Contact", rb_ruburple_blist_node);

    rb_ruburple_blist_chat = rb_define_class_under(rb_ruburple_blist, "Chat", rb_ruburple_blist_node);

    rb_ruburple_blist_buddy_icon = rb_define_class_under(rb_ruburple_blist, "BuddyIcon", rb_cObject);
    rb_define_method(rb_ruburple_blist_buddy_icon, "extension", rb_ruburple_blist_buddy_icon_get_extension, 0);
    rb_define_method(rb_ruburple_blist_buddy_icon, "full_path", rb_ruburple_blist_buddy_icon_get_full_path, 0);

    rb_ruburple_handle = rb_define_class_under(rb_ruburple, "Handle", rb_cObject);

    rb_ruburple_cipher = rb_define_class_under(rb_ruburple, "Cipher", rb_cObject);

    rb_ruburple_log = rb_define_class_under(rb_ruburple, "Log", rb_cObject);

    rb_ruburple_xmlnode = rb_define_class_under(rb_ruburple, "XMLNode", rb_cObject);

    rb_ruburple_userinfo = rb_define_class_under(rb_ruburple, "UserInfo", rb_cObject);

    rb_ruburple_storedimage = rb_define_class_under(rb_ruburple, "StoredImage", rb_cObject);

    rb_ruburple_subscription = rb_define_class_under(rb_ruburple, "Subscription", rb_cObject);
    rb_define_method(rb_ruburple_subscription, "unsubscribe", rb_ruburple_subscription_unsubscribe, 0);

  }
#ifdef __cplusplus
}
#endif

