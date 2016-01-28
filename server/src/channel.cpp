#include "server.hpp"

Channel *Server::find_channel(const uint16_t channel_id) {
	if (!channel_id) return NULL;
	
	if (channels.find(channel_id) != channels.end()) {
		return channels[channel_id];
	}
	return NULL;
}

int Server::remove_user_from_channel(const uint32_t user_id, const uint16_t channel_id) {
	Channel *chan = find_channel(channel_id);
	if (chan) {
		for (uint32_t x = 0; x < chan->n_members; ++x) {
			if (chan->members[x].id == user_id) {
				char msg[1000];
				snprintf(msg, sizeof(msg), "%s left the channel.", chan->members[x].ptr->get_name());
			
				chan->members[x] = chan->members[chan->n_members-1];
				chan->n_members--;
				unsigned char remuser[1+sizeof(chat_user)];
				chat_user update;
				update.user_id = user_id;
				update.channel = channel_id;
				
				msg_channel(msg, strlen(msg)+1, 0, channel_id);
				
				remuser[0] = SERVER_MESSAGE_CHANNEL_REMOVE_USER;
				memcpy(&remuser[1], &update, sizeof(update));
				for (uint32_t y = 0; y < chan->n_members; ++y) {
					chan->members[y].ptr->send_raw(remuser, 1+sizeof(update));
				}
				return 1;
			}
		}
	}
	return 0;
}

void Server::add_user_to_channel(const uint32_t session_id, const uint16_t channel_id, const uint8_t permissions, const uint16_t warn_level, const char *voucher, const uint16_t rating) {
	Channel *chan = NULL;
	uint32_t game_id = 0;
	chan = find_channel(channel_id);
	if (chan) {
		if (session[session_id]->add_channel(channel_id)) {
			unsigned char add_msg[1000], join_msg[1000];
			char user_join_msg[1000];
			snprintf(user_join_msg, sizeof(user_join_msg), "%s joined the channel.", session[session_id]->get_name());
			const int len = strlen(user_join_msg)+1;
			for (uint32_t x = 0; x < chan->n_members; ++x) {
				chan->members[x].ptr->msg(user_join_msg, len, 0, channel_id);
			}
			
			
			join_msg[0] = SERVER_MESSAGE_CHANNEL_JOIN;
			channel_info c;
			c.channel_id = channel_id;
			c.channel_flags = chan->flags;
			strncpy(c.channel_name, chan->name, sizeof(c.channel_name));
			memcpy(&join_msg[1], &c, sizeof(c));
			session[session_id]->send_raw(join_msg, 1 + sizeof(c));
			
			add_msg[0] = SERVER_MESSAGE_CHANNEL_ADD_USER;
			user_info add_info;
			add_info.channel_id = channel_id;
			add_info.rating = rating;
			add_info.permissions = permissions;
			add_info.warn_level = warn_level;
			add_info.user_id = session[session_id]->get_user_id();
			memset(add_info.username, 0, sizeof(add_info.username));
			memset(add_info.voucher_name, 0, sizeof(add_info.voucher_name));
			strncpy(add_info.username, session[session_id]->get_name(), sizeof(add_info.username));
			strncpy(add_info.voucher_name, voucher, sizeof(add_info.voucher_name));
			
			memcpy(&add_msg[1], &add_info, sizeof(add_info));
			for (uint32_t x = 0; x < chan->n_members; ++x) {
				chan->members[x].ptr->send_raw(add_msg, 1 + sizeof(add_info));
			}
		
			
			Member *ptr = (Member*)malloc(sizeof(Member)*(chan->n_members+1));
			memcpy(ptr, chan->members, sizeof(Member)*chan->n_members);
			free(chan->members);
			chan->members = ptr;
			chan->members[chan->n_members].id = session[session_id]->get_user_id();
			chan->members[chan->n_members].ptr = session[session_id];
			chan->members[chan->n_members].permission = permissions;
			strncpy(chan->members[chan->n_members].voucher, voucher, sizeof(add_info.voucher_name));
			chan->members[chan->n_members].warn_level = warn_level;
			chan->members[chan->n_members].rating = rating;
			chan->n_members++;
			for (uint32_t x = 0; x < chan->n_members; ++x) {
				add_info.rating = chan->members[x].rating;
				add_info.permissions = chan->members[x].permission;
				add_info.warn_level = chan->members[x].warn_level;
				add_info.user_id = chan->members[x].id;
				memset(add_info.username, 0, sizeof(add_info.username));
				memset(add_info.voucher_name, 0, sizeof(add_info.voucher_name));
				strncpy(add_info.username, chan->members[x].ptr->get_name(), sizeof(add_info.username));
				strncpy(add_info.voucher_name, chan->members[x].voucher, sizeof(add_info.username));
				
				
				memcpy(&add_msg[1], &add_info, sizeof(add_info));
				session[session_id]->send_raw(add_msg, 1+sizeof(add_info));
			}
			for (uint32_t x = 0; x < chan->n_games; ++x) {
				game_state new_state;
				new_state.league_id = channel_id;
				new_state.game_id = game_id;
				memset(new_state.game_name, 0, sizeof(new_state.game_name));
				memset(new_state.host_name, 0, sizeof(new_state.host_name));

				new_state.n_users = chan->game[game_id].n_signed;
				new_state.start_time = chan->game[game_id].start_time;
				strcpy(new_state.game_name, chan->game[game_id].game_name);
				
				
				Member *m = find_member(chan->game[game_id].hoster_id, channel_id);
				
				if (m && m->ptr) strcpy(new_state.host_name, m->ptr->get_name());
				else strcpy(new_state.host_name, "Unknown"); // this results from the game being disbanded and updated.
				
				unsigned char msg[1000];
				msg[0] = SERVER_MESSAGE_CHANNEL_UPDATE_GAMEINFO;
				memcpy(&msg[1], &new_state, sizeof(new_state));
				session[session_id]->send_raw(msg, 1+sizeof(new_state));
			}
			for (uint32_t x = 0; x < chan->n_games; ++x) {
				game_state new_state;
				new_state.league_id = channel_id;
				new_state.game_id = x;
				memset(new_state.game_name, 0, sizeof(new_state.game_name));
				memset(new_state.host_name, 0, sizeof(new_state.host_name));
				new_state.n_users = chan->game[x].n_signed;
				new_state.start_time = chan->game[x].start_time;
				strcpy(new_state.game_name, chan->game[x].game_name);
				
				
				Member *m = find_member(chan->game[x].hoster_id, channel_id);
				
				if (m && m->ptr) strcpy(new_state.host_name, m->ptr->get_name());
				else strcpy(new_state.host_name, "Unknown"); // this results from the game being disbanded and updated.
				
				unsigned char msg[1000];
				msg[0] = SERVER_MESSAGE_CHANNEL_UPDATE_GAMEINFO;
				memcpy(&msg[1], &new_state, sizeof(new_state));
				session[session_id]->send_raw(msg, 1+sizeof(new_state));
			}
			mysqlpp::Query get_ratings = sql_connection.query();
			try {
				unsigned char msg[(1+sizeof(rank_state))*10];
				rank_state rank;
				get_ratings << "SELECT * FROM league_members WHERE league_id = " << channel_id << " ORDER BY rating DESC LIMIT 10";
				mysqlpp::StoreQueryResult result = get_ratings.store();
				const int size = result.num_rows();
				for (uint32_t x = 0; x < result.num_rows(); ++x) {
					mysqlpp::Query get_user = sql_connection.query();
					get_user << "SELECT * FROM users WHERE id = " << result[x]["user_id"];
					mysqlpp::StoreQueryResult user = get_user.store();
					rank.position = x;
					rank.rating = result[x]["rating"];
					rank.channel = channel_id;
					memset(rank.name, 0, sizeof(rank.name));
					if (user.num_rows()) snprintf(rank.name, sizeof(rank.name), std::string(user[0]["username"]).c_str());
					else strcpy(rank.name,"Unknown");
					msg[(1+sizeof(rank_state))*x] = SERVER_MESSAGE_CHANNEL_UPDATE_LEADERBOARDS;
					memcpy(&msg[1+((1+sizeof(rank_state))*x)], &rank, sizeof(rank));
				}
				//for (uint32_t x = 0; x < chan->n_members; ++x) {
					session[session_id]->send_raw(msg, (1+sizeof(rank))*size);
				//}
			} catch (mysqlpp::BadQuery error) {
				char buffer[2048];
				snprintf(buffer,sizeof(buffer),"SQL reported bad query: %s. Attempting reconnect...",sql_connection.error());
				puts(buffer);
			}
		}
	} 
}

uint8_t Server::get_user_active_channel_permissions(const uint32_t user_id, const uint16_t channel_id) {
	Channel *chan = find_channel(channel_id);
	if (chan) {
		for (uint32_t x = 0; x < chan->n_members; ++x) {
			if (chan->members[x].id == user_id) return chan->members[x].permission;
		}
	}
	return 0; // aka no permissions
}

uint16_t Server::search_channel(const char *name) {
	startquery:
	mysqlpp::Query query = sql_connection.query();
	query << "SELECT * FROM leagues WHERE name=\"" << name << "\" LIMIT 1";
	try {
		mysqlpp::StoreQueryResult result = query.store();
		if (!result || !result.num_rows()) {
			return 0;
		}
		return result[0]["league_id"];
	} catch (mysqlpp::BadQuery error) {
		char buffer[2048];
		snprintf(buffer,sizeof(buffer),"SQL reported bad query: %s. Attempting reconnect...",sql_connection.error());
		puts(buffer);
		try {
			sql_connection.connect(sql_db.c_str(),sql_host.c_str(),sql_user.c_str(),sql_pass.c_str(),3307);
			puts("Successfully reconnected to SQL server.");
			goto startquery;
		} catch (mysqlpp::ConnectionFailed sql_error) {
			std::string error = "SQL reconnect error: "; error += sql_error.what();
			puts(error.c_str());
			return 0;
		}
	}
}

int Server::user_channel_lookup(const uint32_t user_id, const uint16_t channel_id, uint8_t *permission_out, uint16_t *warn_level, char *voucher, uint16_t *rating) {
	startquery:
	mysqlpp::Query query = sql_connection.query();
	query << "SELECT * FROM league_members WHERE user_id=" << user_id << " AND league_id=" << channel_id << " LIMIT 1";
	try {
		mysqlpp::StoreQueryResult result = query.store();
		if (!result || !result.num_rows()) {
			if (channels[channel_id]->flags & CHANNEL_FLAG_CHANNEL_IS_INVITE_ONLY) return 0;
			else {
				// add a new user. this league doesn't require invites.
				mysqlpp::Query new_user = sql_connection.query();
				new_user << "INSERT INTO league_members (user_id, league_id, permissions, n_games, voucher) VALUES (" << user_id << ", " << channel_id << ", 5, 0, \"\")";
				try {
					new_user.exec();
					*permission_out = 3;
					*warn_level = 0;
					*rating = 1500;
					voucher[0] = '\0';
					return 1;
				} catch (mysqlpp::ConnectionFailed sql_error) {
					return 0;
				}
			}
		}
		if (permission_out) *permission_out = result[0]["permissions"];
		if (warn_level) *warn_level = result[0]["warn"];
		if (voucher) strncpy(voucher, std::string(result[0]["voucher"]).c_str(), 64);
		if (rating) *rating = result[0]["rating"];
		return 1;
	} catch (mysqlpp::BadQuery error) {
		char buffer[2048];
		snprintf(buffer,sizeof(buffer),"SQL reported bad query: %s. Attempting reconnect...",sql_connection.error());
		puts(buffer);
		try {
			sql_connection.connect(sql_db.c_str(),sql_host.c_str(),sql_user.c_str(),sql_pass.c_str(),3307);
			puts("Successfully reconnected to SQL server.");
			goto startquery;
		} catch (mysqlpp::ConnectionFailed sql_error) {
			std::string error = "SQL reconnect error: "; error += sql_error.what();
			puts(error.c_str());
			return 0;
		}
	}
}
void Server::add_channel(const char *name, const uint8_t flags, const uint16_t id, const uint32_t owner_id, const uint32_t number_of_games) {
	Channel **ptr = (Channel**)malloc(sizeof(Channel*)*(n_channels+1));
	memcpy(ptr, channel_data, sizeof(Channel*)*n_channels);
	ptr[n_channels] = (Channel*)malloc(sizeof(Channel));
	memset(ptr[n_channels]->name, 0, sizeof(ptr[n_channels]->name));
	strcpy(ptr[n_channels]->name, name);
	ptr[n_channels]->members = NULL;
	ptr[n_channels]->n_members = 0;
	ptr[n_channels]->flags = flags;
	ptr[n_channels]->game_number = number_of_games;
	ptr[n_channels]->game = NULL;
	ptr[n_channels]->n_games = 0;
	ptr[n_channels]->n_unused = 0;
	ptr[n_channels]->channel_id = id;
	channels[id] = ptr[n_channels];
	free(channel_data);
	channel_data = ptr;
	n_channels++;
}

Member *Server::find_member(const uint32_t user_id, const uint16_t channel_id) {
	Channel *chan = find_channel(channel_id);
	if (chan) {
		for (uint32_t x = 0; x < chan->n_members; ++x) {
			if (chan->members[x].id == user_id) return &chan->members[x];
		}
	}
	return NULL;
}

int Server::msg_channel(const char *msg, const uint16_t len, const uint32_t sender, const uint16_t channel) {
	Channel *chan = find_channel(channel);
	if (chan) {
		int success = 0;
		for (uint32_t x = 0; x < chan->n_members; ++x) {
			success += chan->members[x].ptr->msg(msg, len, sender, channel);
		}
		return success;
	}
	return 0;
}
