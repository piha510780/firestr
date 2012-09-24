/*
 * Copyright (C) 2012  Maxim Noah Khailo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FIRESTR_USER_USER_SERVICE_H
#define FIRESTR_USER_USER_SERVICE_H

#include "user/user.hpp"
#include "service/service.hpp"
#include "network/message_queue.hpp"
#include "network/stungun.hpp"
#include "util/thread.hpp"


#include <map>
#include <set>
#include <string>
#include <memory>

namespace fire
{
    namespace user
    {
        struct add_request
        {
            std::string to;
            std::string key;
            user_info_ptr from;
        };

        typedef std::map<std::string, add_request> add_requests;
        typedef std::set<std::string> sent_requests;

        struct user_service_context
        {
            std::string home;
            std::string host;
            std::string port;
            std::string ping_port;
            std::string stun_server;
            std::string stun_port;
            std::string greeter_server;
            std::string greeter_port;
            message::mailbox_ptr events;
            network::stun_gun_ptr stun;
        };

        class user_service : public service::service
        {
            public:
                user_service(user_service_context&);
                virtual ~user_service();

            public:
                local_user& user();
                const local_user& user() const;
                const std::string& home() const;

            public:
                void attempt_to_add_contact(const std::string& address);
                void send_confirmation(const std::string& id, std::string key = "");
                void send_rejection(const std::string& id);
                const add_requests& pending_requests() const;

            public:
                bool contact_available(const std::string& id) const;

            protected:
                virtual void message_recieved(const message::message&);

            private:
                void update_address(const std::string& address);
                void confirm_contact(user_info_ptr contact);
                void update_contact_address(const std::string& id, const std::string& adress);

            private:
                sent_requests _sent_requests;
                add_requests _pending_requests;

            private:
                local_user_ptr _user;

                std::string _home;
                std::mutex _mutex;

            private:
                void fire_new_contact_event(const std::string& id);
                void fire_contact_connected_event(const std::string& id);
                void fire_contact_disconnected_event(const std::string& id);

            private:
                //ping specific 
                typedef std::map<std::string, size_t> last_ping_map;
                typedef std::map<std::string, network::message_queue_ptr> ping_connection_map;
                void init_ping();
                void init_greet();
                void send_ping_port_requests();
                void init_ping_connection(const std::string& from_id, const std::string& ping_address);
                void send_ping_address(user::user_info_ptr, bool send_back = true);
                void send_ping(char t);

            private:
                enum state { started_greet, sent_stun, got_stun, sent_greet, sent_contact_query, done_greet, failed_greet} _state;
                std::mutex _state_mutex;

            private:
                //ping
                std::mutex _ping_mutex;
                std::string _ping_port;
                network::message_queue_ptr _ping_queue;
                util::thread_uptr _ping_thread;
                last_ping_map _last_ping;
                ping_connection_map _ping_connection;

                //greet
                network::stun_gun_ptr _stun;
                std::string _in_host;
                std::string _in_port;
                std::string _stun_server;
                std::string _stun_port;
                std::string _greeter_server;
                std::string _greeter_port;
                util::thread_uptr _greet_thread;
                network::message_queue_ptr _greet_queue;

                bool _done;

            private:
                friend void ping_thread(user_service*);
                friend void greet_thread(user_service*);
        };

        typedef std::shared_ptr<user_service> user_service_ptr;
        typedef std::weak_ptr<user_service> user_service_wptr;

        namespace event
        {
            extern const std::string NEW_CONTACT;
            extern const std::string CONTACT_CONNECTED;
            extern const std::string CONTACT_DISCONNECTED;

            struct new_contact { std::string id; };
            struct contact_connected { std::string id; };
            struct contact_disconnected { std::string id; };

            message::message convert(const new_contact&);
            void convert(const message::message&, new_contact&);

            message::message convert(const contact_connected&);
            void convert(const message::message&, contact_connected&);

            message::message convert(const contact_disconnected&);
            void convert(const message::message&, contact_disconnected&);
        }
    }
}

#endif
