# Ruburple - a ruby wrapper for libpurple
# Copyright (C) 2007 Martin Kihlgren <zond at troja dot ath dot cx>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

require 'monitor'
require 'pp'

module Ruburple

  @@events = {
    :account_added => Ruburple::HANDLE_ACCOUNTS,
    :account_connecting => Ruburple::HANDLE_ACCOUNTS,
    :account_removed => Ruburple::HANDLE_ACCOUNTS,
    :account_disabled => Ruburple::HANDLE_ACCOUNTS,
    :account_enabled => Ruburple::HANDLE_ACCOUNTS,
    :account_setting_info => Ruburple::HANDLE_ACCOUNTS,
    :account_set_info => Ruburple::HANDLE_ACCOUNTS,
    :account_status_changed => Ruburple::HANDLE_ACCOUNTS,
    :account_alias_changed => Ruburple::HANDLE_ACCOUNTS,
    :buddy_status_changed => Ruburple::HANDLE_BLIST,
    :buddy_idle_changed => Ruburple::HANDLE_BLIST,
    :buddy_signed_on => Ruburple::HANDLE_BLIST,
    :buddy_signed_off => Ruburple::HANDLE_BLIST,
    :update_idle => Ruburple::HANDLE_BLIST,
    :blist_node_extended_menu => Ruburple::HANDLE_BLIST,
    :buddy_added => Ruburple::HANDLE_BLIST,
    :buddy_removed => Ruburple::HANDLE_BLIST,
    :buddy_icon_changed => Ruburple::HANDLE_BLIST,
    :blist_node_aliased => Ruburple::HANDLE_BLIST,
    :cipher_added => Ruburple::HANDLE_CIPHERS,
    :cipher_removed => Ruburple::HANDLE_CIPHERS,
    :signing_on => Ruburple::HANDLE_CONNECTIONS,
    :signed_on => Ruburple::HANDLE_CONNECTIONS,
    :signing_off => Ruburple::HANDLE_CONNECTIONS,
    :signed_off => Ruburple::HANDLE_CONNECTIONS,
    :writing_im_msg => Ruburple::HANDLE_CONVERSATIONS,
    :wrote_im_msg => Ruburple::HANDLE_CONVERSATIONS,
    :sending_im_msg => Ruburple::HANDLE_CONVERSATIONS,
    :sent_im_msg => Ruburple::HANDLE_CONVERSATIONS,
    :receiving_im_msg => Ruburple::HANDLE_CONVERSATIONS,
    :received_im_msg => Ruburple::HANDLE_CONVERSATIONS,
    :writing_chat_msg => Ruburple::HANDLE_CONVERSATIONS,
    :wrote_chat_msg => Ruburple::HANDLE_CONVERSATIONS,
    :sending_chat_msg => Ruburple::HANDLE_CONVERSATIONS,
    :sent_chat_msg => Ruburple::HANDLE_CONVERSATIONS,
    :receiving_chat_msg => Ruburple::HANDLE_CONVERSATIONS,
    :received_chat_msg => Ruburple::HANDLE_CONVERSATIONS,
    :conversation_created => Ruburple::HANDLE_CONVERSATIONS,
    :conversation_updated => Ruburple::HANDLE_CONVERSATIONS,
    :deleting_conversation => Ruburple::HANDLE_CONVERSATIONS,
    :buddy_typing => Ruburple::HANDLE_CONVERSATIONS,
    :buddy_typing_stopped => Ruburple::HANDLE_CONVERSATIONS,
    :chat_buddy_joining => Ruburple::HANDLE_CONVERSATIONS,
    :chat_buddy_joined => Ruburple::HANDLE_CONVERSATIONS,
    :chat_buddy_flags => Ruburple::HANDLE_CONVERSATIONS,
    :chat_buddy_leaving => Ruburple::HANDLE_CONVERSATIONS,
    :chat_buddy_left => Ruburple::HANDLE_CONVERSATIONS,
    :chat_inviting_user => Ruburple::HANDLE_CONVERSATIONS,
    :chat_invited_user => Ruburple::HANDLE_CONVERSATIONS,
    :chat_invited => Ruburple::HANDLE_CONVERSATIONS,
    :chat_joined => Ruburple::HANDLE_CONVERSATIONS,
    :chat_left => Ruburple::HANDLE_CONVERSATIONS,
    :chat_topic_changed => Ruburple::HANDLE_CONVERSATIONS,
    :quitting => Ruburple::HANDLE_CORE,
    :log_timestamp => Ruburple::HANDLE_LOG,
    :plugin_load => Ruburple::HANDLE_PLUGINS,
    :plugin_unload => Ruburple::HANDLE_PLUGINS,
    :savedstatus_changed => Ruburple::HANDLE_SAVEDSTATUSES,
    :file_recv_accept => Ruburple::HANDLE_XFERS,
    :file_recv_start => Ruburple::HANDLE_XFERS,
    :file_recv_cancel => Ruburple::HANDLE_XFERS,
    :file_recv_complete => Ruburple::HANDLE_XFERS,
    :file_recv_request => Ruburple::HANDLE_XFERS,
    :file_send_accept => Ruburple::HANDLE_XFERS,
    :file_send_start => Ruburple::HANDLE_XFERS,
    :file_send_cancel => Ruburple::HANDLE_XFERS,
    :file_send_complete => Ruburple::HANDLE_XFERS
  }

  def self.subscribe(event, callable = nil, &block)
    raise "No such event: #{event}" unless subsystem_handle = @@events[event]
    raise "Proc or block required" unless block_given? or callable
    callable = block if block_given?
    self._subscribe(subsystem_handle, event.to_s.gsub(/_/, "-"), callable)
  end

  def self.join
    @@event_fetcher.join
  end

  def self.init
    @@event_fetcher.kill if defined?(@@event_fetcher) && @@event_fetcher
    @@event_fetcher = Thread.new do
      loop do
        begin
          select([EVENT_INPUT], nil, nil)
          _read_event
        rescue Exception => e
          puts "#{e.class}: #{e}"
          pp e.backtrace
        end
      end
    end
    self._init
  end
  
end
