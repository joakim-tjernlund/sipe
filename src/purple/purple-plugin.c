/**
 * @file purple-plugin.c
 *
 * pidgin-sipe
 *
 * Copyright (C) 2010 SIPE Project <http://sipe.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sipe-common.h"

/* Flag needed for correct version of PURPLE_INIT_PLUGIN() */
#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

#include "accountopt.h"
#include "blist.h"
#include "connection.h"
#include "prpl.h"
#include "plugin.h"
#include "request.h"
#include "status.h"
#include "version.h"

#include "sipe-backend.h"
#include "sipe-core.h"
#include "sipe-nls.h"

#include "core-depurple.h"

/* PurplePluginProtocolInfo function calls & data structure */
static const char *sipe_list_icon(SIPE_UNUSED_PARAMETER PurpleAccount *a,
				  SIPE_UNUSED_PARAMETER PurpleBuddy *b)
{
	return "sipe";
}

static gchar *sipe_status_text(PurpleBuddy *buddy)
{
	const PurpleStatus *status = purple_presence_get_active_status(purple_buddy_get_presence(buddy));
	return sipe_core_buddy_status(buddy->account->gc->proto_data,
				      buddy->name,
				      purple_status_get_id(status),
				      purple_status_get_name(status));
}

static GList *sipe_blist_node_menu(PurpleBlistNode *node)
{
	if(PURPLE_BLIST_NODE_IS_BUDDY(node)) {
		return sipe_buddy_menu((PurpleBuddy *) node);
	} else if(PURPLE_BLIST_NODE_IS_CHAT(node)) {
		return sipe_chat_menu((PurpleChat *)node);
	} else {
		return NULL;
	}
}

static void sipe_add_permit(PurpleConnection *gc, const char *name)
{
	sipe_core_contact_allow_deny(gc->proto_data, name, TRUE);
}

static void sipe_add_deny(PurpleConnection *gc, const char *name)
{
	sipe_core_contact_allow_deny(gc->proto_data, name, FALSE);
}

static void sipe_chat_invite(PurpleConnection *gc, int id,
			     SIPE_UNUSED_PARAMETER const char *message,
			     const char *name)
{
	sipe_core_chat_create(gc->proto_data, id, name);
}

static void sipe_alias_buddy(PurpleConnection *gc, const char *name,
			     SIPE_UNUSED_PARAMETER const char *alias)
{
	sipe_core_group_set_user(gc->proto_data, name);
}

#if PURPLE_VERSION_CHECK(2,5,0)
static GHashTable *
sipe_get_account_text_table(SIPE_UNUSED_PARAMETER PurpleAccount *account)
{
	GHashTable *table;
	table = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_insert(table, "login_label", (gpointer)_("user@company.com"));
	return table;
}
#endif

static PurplePluginProtocolInfo prpl_info =
{
	OPT_PROTO_CHAT_TOPIC,
	NULL,					/* user_splits */
	NULL,					/* protocol_options */
	NO_BUDDY_ICONS,				/* icon_spec */
	sipe_list_icon,				/* list_icon */
	NULL,					/* list_emblems */
	sipe_status_text,			/* status_text */
	sipe_tooltip_text,			/* tooltip_text */	// add custom info to contact tooltip
	sipe_status_types,			/* away_states */
	sipe_blist_node_menu,			/* blist_node_menu */
	NULL,					/* chat_info */
	NULL,					/* chat_info_defaults */
	sipe_login,				/* login */
	sipe_close,				/* close */
	sipe_im_send,				/* send_im */
	NULL,					/* set_info */		// TODO maybe
	sipe_send_typing,			/* send_typing */
	sipe_get_info,				/* get_info */
	sipe_set_status,			/* set_status */
	sipe_set_idle,				/* set_idle */
	NULL,					/* change_passwd */
	sipe_add_buddy,				/* add_buddy */
	NULL,					/* add_buddies */
	sipe_remove_buddy,			/* remove_buddy */
	NULL,					/* remove_buddies */
	sipe_add_permit,			/* add_permit */
	sipe_add_deny,				/* add_deny */
	sipe_add_deny,				/* rem_permit */
	sipe_add_permit,			/* rem_deny */
	NULL,					/* set_permit_deny */
	NULL,					/* join_chat */
	NULL,					/* reject_chat */
	NULL,					/* get_chat_name */
	sipe_chat_invite,			/* chat_invite */
	sipe_chat_leave,			/* chat_leave */
	NULL,					/* chat_whisper */
	sipe_chat_send,				/* chat_send */
	sipe_keep_alive,			/* keepalive */
	NULL,					/* register_user */
	NULL,					/* get_cb_info */	// deprecated
	NULL,					/* get_cb_away */	// deprecated
	sipe_alias_buddy,			/* alias_buddy */
	sipe_group_buddy,			/* group_buddy */
	sipe_rename_group,			/* rename_group */
	NULL,					/* buddy_free */
	sipe_convo_closed,			/* convo_closed */
	purple_normalize_nocase,		/* normalize */
	NULL,					/* set_buddy_icon */
	sipe_remove_group,			/* remove_group */
	NULL,					/* get_cb_real_name */	// TODO?
	NULL,					/* set_chat_topic */
	NULL,					/* find_blist_chat */
	NULL,					/* roomlist_get_list */
	NULL,					/* roomlist_cancel */
	NULL,					/* roomlist_expand_category */
	NULL,					/* can_receive_file */
	sipe_ft_send_file,			/* send_file */
	sipe_ft_new_xfer,			/* new_xfer */
	NULL,					/* offline_message */
	NULL,					/* whiteboard_prpl_ops */
	sipe_send_raw,				/* send_raw */
	NULL,					/* roomlist_room_serialize */
	NULL,					/* unregister_user */
	NULL,					/* send_attention */
	NULL,					/* get_attention_types */
#if !PURPLE_VERSION_CHECK(2,5,0)
	/* Backward compatibility when compiling against 2.4.x API */
	(void (*)(void))			/* _purple_reserved4 */
#endif
	sizeof(PurplePluginProtocolInfo),       /* struct_size */
#if PURPLE_VERSION_CHECK(2,5,0)
	sipe_get_account_text_table,		/* get_account_text_table */
#if PURPLE_VERSION_CHECK(2,6,0)
	NULL,					/* initiate_media */
	NULL,					/* get_media_caps */
#if PURPLE_VERSION_CHECK(2,7,0)
	NULL,					/* get_moods */
#endif
#endif
#endif
};

/* PurplePluginInfo function calls & data structure */
static gboolean sipe_plugin_load(SIPE_UNUSED_PARAMETER PurplePlugin *plugin)
{
	return TRUE;
}

static gboolean sipe_plugin_unload(SIPE_UNUSED_PARAMETER PurplePlugin *plugin)
{
	return TRUE;
}

static void sipe_plugin_destroy(SIPE_UNUSED_PARAMETER PurplePlugin *plugin)
{
	GList *entry;

	sipe_core_destroy();

	entry = prpl_info.protocol_options;
	while (entry) {
		purple_account_option_destroy(entry->data);
		entry = g_list_delete_link(entry, entry);
	}
	prpl_info.protocol_options = NULL;

	entry = prpl_info.user_splits;
	while (entry) {
		purple_account_user_split_destroy(entry->data);
		entry = g_list_delete_link(entry, entry);
	}
	prpl_info.user_splits = NULL;
}

static void sipe_show_about_plugin(PurplePluginAction *action)
{
	gchar *tmp = sipe_core_about();
	purple_notify_formatted((PurpleConnection *) action->context,
				NULL, " ", NULL, tmp, NULL, NULL);
	g_free(tmp);
}

static void sipe_show_find_contact(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	PurpleRequestFields *fields;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;

	fields = purple_request_fields_new();
	group = purple_request_field_group_new(NULL);
	purple_request_fields_add_group(fields, group);

	field = purple_request_field_string_new("givenName", _("First name"), NULL, FALSE);
	purple_request_field_group_add_field(group, field);
	field = purple_request_field_string_new("sn", _("Last name"), NULL, FALSE);
	purple_request_field_group_add_field(group, field);
	field = purple_request_field_string_new("company", _("Company"), NULL, FALSE);
	purple_request_field_group_add_field(group, field);
	field = purple_request_field_string_new("c", _("Country"), NULL, FALSE);
	purple_request_field_group_add_field(group, field);

	purple_request_fields(gc,
		_("Search"),
		_("Search for a contact"),
		_("Enter the information for the person you wish to find. Empty fields will be ignored."),
		fields,
		_("_Search"), G_CALLBACK(sipe_search_contact_with_cb),
		_("_Cancel"), NULL,
		purple_connection_get_account(gc), NULL, NULL, gc);
}

static void sipe_republish_calendar(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	sipe_core_update_calendar(gc->proto_data);
}

static void sipe_reset_status(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	sipe_core_reset_status(gc->proto_data);
}

static GList *sipe_actions(SIPE_UNUSED_PARAMETER PurplePlugin *plugin,
			   gpointer context)
{
	PurpleConnection *gc = (PurpleConnection *)context;
	GList *menu = NULL;
	PurplePluginAction *act;
	const char* calendar = purple_account_get_string(purple_connection_get_account(gc),
							 "calendar", "EXCH");

	act = purple_plugin_action_new(_("About SIPE plugin..."), sipe_show_about_plugin);
	menu = g_list_prepend(menu, act);

	act = purple_plugin_action_new(_("Contact search..."), sipe_show_find_contact);
	menu = g_list_prepend(menu, act);

	if (sipe_strequal(calendar, "EXCH")) {
		act = purple_plugin_action_new(_("Republish Calendar"), sipe_republish_calendar);
		menu = g_list_prepend(menu, act);
	}

	act = purple_plugin_action_new(_("Reset status"), sipe_reset_status);
	menu = g_list_prepend(menu, act);

	return g_list_reverse(menu);
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,                           /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                          /**< priority       */
	"prpl-sipe",                                   	  /**< id             */
	"Office Communicator",                            /**< name           */
	PACKAGE_VERSION,                                  /**< version        */
	"Microsoft Office Communicator Protocol Plugin",  /**< summary        */
	"A plugin for the extended SIP/SIMPLE protocol used by "          /**< description */
	"Microsoft Live/Office Communications Server (LCS2005/OCS2007+)", /**< description */
	"Anibal Avelar <avelar@gmail.com>, "              /**< author         */
	"Gabriel Burt <gburt@novell.com>, "               /**< author         */
	"Stefan Becker <stefan.becker@nokia.com>, "       /**< author         */
	"pier11 <pier11@operamail.com>",                  /**< author         */
	PACKAGE_URL,                                      /**< homepage       */
	sipe_plugin_load,                                 /**< load           */
	sipe_plugin_unload,                               /**< unload         */
	sipe_plugin_destroy,                              /**< destroy        */
	NULL,                                             /**< ui_info        */
	&prpl_info,                                       /**< extra_info     */
	NULL,
	sipe_actions,
	NULL,
	NULL,
	NULL,
	NULL
};

static void init_plugin(PurplePlugin *plugin)
{
	PurpleAccountUserSplit *split;
	PurpleAccountOption *option;

	/* This needs to be called first */
	sipe_core_init();

	purple_plugin_register(plugin);

	split = purple_account_user_split_new(_("Login\n   user  or  DOMAIN\\user  or\n   user@company.com"), NULL, ',');
	purple_account_user_split_set_reverse(split, FALSE);
	prpl_info.user_splits = g_list_append(prpl_info.user_splits, split);

	option = purple_account_option_string_new(_("Server[:Port]\n(leave empty for auto-discovery)"), "server", "");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_list_new(_("Connection type"), "transport", NULL);
	purple_account_option_add_list_item(option, _("Auto"), "auto");
	purple_account_option_add_list_item(option, _("SSL/TLS"), "tls");
	purple_account_option_add_list_item(option, _("TCP"), "tcp");
	purple_account_option_add_list_item(option, _("UDP"), "udp");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	/*option = purple_account_option_bool_new(_("Publish status (note: everyone may watch you)"), "doservice", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);*/

	option = purple_account_option_string_new(_("User Agent"), "useragent", "");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

#ifdef HAVE_LIBKRB5
	option = purple_account_option_bool_new(_("Use Kerberos"), "krb5", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	/* Suitable for sspi/NTLM, sspi/Kerberos and krb5 security mechanisms
	 * No login/password is taken into account if this option present,
	 * instead used default credentials stored in OS.
	 */
	option = purple_account_option_bool_new(_("Use Single Sign-On"), "sso", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);
#endif

	option = purple_account_option_list_new(_("Calendar source"), "calendar", NULL);
	purple_account_option_add_list_item(option, _("Exchange 2007/2010"), "EXCH");
	purple_account_option_add_list_item(option, _("None"), "NONE");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	/** Example: https://server.company.com/EWS/Exchange.asmx */
	option = purple_account_option_string_new(_("Email services URL\n(leave empty for auto-discovery)"), "email_url", "");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new(_("Email address\n(if different from Username)"), "email", "");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	/** Example: DOMAIN\user  or  user@company.com */
	option = purple_account_option_string_new(_("Email login\n(if different from Login)"), "email_login", "");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new(_("Email password\n(if different from Password)"), "email_password", "");
	purple_account_option_set_masked(option, TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);
}

/* This macro makes the code a purple plugin */
PURPLE_INIT_PLUGIN(sipe, init_plugin, info);

/*
  Local Variables:
  mode: c
  c-file-style: "bsd"
  indent-tabs-mode: t
  tab-width: 8
  End:
*/
