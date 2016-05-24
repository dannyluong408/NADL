#include "server.hpp"
#include "nervex/nx_timing.h"

Server::Server() {
	quit = false;
	session = NULL;
	sql_connection = mysqlpp::Connection(mysqlpp::use_exceptions);
	
	// should probably load these from a file. or not who cares anyway.
	sql_user = "nel";
	sql_pass = "7f99bef877271bf7dd4aee74c0629e32";
	sql_db = "nel";
	sql_host = "localhost";
	
	n_connections = 0;
	socket4 = 0;
	
	n_channels = 0;
	channel_data = NULL;
	
	server_time = minutes(120);
}

Server::~Server() {
	while (n_connections) {
		disconnect_session(0, SERVER_MESSAGE_DISCONNECT_SERVER_SHUTDOWN);
	}
	free(session);
	sql_connection.disconnect();
	close(socket4);
	fclose(rngstream);
}

void Server::update_game(const uint16_t league_id, const uint16_t game_id) {
	game_state new_state;
	new_state.league_id = league_id;
	new_state.game_id = game_id;
	memset(new_state.game_name, 0, sizeof(new_state.game_name));
	memset(new_state.host_name, 0, sizeof(new_state.host_name));
	
	Channel *chan = find_channel(league_id);
	if (chan) {
		new_state.n_users = chan->game[game_id].n_signed;
		new_state.n_max = (chan->game_id == GAME_DOTA) ? 10 : 6;
		new_state.start_time = chan->game[game_id].start_time;
		strcpy(new_state.game_name, chan->game[game_id].game_name);
		
		
		Member *m = find_member(chan->game[game_id].hoster_id, league_id);
		
		if (m && m->ptr) strcpy(new_state.host_name, m->ptr->get_name());
		else strcpy(new_state.host_name, "Unknown"); // this results from the game being disbanded and updated.
		
		unsigned char msg[1000];
		msg[0] = SERVER_MESSAGE_CHANNEL_UPDATE_GAMEINFO;
		memcpy(&msg[1], &new_state, sizeof(new_state));
		for (int x = 0; x < chan->n_members; ++x) {
			chan->members[x].ptr->send_raw(msg, 1+sizeof(new_state));
		}
	}
}

void Server::disconnect_session(const uint32_t session_id, const uint8_t msg) {
	if (session[session_id]) {
		printf("Disconnecting session %i (%s) reason: %i\n",session_id,session[session_id]->get_name(),msg);
		
		uint16_t user_n_channels;
		uint16_t *joined_channels = session[session_id]->get_channels(&user_n_channels);
		for (int x = 0; x < user_n_channels; ++x) {
			Channel *chan = channels[joined_channels[x]];
			for (int y = 0; y < chan->n_games; ++y) {
				if (chan->game[y].n_signed && chan->game[y].hoster_id == session[session_id]->get_user_id()) {
					char disband_msg[1000];
					snprintf(disband_msg,sizeof(disband_msg),"%s closed (%s disconnected).",chan->game[y].game_name,session[session_id]->get_name());
					msg_channel(disband_msg, strlen(disband_msg)+1, 0, joined_channels[x]);
					chan->game[y].n_signed = 0; // disband the game officially
					update_game(joined_channels[x], y);
				}
				for (int z = 0; z < chan->game[y].n_signed; ++z) {
					if (chan->game[y].signed_members[z] == session[session_id]->get_user_id()) {
						chan->game[y].signed_members[z] = chan->game[y].signed_members[chan->game[y].n_signed-1];
						chan->game[y].n_signed--;
						update_game(joined_channels[x], y);
					}
				}
			}
			remove_user_from_channel(session[session_id]->get_user_id(), joined_channels[x]);
		}
		
		session[session_id]->disconnect(msg);		
		delete session[session_id];
		n_connections--;
		session[session_id] = session[n_connections];
	}
}

void binary_to_hex(const unsigned char *input, const int len, char *output) {
	/* hell yeah */
    const static char *hex[] = {
		"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B", "0C", "0D", "0E", "0F",
		"10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", "1E", "1F",
		"20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
		"30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F",
		"40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F",
		"50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
		"60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F",
		"70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E", "7F",
		"80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
		"90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F",
		"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF",
		"B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
		"C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF",
		"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF",
		"E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
		"F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"
	};
	for (int x = 0; x < len; ++x) {
		snprintf(&output[2*x], 3, hex[input[x]]);
	}
	output[len*2] = '\0';
}

int Server::init(const int argc, char **argv) {
	sockaddr_in server_addr4;
	socket4 = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
	
	server_addr4.sin_family = AF_INET;
	server_addr4.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr4.sin_port = htons(LISTENING_PORT);

	if (bind(socket4, (struct sockaddr*)&server_addr4, sizeof(server_addr4))) {
		puts("Call to bind(2) failed.");
		const int err = errno;
		if (err == EADDRINUSE) {
			puts("Error: Address already in use.");
		} else {
			printf("Errno: %i\n",err);
		}
		return -1;
	}
	if (listen(socket4, 128)) {
		const int err = errno;
		puts("Call to listen(2) failed.");
		if (err == EADDRINUSE) {
			puts("Error: Address already in use.");
		} else {
			printf("Errno: %i\n",err);
		}
		return -2;
	}
	
	FILE *sql_info = fopen("nadl.sqlinfo","r");
	if (!sql_info) return -2;
	
	char buffer[2048]; int line = 1;
	while (fgets(buffer, sizeof(buffer), sql_info)) {
		char cbuffer[2000];
		if (sscanf(buffer, "sql_user = %s", cbuffer) == 1) {
			sql_user = cbuffer;
		} else if (sscanf(buffer, "sql_pass = %s", cbuffer) == 1) {
			sql_pass = cbuffer;
		} else if (sscanf(buffer, "sql_db = %s", cbuffer) == 1) {
			sql_db = cbuffer;
		} else if (sscanf(buffer, "sql_host = %s", cbuffer) == 1) {
			sql_host = cbuffer;
		} else {
			printf("Unknown line %i/\"%s\" in sql info file.\n",line,buffer);
		}
		line++;
	}
	fclose(sql_info);
	
	try {
		sql_connection.connect(sql_db.c_str(),sql_host.c_str(),sql_user.c_str(),sql_pass.c_str(),3306);
	} catch (mysqlpp::ConnectionFailed sql_error) {
		std::string error = "SQL connection error: "; error += sql_error.what();
		puts(error.c_str());
		return -3;
	}
	
	rngstream = fopen("/dev/urandom","r");
	if (!rngstream) {
		puts("Error opening rngstream for reading.");
		return -4;
	}
	// start channels
	mysqlpp::Query query = sql_connection.query();
	query << "SELECT * FROM leagues";
	mysqlpp::StoreQueryResult result = query.store();
	for (int x = 0; x < result.num_rows(); ++x) {	
		std::string name = std::string(result[x]["name"]);
		uint8_t flags = result[x]["settings"];
		uint32_t owner = result[x]["owner"];
		uint16_t id = result[x]["league_id"];
		uint8_t game_id = result[x]["game_id"];
		uint32_t n_games;
		mysqlpp::Query game_count = sql_connection.query();
		game_count << "SELECT * FROM games WHERE league_id = " << id << " ORDER BY game_id DESC LIMIT 1";
		mysqlpp::StoreQueryResult game_count_res = game_count.store();
		if (!game_count_res.num_rows()) n_games = 0;
		else n_games = game_count_res[0]["game_id"];
		add_channel(name.c_str(), flags, id, owner, n_games, game_id);
	}
	printf("Loaded %i channels.\n",result.num_rows());
	
	puts("NEL server started.");
	return 0;
}

void Server::run() {
	const uint64_t initial_timestamp = nx_get_highres_time_us();
	while (!quit) {
		int timestep = 50; // in ms
		uint64_t next_update = server_time + minutes(1);
		int conn;
		do {
			conn = accept(socket4, (struct sockaddr*)NULL, NULL);
			if (conn >= 0) {				
				Session **ptr = (Session**)malloc(sizeof(Session*)*(n_connections+1));
				Session *new_session = new Session(conn);
				if (!ptr || !new_session) {
					// can't accept connection. 
					unsigned char msg = SERVER_MESSAGE_DISCONNECT;
					puts("Can't accept new session");
					send(conn, (void*)&msg, sizeof(msg), 0);
					free(ptr);
					free(new_session);
					close(conn);
				} else {
					server_connect_msg msg;
					msg.server_version = NETWORK_PROTOCOL_VERSION;
					msg.current_time = server_time;
					
					unsigned char buffer[1+sizeof(msg)];
					buffer[0] = SERVER_MESSAGE_CONNECT;
					memcpy(&buffer[1], &msg, sizeof(msg));
					
					send(conn, (void*)buffer, sizeof(buffer), 0);
					memcpy(ptr, session, sizeof(Session*)*n_connections);
					free(session);
					session = ptr;
					session[n_connections] = new_session;
					session[n_connections]->touch(server_time);
					n_connections++;
					puts("Accepted new connection");fflush(stdout);
				}
			}
		} while (conn > 0);
		unsigned char buffer[RECV_MAXLEN];
		for (int x = 0; x < n_connections; ++x) {
			const int len = recv(session[x]->get_sockfd(), (void*)buffer, sizeof(buffer), MSG_DONTWAIT);
			if (len > 0) {
				command(x, buffer, len);
			} else if (len == 0) {
				// socket disconnected properly
				disconnect_session(x, SERVER_MESSAGE_DISCONNECT);
			} else if (session[x]->get_last_update() < server_time - minutes(2)) {
				// there's a couple other error conditions but we're mostly not concerned with them
				disconnect_session(x, SERVER_MESSAGE_DISCONNECT_CONNECTION_TIMEOUT);
			} else if (session[x]->get_last_update() <= server_time - seconds(20)) {
				session[x]->ping(server_time);
			}
		}
		if (server_time >= next_update) {
			next_update += minutes(1);
			for (int x = 0; x < n_channels; ++x) {
				for (int y = 0; y < channel_data[x]->n_games; ++y) {
					if (channel_data[x]->game[y].start_time) {
						channel_data[x]->game[y].start_time += minutes(1);
						update_game(channel_data[x]->channel_id, y);
					}
				}
			}
		}
		// throttle updates. minutes(120) is our starting time.
		while ((server_time + timestep - minutes(120)) * 1000 >= nx_get_highres_time_us() - initial_timestamp) {
			nx_usleep(1500);
		}
		
		server_time += timestep;
	}
}

void Server::auth_user(const int session_id, user_login_msg auth_info) {
	if (auth_info.version != NETWORK_PROTOCOL_VERSION) {
		printf("client out of date. expected %i, got %i.\n",NETWORK_PROTOCOL_VERSION, auth_info.version);
		session[session_id]->disconnect(SERVER_MESSAGE_DISCONNECT_CLIENT_OUT_OF_DATE);
	} else {
		// do a bunch of silly things to prevent silly things from being done
		auth_info.username[sizeof(auth_info.username)-1] = '\0';
		startquery:
		mysqlpp::Query query = sql_connection.query();
		std::string escaped_name;
		query.escape_string(&escaped_name, auth_info.username, strlen(auth_info.username));
		query << "SELECT * FROM users WHERE username=\"" << escaped_name << "\" LIMIT 1";
		try {
			mysqlpp::StoreQueryResult result = query.store();
			if (!result || !result.num_rows()) {
				const char *errmsg = "Invalid login.";
				session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
				//disconnect_session(session_id, SERVER_MESSAGE_DISCONNECT_INVALID_LOGIN);
				return;
			} 
			
			bool success = 0;
			if (auth_info.using_token) {
				uint64_t stored_token = result[0]["token"];
				if (stored_token) { // 0 tokens are not allowed
					uint64_t token_hash[2];
					MD5((unsigned char*)auth_info.auth, 60, (unsigned char*)token_hash);
					
					if (!token_hash[0]) token_hash[0] = 1;
					success = token_hash[0] == stored_token;
				}
			} else {
				uint64_t salt = result[0]["salt"];
				uint8_t hash_output[32];
				const int r = libscrypt_scrypt((uint8_t*)auth_info.auth, sizeof(auth_info.auth), (uint8_t*)&salt, sizeof(salt), 16 * 1024, 8, 1, hash_output, sizeof(hash_output));
				const unsigned char *hash_sql = reinterpret_cast<const unsigned char*>(result[0]["password_hash"].data());
				
				char sql_hash_as_hex[65], input_password_hash_as_hex[65];
				binary_to_hex(hash_output, sizeof(hash_output), input_password_hash_as_hex);
				binary_to_hex(hash_sql, sizeof(hash_output), sql_hash_as_hex);
				
				success = (strcmp(sql_hash_as_hex, input_password_hash_as_hex) == 0);
			}
			if (success) {
				unsigned char data[1024];
				data[0] = SERVER_MESSAGE_AUTH_SUCCESSFUL;
				data[1] = SERVER_MESSAGE_USER_INFO;
				user_info info;
				info.permissions = result[0]["global_permissions"];
				info.user_id = result[0]["id"];
				info.channel_id = 0;
				info.rating = 0;
				info.warn_level = 0;
				std::string username = std::string(result[0]["username"]);
				std::string voucher_name = "";
				memset(info.username, 0, sizeof(info.username));
				memset(info.voucher_name, 0, sizeof(info.voucher_name));
				strncpy(info.username, username.c_str(), sizeof(info.username));
				strncpy(info.voucher_name, voucher_name.c_str(), sizeof(info.voucher_name));
				memcpy(&data[2], &info, sizeof(info));
				send(session[session_id]->get_sockfd(), (void*)data, 2 + sizeof(info), 0);
				printf("User %s/%u successful login.\n",username.c_str(),info.user_id);

				// disconnect and old sessions using this user
				for (int x = 0; x < n_connections; ++x) {
					if (session[x]->get_user_id() == info.user_id) {
						/*
						uint16_t n_channels;
						// removing a user from the channel doesn't modify the channel ptr, so this is usable until we're done.
						uint16_t *channels = session[x]->get_channels(&n_channels);
						while (n_channels) {
							n_channels--;
							const uint16_t channel = channels[n_channels];
							remove_user_from_channel(session[session_id]->get_user_id(), channel);
							session[x]->remove_channel(channel);
						}*/
						disconnect_session(x, SERVER_MESSAGE_DISCONNECT_USER_LOGGED_IN_ELSEWHERE);
						x = n_connections;
					}
				}
				session[session_id]->auth(username.c_str(), info.user_id, info.permissions);
				session[session_id]->touch(server_time);
				char login_msg[1000];
				snprintf(login_msg,sizeof(login_msg),"Successfully logged in as %s.", session[session_id]->get_name());
				session[session_id]->server_msg(login_msg, strlen(login_msg)+1);
				
				for (int x = 0; x < n_channels; ++x) {
					const int n_games = channel_data[x]->n_games;
					for (int y = 0; y < n_games; ++y) {
						const int n_signed = channel_data[x]->game[y].n_signed;
						for (int z = 0; z < n_signed; ++z) {
							if (channel_data[x]->game[y].signed_members[z] == info.user_id) {
								session[session_id]->sign();
								z = n_signed;
								y = n_games;
								x = n_channels;
							}
						}
					}
				}
				
				mysqlpp::Query channel_query = sql_connection.query();
				channel_query << "SELECT * FROM league_members WHERE user_id=" << session[session_id]->get_user_id() << " ORDER BY league_id ASC";
				try {
					mysqlpp::StoreQueryResult channel_result = channel_query.store();
					for (int x = 0; x < channel_result.num_rows(); ++x) {
						const uint8_t permission = channel_result[x]["permissions"];
						const uint16_t warn_level = channel_result[x]["warn"];
						const uint16_t rating = channel_result[x]["rating"];
						const uint16_t channel_id = channel_result[x]["league_id"];
						char voucher[64];
						snprintf(voucher, sizeof(voucher), std::string(channel_result[x]["voucher"]).c_str());
						if (permission >= CHANNEL_JOIN_PERMISSIONS) {
							add_user_to_channel(session_id, channel_id, permission, warn_level, voucher, rating);
						}
					}
				} catch (mysqlpp::BadQuery error) {
					// we don't care about this really.
				}
			} else {
				//disconnect_session(session_id, SERVER_MESSAGE_DISCONNECT_INVALID_LOGIN);
				if (auth_info.using_token) {
					unsigned char msg = SERVER_MESSAGE_BAD_AUTH_TOKEN;
					session[session_id]->send_raw(&msg, 1);
				} else {
					const char *errmsg = "Invalid password.";
					session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
				}
				puts("failed login");
			}
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
				return;
			}
		}	
	}
}

void Server::interpret_message(const int session_id, const chat_message_header header, char *message) {
	//printf("Interpreting message %i bytes long: \n%s\n",strlen(message),message);fflush(stdout);
	int len = strlen(message);
	if (len == 0) return;
	// sanitize the string
	char previous_char = 0;
	for (int x = 0; x < len; ++x) {
		if (message[x] < 32 || (message[x] == ' ' && previous_char == ' ')) {
			// shift the entire array over by one. @len does not include the null-terminator.
			for (int y = x; y < len; ++y) {
				message[y] = message[y+1];
			}
			len--;
		}
		previous_char = message[x];
	}

	// this is a command.
	if (len > 2 && ((message[0] == '.' && message[1] != '.') || message[0] == '/' || message[0] == '-' || message[0] == '!')) {
		message[0] = '/';
		int count = 1;
		for (int x = 0; x < len; ++x) {
			if (message[x] == ' ') count++;
		}
		char *data = (char*)malloc(len+1);
		char **arg = (char**)malloc(sizeof(char*)*(count));
		strcpy(data, message);
		
		int last = 0;
		count = 0;
		for (int x = 0; x < len; ++x) {
			if (data[x] == ' ') {
				data[x] = '\0';
				arg[count] = &data[last];
				last = x + 1;
				count++;
			}
		}
		arg[count] = &data[last];
		count++; // change from space count to word count
		
		// Generic commands
		if (strcmp("/passwd",arg[0]) == 0 || strcmp("/changepw",arg[0]) == 0) {
			if (session[session_id]->get_user_id()) {
				if (count == 3) {
					if (strcmp(arg[1], arg[2]) == 0) {
						const int str_len = strlen(arg[1]);
						if (str_len > 63 || str_len < 6) {
							const char *errmsg = "passwd: Passwords must be between 6 and 63 characters long.";
							session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
						} else {
							char padded_password[64];
							memset(padded_password, 0, sizeof(padded_password));
							strcpy(padded_password, arg[2]);
							
							uint64_t salt;
							get_random((void*)&salt, sizeof(salt));
							unsigned char hash_output[32];
							char hash_as_hex[65];
							const int r = libscrypt_scrypt((uint8_t*)padded_password, sizeof(padded_password), (uint8_t*)&salt, sizeof(salt), 16 * 1024, 8, 1, hash_output, sizeof(hash_output));
							if (r != -1) {
								binary_to_hex(hash_output, 32, hash_as_hex);
								
								passwd_startquery:
								mysqlpp::Query query = sql_connection.query();
								std::string escaped_name;
								query.escape_string(&escaped_name, session[session_id]->get_name(), strlen(session[session_id]->get_name()));
								query << "UPDATE users SET password_hash=UNHEX('" << hash_as_hex << "'), salt=" << salt << ", token=0 WHERE username='" << escaped_name << "'";
								try {
									query.exec();
									const char *errmsg = "Password successfully updated.";
									session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
								}  catch (mysqlpp::BadQuery error) {
									char buffer[2048];
									snprintf(buffer,sizeof(buffer),"SQL reported bad query: %s. Attempting reconnect...",sql_connection.error());
									puts(buffer);
									try {
										sql_connection.connect(sql_db.c_str(),sql_host.c_str(),sql_user.c_str(),sql_pass.c_str(),3307);
										puts("Successfully reconnected to SQL server.");
										goto passwd_startquery;
									} catch (mysqlpp::ConnectionFailed sql_error) {
										std::string error = "SQL reconnect error: "; error += sql_error.what();
										puts(error.c_str());
										
										const char *errmsg = "passwd: Error updating password. Please try again later.";
										session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
									}
								}
							} else {
								const char *errmsg = "passwd: unspecified error";
								session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
							}
						}
					} else {
						const char *errmsg = "passwd: Passwords don't match.";
						session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
					}
				} else {
					const char *errmsg1 = "passwd: Invalid syntax";
					const char *errmsg2 = "Usage: /passwd (newpass) (same password)";
					session[session_id]->server_msg(errmsg1,strlen(errmsg1)+1);
					session[session_id]->server_msg(errmsg2,strlen(errmsg2)+1);
				}
			} else {
				const char *errmsg = "passwd: You need to login first!";
				session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
			}
		} else if (strcmp("/resetpw", arg[0]) == 0) {
			if (session[session_id]->get_permissions() >= PERMISSION_ADMINISTRATOR) {
				if (count == 2) {
					uint32_t new_password;
					get_random((void*)&new_password, sizeof(new_password));
					char password[33];
					binary_to_hex((unsigned char*)&new_password, sizeof(new_password), password);
					char padded_password[64];
					memset(padded_password, 0, sizeof(padded_password));
					strcpy(padded_password, password);

					uint64_t salt;
					get_random((void*)&salt, sizeof(salt));

					unsigned char hash_output[32];
					char hash_as_hex[65];
					const int r = libscrypt_scrypt((uint8_t*)padded_password, sizeof(padded_password), (uint8_t*)&salt, sizeof(salt), 16 * 1024, 8, 1, hash_output, sizeof(hash_output));
					if (r != -1) {
						binary_to_hex(hash_output, 32, hash_as_hex);
						
						mysqlpp::Query query = sql_connection.query();
						std::string escaped_name;
						query.escape_string(&escaped_name, arg[1], strlen(arg[1]));
						query << "UPDATE users SET password_hash = UNHEX('" << hash_as_hex << "'), salt = " << salt << ", token = 0 WHERE username = '" << escaped_name << "' LIMIT 1";
						try {
							query.exec();
							if (query.affected_rows()) {
								char msgbuffer[1024];
								snprintf(msgbuffer,sizeof(msgbuffer),"resetpw: User %s reset. New password: %s",arg[1],password);
								session[session_id]->server_msg(msgbuffer,strlen(msgbuffer)+1);
							} else {
								char msgbuffer[1024];
								snprintf(msgbuffer,sizeof(msgbuffer),"resetpw: User %s now found.",arg[1]);
								session[session_id]->server_msg(msgbuffer,strlen(msgbuffer)+1);
							}
						} catch (mysqlpp::BadQuery error) {
							const char *errmsg = "resetpw: SQL error resetting password.";
							session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
						}
					} else {
						const char *errmsg = "resetpw: libscrypt_scrypt() error.";
						session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
					}
				} else {
					const char *errmsg = "resetpw: Invalid syntax.";
					const char *errmsg2 = "Usage: /resetpw (username)";
					session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
					session[session_id]->server_msg(errmsg2,strlen(errmsg2)+1);
				}
			} else {
				const char *errmsg = "resetpw: Permission denied.";
				session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
			}
		} else if (strcmp("/adduser", arg[0]) == 0 || strcmp("/au",arg[0]) == 0) {
			if (session[session_id]->get_permissions() >= PERMISSION_ADMINISTRATOR) {
				if (count == 2) {
					// check for proper name
					int name_len = strlen(arg[1]);
					bool valid_name = true;
					for (int x = 0; x < name_len; ++x) {
						// there's actually 100% a better way to do this probably.
						if (arg[1][x] == '?' || arg[1][x] == '.' || arg[1][x] == '!' || arg[1][x] == ',' 
							|| arg[1][x] == '[' || arg[1][x] == ']' || arg[1][x] == '(' || arg[1][x] == ')' 
							|| arg[1][x] == '<' || arg[1][x] == '>' || arg[1][x] == '\'' || arg[1][x] == '=' 
							|| arg[1][x] == '+' || arg[1][x] == '\\' || arg[1][x] == '|' || arg[1][x] == ';'
							|| arg[1][x] == ':' || arg[1][x] == '\"' || arg[1][x] == '@' || arg[1][x] == '#'
							|| arg[1][x] == '$' || arg[1][x] == '^' || arg[1][x] == '&' || arg[1][x] == '*'
							|| arg[1][0] > 'z') {
								valid_name = false;
						}
					}
					if (arg[1][0] == '/' || arg[1][0] == '-' || arg[1][0] == '~') {
						valid_name = false;
					}
					
					if (!valid_name) {
						char errmsg[800];
						snprintf(errmsg, sizeof(errmsg), "adduser: Name '%s' contains invalid characters", arg[1]);
						session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
					} else {
						printf("Admin: %s->adduser(%s)\n",session[session_id]->get_name(),arg[1]);
						
						uint32_t new_password;
						get_random((void*)&new_password, sizeof(new_password));
						char password[33];
						binary_to_hex((unsigned char*)&new_password, sizeof(new_password), password);
						char padded_password[64];
						memset(padded_password, 0, sizeof(padded_password));
						strcpy(padded_password, password);

						uint64_t salt;
						get_random((void*)&salt, sizeof(salt));

						unsigned char hash_output[32];
						char hash_as_hex[65];
						const int r = libscrypt_scrypt((uint8_t*)padded_password, sizeof(padded_password), (uint8_t*)&salt, sizeof(salt), 16 * 1024, 8, 1, hash_output, sizeof(hash_output));
						if (r != -1) {
							binary_to_hex(hash_output, 32, hash_as_hex);
							
							
							adduser_startquery:
							mysqlpp::Query query = sql_connection.query();
							std::string escaped_name;
							query.escape_string(&escaped_name, arg[1], strlen(arg[1]));
							query << "INSERT INTO users (username, password_hash, global_permissions, salt) VALUES ('"
								<< escaped_name << "', UNHEX('" << hash_as_hex << "'), 0, " << salt << ")";
							
							try {
								query.exec();
								char msgbuffer[1024];
								snprintf(msgbuffer,sizeof(msgbuffer),"adduser: User %s added. Password: %s",arg[1],password);
								session[session_id]->server_msg(msgbuffer,strlen(msgbuffer)+1);
							} catch (mysqlpp::BadQuery error) {
								if (error.errnum() == 1062) { // Error: 1062 SQLSTATE: 23000 (ER_DUP_ENTRY)
									char errmsg[800];
									snprintf(errmsg, sizeof(errmsg), "adduser: User %s already exists.",arg[1]);
									session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
								} else {
									char buffer[2048];
									snprintf(buffer,sizeof(buffer),"SQL reported bad query: %s. Attempting reconnect...",sql_connection.error());
									puts(buffer);
									try {
										sql_connection.connect(sql_db.c_str(),sql_host.c_str(),sql_user.c_str(),sql_pass.c_str(),3307);
										puts("Successfully reconnected to SQL server.");
										goto adduser_startquery;
									} catch (mysqlpp::ConnectionFailed sql_error) {
										std::string error = "SQL reconnect error: "; error += sql_error.what();
										puts(error.c_str());
										
										const char *errmsg = "adduser: Error adding user. Please try again later.";
										session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
									}
								}
							}
						} else {
							const char *errmsg = "adduser: libscrypt_scrypt() error";
							session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
						}
					}
				} else {
					const char *errmsg1 = "adduser: Invalid syntax";
					const char *errmsg2 = "Usage: /adduser (username)";
					session[session_id]->server_msg(errmsg1,strlen(errmsg1)+1);
					session[session_id]->server_msg(errmsg2,strlen(errmsg2)+1);
				}
			} else {
				const char *errmsg = "adduser: Permission denied";
				session[session_id]->server_msg(errmsg,sizeof(errmsg));
			}
		} else if (strcmp("/msg", arg[0]) == 0 || strcmp("/message", arg[0]) == 0 || strcmp("/whisper", arg[0]) == 0 || strcmp("/tell", arg[0]) == 0 || strcmp("/pm", arg[0]) == 0) {
			if (session[session_id]->get_user_id()) {
				if (count > 2) {
					bool found = false;
					const int len = strlen(arg[1]);
					if (len < 64 && len > 0) {
						char lowerarg[64];
						lower(arg[1], lowerarg);
						for (int x = 0; x < n_connections; ++x) {
							if (session[x]) {
								char lowername[64]; 
								lower(session[x]->get_name(), lowername);
								if (strcmp(lowername,lowerarg) == 0) {
									found = true;
									char return_msg[2000], whisper_msg[2000];
									int pos = arg[2] - arg[0];
									snprintf(return_msg, sizeof(return_msg), "<span style=\"color:#004e8b\">To %s: %s</span>", session[x]->get_name(), &message[pos]);
									session[session_id]->msg(return_msg, strlen(return_msg), session[session_id]->get_user_id(), UINT16_MAX);
									
									snprintf(whisper_msg, sizeof(whisper_msg), "<span style=\"color:#004e8b\">From %s: %s</span>", session[session_id]->get_name(), &message[pos]);
									session[x]->msg(whisper_msg, strlen(whisper_msg), session[session_id]->get_user_id(), UINT16_MAX);
								}
							}
						}
					}
					if (!found) {
						char errmsg[800];
						snprintf(errmsg, sizeof(errmsg), "msg: Couldn't find user '%s'.",arg[1]);
						session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
					} 
				} else {
					const char *errmsg1 = "message: Invalid syntax";
					const char *errmsg2 = "Usage: /message (recipient) (message...)";
					session[session_id]->server_msg(errmsg1,strlen(errmsg1)+1);
					session[session_id]->server_msg(errmsg2,strlen(errmsg2)+1);
				}
			} else {
				const char *errmsg = "message: You need to login first!";
				session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
			}
		// Channel-specific commands
		} else if (strcmp("/sg",arg[0]) == 0 || strcmp("/startgame",arg[0]) == 0) {
			uint8_t permission = 0;
			const int success = user_channel_lookup(session[session_id]->get_user_id(), header.channel, &permission, NULL, NULL, NULL);
			int found = -1;
			if (success && permission >= CHANNEL_PLAYER_PERMISSIONS) {
				Channel *chan = find_channel(header.channel);
				if (chan) {
					for (int x = 0; x < chan->n_games; ++x) {
						const int n_signed = chan->game[x].n_signed;
						for (int y = 0; y < n_signed; ++y) {
							if (chan->game[x].signed_members[y] == session[session_id]->get_user_id()) {
								const uint32_t n_players = (chan->game_id == GAME_DOTA) ? 10 : 6;
								if (chan->game[x].hoster_id == session[session_id]->get_user_id()) {
									if (chan->game[x].n_signed == n_players) {
										// start the game
										chan->game[x].start_time = 1;
										// convert session ids to channel member ids
										Member **member_arr = (Member**)malloc(sizeof(Member*)*(n_players*2));
										int success = 1;
										for (int z = 0; z < n_players; ++z) {
											member_arr[z] = find_member(chan->game[x].signed_members[z], header.channel);
											// need 2 copies
											member_arr[z+n_players] = member_arr[z];
											if (!member_arr[z]) success = false;
										}
										if (success) {
											update_game(header.channel, x);
											int best_diff = 100000;
											int best_pos = 0;
											int ratings[2];
											// here we go
											for (uint8_t z = 0; z < n_players; ++z) {
												int team_radiant = 0, team_dire = 0;
												for (uint8_t i = 0; i < n_players/2; ++i) {
													team_radiant += member_arr[z+i]->rating;
													team_dire += member_arr[z+(n_players/2)+i]->rating;
												}
												const int diff = abs(team_radiant - team_dire);
												if (diff < best_diff) {
													best_diff = diff;
													best_pos = z;
													ratings[0] = team_radiant;
													ratings[1] = team_dire;
												}
												chan->game[x].results[z] = 0;
												chan->game[x].truant[z] = 0;
											}
											int radiant_captain, dire_captain;
											int radiant_rating = 0, dire_rating = 0;
											for (int i = 0; i < n_players/2; ++i) {
												if (member_arr[best_pos+i]->rating > radiant_rating) {
													radiant_rating = member_arr[best_pos+i]->rating;
													radiant_captain = i;
												}
												if (member_arr[best_pos+i+(n_players/2)]->rating > dire_rating) {
													dire_rating = member_arr[best_pos+i+5]->rating;
													dire_captain = i+(n_players/2);
												}
											}
											Member *temp = member_arr[best_pos];
											member_arr[best_pos] = member_arr[best_pos+radiant_captain];
											member_arr[best_pos+radiant_captain] = temp;
											
											temp = member_arr[best_pos+(n_players/2)];
											member_arr[best_pos+(n_players/2)] = member_arr[best_pos+dire_captain];
											member_arr[best_pos+dire_captain] = temp;
											
											chan->game[x].rating_change = (ratings[0] - ratings[1]) / 50;
											
											for (int y = 0; y < n_players; ++y) { 
												chan->game[x].results[y] = 0;
												chan->game[x].truant[y] = 0;
											}
											
											char msg[1000];
											if (chan->game_id == GAME_DOTA) {
												snprintf(msg, sizeof(msg), "%s started.", chan->game[x].game_name);
												msg_channel_as_admin(msg, strlen(msg)+1, 0, header.channel);
												snprintf(msg, sizeof(msg), "Radiant: %s (c), %s, %s, %s, %s (%i) - Dire: %s (c), %s, %s, %s, %s (%i)",
													member_arr[best_pos]->ptr->get_name(), member_arr[best_pos+1]->ptr->get_name(), member_arr[best_pos+2]->ptr->get_name(), member_arr[best_pos+3]->ptr->get_name(), member_arr[best_pos+4]->ptr->get_name(), ratings[0],
													member_arr[best_pos+5]->ptr->get_name(), member_arr[best_pos+6]->ptr->get_name(), member_arr[best_pos+7]->ptr->get_name(), member_arr[best_pos+8]->ptr->get_name(), member_arr[best_pos+9]->ptr->get_name(), ratings[1]);
												msg_channel_as_admin(msg, strlen(msg)+1, 0, header.channel);
												
												chan->game[x].teams_str = (char*)malloc(strlen(msg)+1);
												memcpy(chan->game[x].teams_str, msg, strlen(msg)+1);
												
												uint16_t random;
												get_random(&random, sizeof(random));
												// 100 - 999 - always 3 chars long
												snprintf(msg, sizeof(msg), "Password is \"nadl%i\" (all lowercase)",random%900 + 100);
												msg_channel(msg, strlen(msg)+1, 0, header.channel);
												found = -2;
											} else if (chan->game_id == GAME_BLC) {
												snprintf(msg, sizeof(msg), "%s started.", chan->game[x].game_name);
												msg_channel_as_admin(msg, strlen(msg)+1, 0, header.channel);
												snprintf(msg, sizeof(msg), "Warm: %s*, %s, %s, (%i) - Cold: %s*, %s, %s (%i)",
													member_arr[best_pos]->ptr->get_name(), member_arr[best_pos+1]->ptr->get_name(), member_arr[best_pos+2]->ptr->get_name(), ratings[0],
													member_arr[best_pos+3]->ptr->get_name(), member_arr[best_pos+4]->ptr->get_name(), member_arr[best_pos+5]->ptr->get_name(), ratings[1]);
												msg_channel_as_admin(msg, strlen(msg)+1, 0, header.channel);
												chan->game[x].teams_str = (char*)malloc(strlen(msg)+1);
												memcpy(chan->game[x].teams_str, msg, strlen(msg)+1);
												
												found = -2;
											}
											
											free(member_arr);
										} else {
											char errmsg[200];
											snprintf(errmsg, sizeof(errmsg), "startgame error on game %s. - closing game.", chan->game[x].game_name);
											msg_channel_as_admin(errmsg, strlen(errmsg)+1, 0, header.channel);
											chan->game[x].n_signed = 0;
											update_game(header.channel, x);
										}
									} else {
										
										char errmsg[200];
										snprintf(errmsg, sizeof(errmsg), "Can't start game until %i members have signed.", n_players);
										session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
										found = -2;
									}
								}
								y = n_signed;
								x = chan->n_games;
							}
						}
					} 
					if (found == -1 && !session[session_id]->sign_state()) {
						// make new game
						// search for an empty game
						for (int x = 0; x < chan->n_games; ++x) {
							if (chan->game[x].n_signed == 0) {
								found = x;
								x = chan->n_games;
							}
						} 
						// allocate a new game if we need to
						if (found == -1) {
							Game *ptr = (Game*)malloc(sizeof(Game)*(chan->n_games+1));
							memcpy(ptr, chan->game, sizeof(Game)*chan->n_games);
							free(chan->game);
							chan->game = ptr;
							found = chan->n_games;
							chan->n_games++;
						}
						for (int x = 0; x < 10; ++x) {
							chan->game[found].signed_members[x] = 0;
							chan->game[found].results[x] = 0;
							chan->game[found].truant[x] = 0;
						}
						chan->game[found].teams_str = NULL;
						chan->game[found].start_time = 0;
						chan->game[found].hoster_id = session[session_id]->get_user_id();
						chan->game[found].n_signed = 1;
						chan->game[found].signed_members[0] = session[session_id]->get_user_id();
						chan->game[found].n_forbid = 0;
						chan->game[found].forbid_list = NULL;
						if (chan->n_unused) {
							chan->n_unused--;
							chan->game[found].game_id = chan->unused[chan->n_unused];
						} else {
							chan->game[found].game_id = chan->game_number;
							chan->game_number++;
						}
						snprintf(chan->game[found].game_name, 64, "%s-%i", chan->name, chan->game[found].game_id);
						
						update_game(header.channel, found);
						
						session[session_id]->sign();
						char start_msg[1000];
						snprintf(start_msg, sizeof(start_msg), "%s has started a new game. \"/sign %s\" to join.", session[session_id]->get_name(), chan->game[found].game_name);
						msg_channel_as_admin(start_msg, strlen(start_msg)+1, 0, header.channel);
						puts("New game started");
					} else if (found != -2) {
						const char *errmsg = "sg: You're already signed to a game!";
						session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
					}
				} else {
					const char *errmsg = "Only vouched (non trial) players can start new games.";
					session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
				}
			} else {
				const char *errmsg = "Unknown channel";
				session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
			}
		} else if (strcmp("/stats",arg[0]) == 0) {
			if (session[session_id]->get_user_id()) {
				Channel *chan = find_channel(header.channel);
				if (chan) {
					char user[64];
					if (count == 1) {
						strcpy(user, session[session_id]->get_name());
					} else {
						strncpy(user, arg[1], sizeof(user));
						user[sizeof(user)-1] = '\0';
					}
					char escaped_name[129];
					mysqlpp::Query name_query = sql_connection.query();
					name_query.escape_string(escaped_name, user, strlen(user));
					name_query << "SELECT * FROM users WHERE username = '" << escaped_name << "' LIMIT 1";
					try {
						mysqlpp::StoreQueryResult name_result = name_query.store();
						if (!name_result.num_rows()) {
							char errmsg[1000];
							snprintf(errmsg, sizeof(errmsg), "stats: Couldn't find user '%s'.", user);
							session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
						} else {
							mysqlpp::Query games_query = sql_connection.query();
							games_query << "SELECT * FROM game_members WHERE (league_id = " << header.channel << " AND user_id = " << name_result[0]["id"] << ")";
							mysqlpp::StoreQueryResult result = games_query.store();
							uint32_t wins = 0;
							const uint32_t games = result.num_rows();
							for (uint32_t x = 0; x < games; ++x) {
								const uint32_t team = result[x]["team"];
								const uint32_t game_result = result[x]["result"];
								if (game_result == team) wins++;
							}
							char msg[1000];
							if (games) snprintf(msg, sizeof(msg), "%s stats: %u wins/%u losses (%.2f%%)",std::string(name_result[0]["username"]).c_str(), wins, games-wins, 100.f * ((float)wins / (float)games));
							else snprintf(msg, sizeof(msg), "stats: %s has no games played this season.",std::string(name_result[0]["username"]).c_str());
							session[session_id]->msg(msg, strlen(msg)+1, 0, header.channel);
						}
					} catch (mysqlpp::BadQuery error) {
						const char *errmsg = "stats: Lookup error. Try again later.";
						session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
						std::string err = "SQL  error: "; err += error.what();
						puts(err.c_str());
									
					}
				} else {
					const char *errmsg = "stats: Unknown channel.";
					session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
				}
			} else {
				const char *errmsg = "stats: You must be logged in to use this command.";
				session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
			}
		} else if (strcmp("/compare",arg[0]) == 0) {
			if (session[session_id]->get_user_id()) {
				char user1[64];
				char user2[64];
				if (count == 3) {
					strncpy(user1, arg[1], 63);
					strncpy(user2, arg[2], 63);
				} else {
					strncpy(user1, session[session_id]->get_name(), 63);
					strncpy(user2, arg[1], 63);
				}
				char escaped_user1[129], escaped_user2[129];
				mysqlpp::Query name_query = sql_connection.query();
				name_query.escape_string(escaped_user1, user1, strlen(user1));
				name_query.escape_string(escaped_user2, user2, strlen(user2));
				
				name_query << "SELECT * FROM users WHERE username = '" << escaped_user1 << "' OR username = '" << escaped_user2 << "'";
				try {
					mysqlpp::StoreQueryResult name_result = name_query.store();
					if (name_result.num_rows() == 2) {
						const uint32_t id1 = name_result[0]["id"];
						const uint32_t id2 = name_result[1]["id"];
						if (id1 != id2) {
							mysqlpp::Query game_query = sql_connection.query();
							game_query << "SELECT * FROM game_members WHERE (user_id = " << name_result[0]["id"] << " OR user_id = " << name_result[1]["id"] << ") AND league_id = " << header.channel;
							mysqlpp::StoreQueryResult game_result = game_query.store();
							int wins1 = 0, wins2 = 0, games1 = 0, games2 = 0;
							int wins_together = 0, games_together = 0;
							int wins_against = 0, games_against = 0;
							for (int x = 0; x < game_result.num_rows(); ++x) {
								const uint32_t selected_id = game_result[x]["user_id"];
								if (selected_id == id1) {
									const int game_id = game_result[x]["game_id"];
									games1++;
									if (game_result[x]["result"] == game_result[x]["team"]) wins1++;
									for (int y = 0; y < game_result.num_rows(); ++y) {
										const uint32_t current_game_id = game_result[y]["game_id"];
										const uint32_t current_user_id = game_result[y]["user_id"];
										if (current_game_id == game_id && current_user_id == id2) {
											if (game_result[y]["team"] == game_result[x]["team"]) {
												games_together++;
												if (game_result[y]["team"] == game_result[y]["result"]) {
													wins_together++;
												} 
											} else {
												games_against++;
												if (game_result[x]["team"] == game_result[x]["result"]) {
													wins_against++;
												} 
											}
										}
									}
								}
							}
							// 2nd pass - count other player total wins/losses
							for (int x = 0; x < game_result.num_rows(); ++x) {
								const uint32_t current_user_id = game_result[x]["user_id"];
								if (current_user_id == id2) {
									games2++;
									if (game_result[x]["result"] == game_result[x]["team"]) {
										wins2++;
									}
								}
							}
							char msg[1000];
							snprintf(msg, sizeof(msg), "Compare: %s (%i/%i) vs %s (%i/%i) - %i/%i together, %i/%i against.",
								std::string(name_result[0]["username"]).c_str(), wins1,games1-wins1, 
								std::string(name_result[1]["username"]).c_str(), wins2,games2-wins2,
								wins_together, games_together-wins_together, wins_against, games_against-wins_against);
							session[session_id]->msg(msg, strlen(msg)+1, 0, header.channel);
						} else {
							const char *errmsg = "compare: Can't compare the same user.";
							session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
						}
					} else {
						if (!name_result.num_rows()) {
							const char *errmsg = "compare: Couldn't find users.";
							session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
						} else {
							char errmsg[1000];
							char lower_result[100], lower_name1[100], lower_name2[100];
							lower(std::string(name_result[0]["username"]).c_str(), lower_result);
							lower(user1, lower_name1);
							lower(user2, lower_name2);
							
							
							if (strcmp(lower_result,lower_name1) != 0) {
								snprintf(errmsg, sizeof(errmsg), "compare: Couldn't find user '%s'", user1);
							} else if (strcmp(lower_result,lower_name2) != 0) {
								snprintf(errmsg, sizeof(errmsg), "compare: Couldn't find user '%s'", user2);
							} else {
								snprintf(errmsg, sizeof(errmsg), "compare: Usernames must be different.");
							}
							session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
						}
					}
				} catch (mysqlpp::BadQuery error) {
					const char *errmsg = "Error looking up users.";
					std::cout << sql_connection.error() << std::endl;
					session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
				}
			} else {
				const char *errmsg = "compare: You must be signed in and in a channel to use this command.";
				session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
			}
		} else if (strcmp("/forbid",arg[0]) == 0) {
			if (count == 2) {
				if (session[session_id]->sign_state()) {
					Channel *chan = find_channel(header.channel);
					const uint32_t user_id = session[session_id]->get_user_id();
					if (chan) {
						int found = -1;
						for (int x = 0; x < chan->n_games; ++x) {
							const int n_signed = chan->game[x].n_signed;
							for (int y = 0; y < n_signed; ++y) {
								if (chan->game[x].signed_members[y] == user_id) {
									found = x;
									y = n_signed;
									x = chan->n_games;
								}
							}
						}
						if (found != -1) {
							if (chan->game[found].hoster_id != user_id) {
								const char *errmsg = "forbid: Only the host can forbid players.";
								session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
								printf("hoster id %u, user id %u\n",chan->game[found].hoster_id, user_id);fflush(stdout);
							} else {
								if (chan->game[found].start_time) {
									const char *errmsg = "forbid: Can't forbid players after the game has started.";
									session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
								} else {
									// find the target user id
									std::string escaped_name;
									mysqlpp::Query query = sql_connection.query();
									query.escape_string(&escaped_name, arg[1], strlen(arg[1]));
									query << "SELECT * FROM users WHERE username = '" << escaped_name << "' LIMIT 1";
									try {
										mysqlpp::StoreQueryResult result = query.store();
										if (!result.num_rows()) {
											char errmsg[1000];
											snprintf(errmsg, sizeof(errmsg), "forbid: Error looking up user '%s'.", arg[1]);
											session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
										} else {
											const uint32_t found_user = result[0]["id"];
											if (found_user != user_id) {
												char msg[1000];
												snprintf(msg, sizeof(msg), "%s has forbidden %s from the channel.",session[session_id]->get_name(),std::string(result[0]["username"]).c_str());
												msg_channel(msg, strlen(msg)+1, 0, header.channel);
												for (int x = 0; x < chan->game[found].n_signed; ++x) {
													if (chan->game[found].signed_members[x] == found_user) {
														for (int y = 0; y < n_connections; ++y) {
															if (session[y]->get_user_id() == found_user) {
																out_game(y, header.channel, false);
															}
														}
													}
												}
												uint32_t *ptr = (uint32_t*)malloc(sizeof(uint32_t)*(chan->game[found].n_forbid+1));
												memcpy(ptr, chan->game[found].forbid_list, sizeof(uint32_t)*(chan->game[found].n_forbid));
												ptr[chan->game[found].n_forbid] = found_user;
												free(chan->game[found].forbid_list);
												chan->game[found].forbid_list = ptr;
												chan->game[found].n_forbid++;
											} else {
												const char *errmsg = "forbid: Can't forbid yourself!";
												session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
											}
										}
									} catch (mysqlpp::BadQuery error) {
										char errmsg[1000];
										snprintf(errmsg, sizeof(errmsg), "forbid: Error looking up user '%s'.", arg[1]);
										session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
									}
								}
							}
						}
					} else {
						const char *errmsg = "forbid: Invalid channel.";
						session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
					}
				} else {
					const char *errmsg = "forbid: Must be hosting a game to forbid users.";
					session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
				}
			} else {
				const char *errmsg = "forbid: Invalid syntax";
				const char *errmsg2 = "Usage: /forbid (username)";
				session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
				session[session_id]->msg(errmsg2, strlen(errmsg2)+1, 0, header.channel);
			}
		} else if (strcmp("/sign",arg[0]) == 0) {
			if (!session[session_id]->sign_state()) {
				uint8_t permission = 0;
				const int success = user_channel_lookup(session[session_id]->get_user_id(), header.channel, &permission, NULL, NULL, NULL);
				if (success && permission >= CHANNEL_TRIAL_PERMISSIONS) {
					Channel *chan = find_channel(header.channel);
					if (chan) {
						int game_id = -1;
						char formatted_str[1000];
						// this is actually the dumbest shit
						snprintf(formatted_str, sizeof(formatted_str), "%%*%uc-%%u", strlen(chan->name));
						if (count >= 2) {
							int buffer;
							if (sscanf(arg[1], formatted_str, &buffer) == 1) {
								game_id = buffer;
							} else if (sscanf(arg[1], "%u", &buffer) == 1) {
								game_id = buffer;
							}
						} else {
							int available_game_count = 0;
							for (int x = 0; x < chan->n_games; ++x) {
								const int n = chan->game[x].n_signed;
								if (n > 0 && n < 10) {	
									available_game_count++;
									game_id = chan->game[x].game_id;
								}
							}
							if (available_game_count > 1) {
								game_id = -1;
								const char *errmsg = "sign: Must enter a game id when there's more than one available game.";
								session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
							}
						}
						if (game_id != -1) {
							// find this game now
							for (int x = 0; x < chan->n_games; ++x) {
								if (chan->game[x].game_id == game_id) {
									const int n_signed = chan->game[x].n_signed;
									if (n_signed < 10 && n_signed > 0) {
										int forbid = false;
										for (uint32_t y = 0; y < chan->game[x].n_forbid; ++y) {
											if (chan->game[x].forbid_list[y] == session[session_id]->get_user_id()) {
												forbid = true;
												y = chan->game[x].n_forbid;
											}
										}
										if (!forbid) {
											chan->game[x].signed_members[chan->game[x].n_signed] = session[session_id]->get_user_id();
											chan->game[x].n_signed++;
											const uint32_t n_players = (chan->game_id == GAME_DOTA) ? 10 : 6;
											char msg[1000];
											snprintf(msg, sizeof(msg), "%s signed game %s (%i/%i).",session[session_id]->get_name(), chan->game[x].game_name, chan->game[x].n_signed, n_players);
											msg_channel(msg, strlen(msg)+1, 0, header.channel);
											session[session_id]->sign();
											if (chan->game[x].n_signed == n_players) {
												snprintf(msg, sizeof(msg), "%s is full. The host may /sg again to start the game.", chan->game[x].game_name);
												msg_channel(msg, strlen(msg)+1, 0, header.channel);
											}
											update_game(header.channel, x);
										} else {
											char msg[1000];
											snprintf(msg, sizeof(msg), "sign: You are forbidden from joining %s.", chan->game[x].game_name);
											session[session_id]->msg(msg, strlen(msg)+1, 0, header.channel);
										}
									}
									x = chan->n_games;
								}
							}
						} 
					} else {
						const char *errmsg = "Unknown channel";
						session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
					}
				} else {
					const char *errmsg = "sign: Permission denied";
					session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
				}
			}
		} else if (strcmp("/pool",arg[0]) == 0) {
			if (session[session_id]->sign_state()) {
				// find game
				Channel *chan = find_channel(header.channel);
				if (chan) {
					for (int x = 0; x < chan->n_games; ++x) {
						for (int y = 0; y < chan->game[x].n_signed; ++y) {
							if (chan->game[x].signed_members[y] == session[session_id]->get_user_id()) {
								char str[2000];
								snprintf(str, sizeof(str), "%s pool:",chan->game[x].game_name);
								int pos = strlen(str);
								for (int z = 0; z < chan->game[x].n_signed; ++z) {
									// running out of iterator names
									int found = 0;
									const char *fmt_strings[2] = { " %s (%i),", " %s (%i)" };
									const int use = (z < chan->game[x].n_signed-1) ? 0 : 1;
									for (int i = 0; i < chan->n_members; ++i) {
										if (chan->members[i].id == chan->game[x].signed_members[z]) {
											snprintf(&str[pos], sizeof(str)-pos, fmt_strings[use],chan->members[i].ptr->get_name(),chan->members[i].rating);
											
											pos += strlen(&str[pos]);
											found = 1;
											i = chan->n_members;
										}
									}
									if (!found) {
										snprintf(&str[pos], sizeof(str)-pos, fmt_strings[use],"unknown");
										pos += strlen(&str[pos]);
									}
								}
								session[session_id]->msg(str, strlen(str)+1, 0, header.channel);
							}
						}
					}
				} else {
					const char *errmsg = "Unknown channel.";
					session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
				}
			} else {
				const char *errmsg = "You aren't signed to a game";
				session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
			}
		} else if (strcmp("/result",arg[0]) == 0 || strcmp("/r",arg[0]) == 0) {
			if (count != 2) {
				const char *errmsg = "Syntax: /result (team name)";
				session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
			} else {
				if (session[session_id]->sign_state()) {
					if (strlen(arg[1]) > 100) {
						char errmsg[1000];
						snprintf(errmsg, sizeof(errmsg), "Unknown team '%s'.", arg[1]);
						session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
					} else {
						// identify team
						char arg_lower[100];
						lower(arg[1], arg_lower);
						uint8_t result = 0;
						if (strcmp(arg_lower,"radiant") == 0 || 
							strcmp(arg_lower,"good") == 0 || 
							strcmp(arg_lower,"warm") == 0 ||
							strcmp(arg_lower,"ct") == 0) {
								result = 1;
						} else if (strcmp(arg_lower,"dire") == 0 || 
							strcmp(arg_lower,"bad") == 0 || 
							strcmp(arg_lower,"cold") == 0 ||
							strcmp(arg_lower,"t") == 0) {
								result = 2;
						} else if (strcmp(arg_lower,"draw") == 0) {
							result = 3;
						}
						if (result) {
							Channel *chan = find_channel(header.channel);
							if (chan) {
								for (int x = 0; x < chan->n_games; ++x) {
									const uint32_t n_players = (chan->game_id == GAME_DOTA) ? 10 : 6;
									if (chan->game[x].n_signed == n_players && chan->game[x].start_time) {
										for (int y = 0; y < n_players; ++y) {
											if (chan->game[x].signed_members[y] == session[session_id]->get_user_id()) {
												chan->game[x].results[y] = result;
												const char *result_strings_dota[3] = { "Radiant", "Dire", "Draw" };
												const char *result_strings_blc[3] = { "Warm", "Cold", "Draw" };
												char msg[1000];
												if (chan->game_id == GAME_DOTA) {
													snprintf(msg, sizeof(msg), "%s has resulted %s.",session[session_id]->get_name(), result_strings_dota[result-1]);
												} else if (chan->game_id == GAME_BLC) {
													snprintf(msg, sizeof(msg), "%s has resulted %s.",session[session_id]->get_name(), result_strings_blc[result-1]);
												}
												msg_channel(msg, strlen(msg)+1, 0, header.channel);
												uint8_t result_count[4];
												result_count[1] = 0;
												result_count[2] = 0;
												result_count[3] = 0;
												
												for (int z = 0; z < n_players; ++z) {
													result_count[chan->game[x].results[z]]++;
												}
												uint8_t official_result = 0;
												if (result_count[1] > n_players / 2) official_result = 1;
												else if (result_count[2] > n_players / 2) official_result = 2;
												else if (result_count[3] > n_players / 2) official_result = 3;
												if (official_result) {
													free(chan->game[x].teams_str);
													result_game(header.channel, x, official_result);
												}
											}
										}
									}
								}
							} else {
								const char *errmsg = "Unknown channel.";
								session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
							}
						} else {
							char errmsg[1000];
							snprintf(errmsg, sizeof(errmsg), "Unknown team '%s'.", arg[1]);
							session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
						}
					}
				} else {
					const char *errmsg = "You aren't in a game!";
					session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
				}
			}
		} else if (strcmp("/teams",arg[0]) == 0) {
			if (session[session_id]->sign_state()) {
				Channel *chan = find_channel(header.channel);
				if (chan) {
					for (int x = 0; x < chan->n_games; ++x) {
						for (int y = 0; y < chan->game[x].n_signed; ++y) {
							if (chan->game[x].signed_members[y] == session[session_id]->get_user_id()) {
								if (chan->game[x].start_time) {
									if (chan->game[x].teams_str) session[session_id]->msg(chan->game[x].teams_str, strlen(chan->game[x].teams_str)+1, 0, header.channel);
									y = chan->game[x].n_signed;
									x = chan->n_games;
								} else {
									const char *errmsg = "teams: Game hasn't started yet.";
									session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
								}
							}
						}
					}	
				} else {
					const char *errmsg = "teams: Invalid channel.";
					session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
				}
			} else {
				const char *errmsg = "teams: Not signed into a game.";
				session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
			}
		} else if (strcmp("/out",arg[0]) == 0 || strcmp("/outgame",arg[0]) == 0) {
			out_game(session_id, header.channel, false);
		} else if (strcmp("/vouch",arg[0]) == 0 || strcmp("/unvouch",arg[0]) == 0) {
			if (header.channel) {
				uint8_t permission = 0;
				const int success = user_channel_lookup(session[session_id]->get_user_id(), header.channel, &permission, NULL, NULL, NULL);
				if (success && permission >= CHANNEL_VOUCHER_PERMISSIONS) {
				// this is sort of messy
					if ((count == 2 || count == 3) || strcmp("/unvouch",arg[0]) == 0) {
						int level = CHANNEL_PLAYER_PERMISSIONS;
						if (count == 3) {
							// what ap ain in the ass
							if (strcmp("-unvouch",arg[2]) == 0) {
								level = 0;
							} else if (strcmp("-spectator",arg[2]) == 0) {
								level = CHANNEL_SPECTATOR_PERMISSIONS;
							} else if (strcmp("-trial",arg[2]) == 0) {
								level = CHANNEL_TRIAL_PERMISSIONS;
							} else if (strcmp("-player",arg[2]) == 0) {
								level = CHANNEL_PLAYER_PERMISSIONS;
							} else if (strcmp("-voucher",arg[2]) == 0) {
								level = CHANNEL_VOUCHER_PERMISSIONS;
							} else if (strcmp("-moderator",arg[2]) == 0) {
								level = CHANNEL_MODERATOR_PERMISSIONS;
							} else if (strcmp("-admin",arg[2]) == 0) {
								level = CHANNEL_ADMIN_PERMISSIONS;
							} else {
								level = -1;
							}
						} else if (count == 2) {
							if (strcmp("/unvouch",arg[0]) == 0) level = 0;
						}
						if (level == -1) {
							char errmsg[800];
							snprintf(errmsg, sizeof(errmsg), "vouch(0*): Unknown vouch type '%s'", arg[2]);
							session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
						} else if (level >= permission) {
							char errmsg[800];
							if (count == 3) snprintf(errmsg, sizeof(errmsg), "vouch(0*): Insufficient privilege for '%s'", arg[2]);
							else snprintf(errmsg, sizeof(errmsg), "vouch(0*): Insufficient privilege for vouch level %i", level);
							session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
						} else {
							// find user id
							start_user_query:
							uint32_t user_id = 0;
							mysqlpp::Query get_user = sql_connection.query();
							get_user << "SELECT id FROM users WHERE username='" << arg[1] << "'";
							try {
								mysqlpp::StoreQueryResult result = get_user.store();
								if (!result || !result.num_rows()) {
									char errmsg[800];
									snprintf(errmsg, sizeof(errmsg), "vouch: Couldn't find user '%s'",arg[1]);
									session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
								} else {
									user_id = result[0]["id"];
								}
								if (user_id) {
									mysqlpp::Query query = sql_connection.query();
									query << "SELECT permissions FROM league_members WHERE league_id=" << header.channel << " AND user_id=" << user_id;
									mysqlpp::StoreQueryResult result = query.store();
									mysqlpp::Query store = sql_connection.query();
									if (!result || !result.num_rows()) {
										store << "INSERT INTO league_members (user_id, league_id, permissions, voucher) VALUES (" 
											<< user_id << ", " << header.channel << ", " << level << ", \"" << session[session_id]->get_name() << "\")";
									} else {
										const uint8_t target_user_permissions = result[0]["permissions"];
										if (target_user_permissions >= permission) {
											const char *errmsg = "vouch: Cannot modify users with higher privilege level";
											session[session_id]->msg(errmsg, strlen(errmsg)+1, 0, header.channel);
											free(data);
											free(arg);
											return;
										} else {
											store << "UPDATE league_members SET permissions=" << level << " WHERE user_id=" << user_id << " AND league_id=" << header.channel;
										}
									}
									if (store.exec()) {
										char vouch_message[800];
										if (level == 0) {
											snprintf(vouch_message, sizeof(vouch_message), "%s unvouched %s.", session[session_id]->get_name(), arg[1]);
										} else if (level == CHANNEL_SPECTATOR_PERMISSIONS) {
											snprintf(vouch_message, sizeof(vouch_message), "%s spectator vouched %s.", session[session_id]->get_name(), arg[1]);
										} else if (level == CHANNEL_TRIAL_PERMISSIONS) {
											snprintf(vouch_message, sizeof(vouch_message), "%s trial vouched %s.", session[session_id]->get_name(), arg[1]);
										} else if (level == CHANNEL_PLAYER_PERMISSIONS) {
											snprintf(vouch_message, sizeof(vouch_message), "%s vouched %s.", session[session_id]->get_name(), arg[1]);
										} else if (level == CHANNEL_VOUCHER_PERMISSIONS) {
											snprintf(vouch_message, sizeof(vouch_message), "%s promoted %s to voucher.", session[session_id]->get_name(), arg[1]);
										} else if (level == CHANNEL_MODERATOR_PERMISSIONS) {
											snprintf(vouch_message, sizeof(vouch_message), "%s promoted %s to moderator.", session[session_id]->get_name(), arg[1]);
										} else if (level == CHANNEL_ADMIN_PERMISSIONS) {
											snprintf(vouch_message, sizeof(vouch_message), "%s promoted %s to admin.", session[session_id]->get_name(), arg[1]);
										} else {
											vouch_message[0] = '\0';
										}
										msg_channel_as_admin(vouch_message, strlen(vouch_message)+1, 0, header.channel);
										if (level == 0) { // remove them if theyre in the channel.
											if (remove_user_from_channel(user_id, header.channel)) {
												for (int x = 0; x < n_connections; ++x) {
													if (session[x]->get_user_id() == user_id) {
														session[x]->remove_channel(header.channel);
														x = n_connections;
													}
												}
											}
										}
									}
								}  else {
									// redundant
									char errmsg[800];
									snprintf(errmsg, sizeof(errmsg), "vouch: Couldn't find user '%s'",arg[1]);
									session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
								} 
							} catch (mysqlpp::BadQuery error) {
								char buffer[2048];
								snprintf(buffer,sizeof(buffer),"SQL reported bad query: %s. Attempting reconnect...",sql_connection.error());
								puts(buffer);
								try {
									sql_connection.connect(sql_db.c_str(),sql_host.c_str(),sql_user.c_str(),sql_pass.c_str(),3307);
									puts("Successfully reconnected to SQL server.");
									goto start_user_query;
								} catch (mysqlpp::ConnectionFailed sql_error) {
									std::string error = "SQL reconnect error: "; error += sql_error.what();
									puts(error.c_str());
									
									const char *errmsg = "Error adding user. Please try again later.";
									session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
								}
							}
						}
					} else {
						const char *errmsg1 = "vouch(0*): Invalid syntax";
						const char *errmsg2 = "Usage: /vouch (username) (optional: vouch level, default: -player)";
						const char *errmsg3 = "Or: /unvouch (username) ";
						session[session_id]->server_msg(errmsg1,strlen(errmsg1)+1);
						session[session_id]->server_msg(errmsg2,strlen(errmsg2)+1);
						session[session_id]->server_msg(errmsg3,strlen(errmsg3)+1);
					}
				} else {
					const char *errmsg = "vouch(0*): Permission denied";
					session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
				}
			} else {
				const char *errmsg = "vouch: Not in a channel.";
				session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
			}
		} else if (strcmp("/truant",arg[0]) == 0) {
			if (count == 2) {
				if (session[session_id]->sign_state()) {
					Channel *chan = find_channel(header.channel);
					if (chan) {
						for (int x = 0; x < chan->n_games; ++x) {
							for (int y = 0; y < chan->game[x].n_signed; ++y) {
								if (chan->game[x].signed_members[y] == session[session_id]->get_user_id()) {
									// game id is x.
									if (chan->game[x].start_time >= minutes(5)) {
										mysqlpp::Query query = sql_connection.query();
										query << "SELECT * FROM users WHERE username = \'" << arg[1] << "\' LIMIT 1";
										try {
											mysqlpp::StoreQueryResult result = query.store();
											if (result.num_rows()) {
												const uint32_t truant_id = result[0]["id"];
												bool found = false;
												// find this player in the game
												for (int z = 0; z < chan->game[x].n_signed; ++z) {
													if (chan->game[x].signed_members[z] == truant_id) found = true;
												}
												if (found) {
													if (chan->game[x].truant[y] == truant_id) {
														chan->game[x].truant[y] = 0;
														char msg[1000];
														snprintf(msg, sizeof(msg), "%s has un-truanted %s.", session[session_id]->get_name(), std::string(result[0]["username"]).c_str());
														msg_channel(msg, strlen(msg)+1, 0, header.channel);
													} else {
														chan->game[x].truant[y] = truant_id;
														char msg[1000];
														snprintf(msg, sizeof(msg), "%s has truanted %s.", session[session_id]->get_name(), std::string(result[0]["username"]).c_str());
														msg_channel(msg, strlen(msg)+1, 0, header.channel);
														int truant_count = 0;
														const uint32_t n_players = (chan->game_id == GAME_DOTA) ? 10 : 6;
														for (int z = 0; z < n_players; ++z) {
															if (chan->game[x].truant[z] == truant_id) truant_count++;
														}
														if (truant_count > n_players / 2) {
															chan->game[x].start_time = 0;
															chan->game[x].n_signed = 0;
															char newmsg[1000];
															snprintf(newmsg, sizeof(newmsg), "%s closed. (%s truanted)",chan->game[x].game_name, std::string(result[0]["username"]).c_str());
															for (int z = 0; z < n_connections; ++z) {
																// it seems like this will fail if the hoster isnt active.
																if (chan->game[x].hoster_id == session[z]->get_user_id()) {
																	out_game(z, header.channel, false);
																}
																// make sure everyones unsigned anyway.
																for (int i = 0; i < n_players; ++i) {
																	if (session[z]->get_user_id() == chan->game[x].signed_members[i]) {
																		session[z]->unsign();
																		i = 10;
																	}
																}
															}
															msg_channel(newmsg, strlen(newmsg)+1, 0, header.channel);
															// update manually in case the host is dced.
															update_game(header.channel, x);						
															mysqlpp::Query rating_update = sql_connection.query();
															rating_update << "UPDATE league_members SET rating = rating - 50 WHERE user_id = " << truant_id << " LIMIT 1";
															try {
																rating_update.exec();
															} catch (mysqlpp::BadQuery error) {
															
															}
														}
													}
												} else {
													const char *errmsg = "truant: Can only truant players in the same game as you.";
													session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
												}
											} else {
												const char *errmsg = "truant: Couldn't find player.";
												session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
											}
										} catch (mysqlpp::BadQuery error) {
											const char *errmsg = "truant: Error looking up player.";
											session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
										}
									} else {
										const char *errmsg = "truant: Can't truant players until game has started and 5 minutes have passed.";
										session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
									}
								}
							}
						}
					} else {
						const char *errmsg = "truant: Invalid channel.";
						session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
					}
				} else {
					const char *errmsg = "truant: Must be in a game to truant a player.";
					session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
				}
			} else {
				const char *errmsg = "truant: Syntax: /truant (name)";
				session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
			}
		} else if (strcmp("/join",arg[0]) == 0) {
			if (session[session_id]->get_user_id()) {
				if (count >= 2) {
					for (int x = 1; x < count; ++x) {
						const uint16_t channel_id = search_channel(arg[x]);
						Channel *chan = find_channel(channel_id);
						if (channel_id && chan) {
							int found = 0;
							// make sure the user isn't already in the channel.
							for (int y = 0; y < chan->n_members; ++y) {
								if (chan->members[y].id == session[session_id]->get_user_id()) {
									found = 1;
									y = chan->n_members;
								}
							}
							if (!found) {
								uint8_t permission;
								uint16_t warn_level, rating;
								char voucher[64];
								const int is_allowed = user_channel_lookup(session[session_id]->get_user_id(), channel_id, &permission, &warn_level, voucher, &rating);
								if (is_allowed) {
									if (permission) {
										add_user_to_channel(session_id, channel_id, permission, warn_level, voucher, rating);
										printf("user joined channel\n");
									} else {
										char errmsg[800];
										snprintf(errmsg, sizeof(errmsg), "join: Unable to join channel '%s' (Permission denied)", arg[x]);
										session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
									}
								} else {
									char errmsg[800];
									snprintf(errmsg, sizeof(errmsg), "join: Can't join '%s' - not vouched", arg[x]);
									session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
								}
							} else {
								char errmsg[800];
								snprintf(errmsg, sizeof(errmsg), "join: Already in channel %s.", arg[x]);
								session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
							}
						} else {
							char errmsg[800];
							snprintf(errmsg, sizeof(errmsg), "join: Unknown channel '%s'", arg[x]);
							session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
						}
					}
				}
			} else {
				char errmsg[800];
				snprintf(errmsg, sizeof(errmsg), "join: You need to login first!");
				session[session_id]->server_msg(errmsg,strlen(errmsg)+1);
			}
		} else if (strcmp("/leave",arg[0]) == 0) {
			if (out_game(session_id, header.channel, true)) {
				remove_user_from_channel(session[session_id]->get_user_id(), header.channel);
				session[session_id]->remove_channel(header.channel);
			} else {
				const char *errmsg = "Can't leave while a game is in progress.";
				session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
			}
		} else if (strcmp("/login",arg[0]) == 0) {
			if (count == 3 || (count == 4 && strcmp(arg[3],"-save") == 0)) {
				if (!session[session_id]->get_user_id()) {
					if (session[session_id]->get_last_login_attempt() + seconds(1) > server_time) {
						char msg[100];
						snprintf(msg, sizeof(msg), "login: Tried logging in again too early. Try again in %f seconds.",(session[session_id]->get_last_login_attempt() + seconds(1) - server_time)/1000.f);
						session[session_id]->server_msg(msg, strlen(msg)+1);
					} else {
						session[session_id]->log_login_attempt(server_time);
						user_login_msg auth;
						auth.version = NETWORK_PROTOCOL_VERSION;
						auth.region = 0;
						auth.using_token = 0;
						memset(auth.username, 0, sizeof(auth.username));
						strncpy(auth.username, arg[1], sizeof(auth.username)-1);
						memset(auth.auth, 0, sizeof(auth.auth));
						strncpy(auth.auth, arg[2], sizeof(auth.auth)-1);
						auth_user(session_id, auth);
						
						// user_id is set on success
						if (session[session_id]->get_user_id() && count == 4) {
							// return a token
							unsigned char random[30];
							get_random(random, sizeof(random));
							char new_token[61];
							binary_to_hex((unsigned char*)random, sizeof(random), new_token);
							
							uint64_t token[2];
							MD5((unsigned char*)new_token, 60, (unsigned char*)token);
							
							login_token_startquery:
							mysqlpp::Query query = sql_connection.query();
							std::string escaped_name;
							query.escape_string(&escaped_name, session[session_id]->get_name(), strlen(session[session_id]->get_name()));
							query << "UPDATE users SET token=" << token[0] << " WHERE username='" << escaped_name << "'";
							try {
								query.exec();
								token_update user_token;
								strcpy(user_token.username, escaped_name.c_str());
								strcpy(user_token.token, new_token);
								
								unsigned char msg[1000];
								msg[0] = SERVER_MESSAGE_NEW_AUTH_TOKEN;
								memcpy(&msg[1], &user_token, sizeof(user_token));
								session[session_id]->send_raw(msg, 1+sizeof(user_token));
							}  catch (mysqlpp::BadQuery error) {
								char buffer[2048];
								snprintf(buffer,sizeof(buffer),"SQL reported bad query: %s. Attempting reconnect...",sql_connection.error());
								puts(buffer);
								try {
									sql_connection.connect(sql_db.c_str(),sql_host.c_str(),sql_user.c_str(),sql_pass.c_str(),3307);
									puts("Successfully reconnected to SQL server.");
									goto login_token_startquery;
								} catch (mysqlpp::ConnectionFailed sql_error) {
									std::string error = "SQL reconnect error: "; error += sql_error.what();
									puts(error.c_str());
									
									const char *errmsg = "Error generating login token. Please try again later.";
									session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
								}
							}
						}
					}
				} else {
					const char *errmsg = "login: You're already logged in!";
					session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
				}
			} else {
				const char *errmsg = "login: /login (username) (password)";
				session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
			}
		} else if (strcmp("/help",arg[0]) == 0) {
		// what a fucking mess
			if (count != 2) {
				const char *helpmsg1 = "help: List info about user commands.";
				const char *helpmsg2 = "Usage: /help (category)";
				const char *helpmsg3 = "Categories are: 'channel', 'account', 'games'";
				const char *helpmsg4 = "All commands can be prefixed with '-', '.', or '/'.";
				session[session_id]->server_msg(helpmsg1, strlen(helpmsg1)+1);
				session[session_id]->server_msg(helpmsg2, strlen(helpmsg2)+1);
				session[session_id]->server_msg(helpmsg3, strlen(helpmsg3)+1);
				session[session_id]->server_msg(helpmsg4, strlen(helpmsg4)+1);
			} else if (count == 2) {
				if (strlen(arg[1]) < 100) {
					char lower_arg[100];
					lower(arg[1], lower_arg);
					if (strcmp(lower_arg,"channel") == 0) {
						const char *helpmsg1 = "help: Channel commands";
						const char *helpmsg2 = "1. /join (channel name): Joins the channel (channel name). Example channel names: NADLo, NADLi.";
						const char *helpmsg3 = "2. /leave: Leaves the current channel";
						const char *helpmsg4 = "3. /stats (optional: username): Display the stats of username. If no username is provided, your name is used instead. NOT IMPLEMENTED";
						const char *helpmsg5 = "4. /compare (user1) (optional: user2): Compares game stats between (user1) and (user2). If no user2 is provided, your name is used instead.";
						session[session_id]->server_msg(helpmsg1, strlen(helpmsg1)+1);
						session[session_id]->server_msg(helpmsg2, strlen(helpmsg2)+1);
						session[session_id]->server_msg(helpmsg3, strlen(helpmsg3)+1);
						session[session_id]->server_msg(helpmsg4, strlen(helpmsg4)+1);
						session[session_id]->server_msg(helpmsg5, strlen(helpmsg5)+1);
					} else if (strcmp(lower_arg,"account") == 0) {
						const char *helpmsg1 = "help: Account commands";
						const char *helpmsg2 = "1. /login (username) (password) (optional: -save): Logs into (username) with (password). If -save is appended, the login is saved and used to automatically login.";
						const char *helpmsg3 = "2. /passwd (new password) (repeat password): Change your password to (new password).";
						session[session_id]->server_msg(helpmsg1, strlen(helpmsg1)+1);
						session[session_id]->server_msg(helpmsg2, strlen(helpmsg2)+1);
						session[session_id]->server_msg(helpmsg3, strlen(helpmsg3)+1);
					} else if (strcmp(lower_arg,"games") == 0) {
						const char *helpmsg1 = "help: Game commands";
						const char *helpmsg2 = "1. /sign (optional: game name or number): Signs into a game. If no game id is provided and only one game is open, that game is joined. Requires trial vouch or greater.";
						const char *helpmsg3 = "2. /out: Leaves your currently-signed game.";
						const char *helpmsg4 = "3. /sg: Starts a new game. Requires vouched level. /sg can be used by the game host to start a a full game.";
						const char *helpmsg5 = "4. /forbid (name): Forbids and removes a user from your game. Requires game host.";
						const char *helpmsg6 = "5. /result (team: radiant, dire, draw): Votes to result the game. Only available after 5 minutes have passed.";
						const char *helpmsg7 = "5. /truant (name): Votes to draw the game as a result of (name)'s truancy. Truanted players are deducted rating.";
						
						session[session_id]->server_msg(helpmsg1, strlen(helpmsg1)+1);
						session[session_id]->server_msg(helpmsg2, strlen(helpmsg2)+1);
						session[session_id]->server_msg(helpmsg3, strlen(helpmsg3)+1);
						session[session_id]->server_msg(helpmsg4, strlen(helpmsg4)+1);
						session[session_id]->server_msg(helpmsg5, strlen(helpmsg5)+1);
						session[session_id]->server_msg(helpmsg6, strlen(helpmsg6)+1);
						session[session_id]->server_msg(helpmsg7, strlen(helpmsg7)+1);
					} else {
						char helpmsg[1000];
						snprintf(helpmsg, sizeof(helpmsg), "help: Unknown category '%s'.", arg[1]);
						session[session_id]->server_msg(helpmsg, strlen(helpmsg)+1);
					}
				}
			} 
		} else {
			char errmsg[800];
			snprintf(errmsg, sizeof(errmsg), "Unknown command '%s'.",&arg[0][1]);
			session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
		}
		free(data);
		free(arg);
	} else { // this is a standard message.
		std::string sanitized_string = sanitize_string(message);
		//printf("%s sanitized to \n-> %s\n",message, sanitized_string.c_str());
		//fflush(stdout);
		
		const uint32_t id = session[session_id]->get_user_id();
		const uint8_t permissions = get_user_active_channel_permissions(id, header.channel);
		if (permissions >= CHANNEL_CHAT_PERMISSIONS) {
			char msg[4000];
			if (permissions >= CHANNEL_ADMIN_PERMISSIONS) {
				snprintf(msg, sizeof(msg), "<span style=\"color:#c4181d;\">%s</span>: %s", session[session_id]->get_name(), sanitized_string.c_str());
			} else if (permissions >= CHANNEL_VOUCHER_PERMISSIONS) {
				snprintf(msg, sizeof(msg), "<span style=\"color:#005bae;\">%s</span>: %s", session[session_id]->get_name(), sanitized_string.c_str());
			} else {
				snprintf(msg, sizeof(msg), "%s: %s", session[session_id]->get_name(), sanitized_string.c_str());
			}
			msg_channel(msg, strlen(msg), id, header.channel);
		}
	}
}

int Server::out_game(const uint32_t session_id, const uint32_t channel_id, const bool silent) {
	Channel *chan = find_channel(channel_id);
	if (chan) {
		int found = 0;
		for (int x = 0; x < chan->n_games; ++x) {
			for (int y = 0; y < chan->game[x].n_signed; ++y) {
				if (chan->game[x].signed_members[y] == session[session_id]->get_user_id()) {
					found = 1;
					if (!chan->game[x].start_time) {
						if (chan->game[x].hoster_id == session[session_id]->get_user_id()) {
							for (int y = 0; y < chan->game[x].n_signed; ++y) {
								for (int z = 0; z < n_connections; ++z ) {
									if (session[z] && session[z]->get_user_id() == chan->game[x].signed_members[y]) {
										session[z]->unsign();
									}
								}
							}
							// disband the game
							chan->game[x].n_signed = 0;
							chan->game[x].hoster_id = 0;
							free(chan->game[x].forbid_list);
							chan->game[x].forbid_list = NULL;
							chan->game[x].n_forbid = 0;
							char msg[1000];
							snprintf(msg,sizeof(msg),"%s closed game %s.",session[session_id]->get_name(),chan->game[x].game_name);
							msg_channel_as_admin(msg, strlen(msg)+1, 0, channel_id);
							if (chan->n_unused < 50) chan->unused[chan->n_unused] = chan->game[x].game_id;
						} else {
							chan->game[x].n_signed--;
							chan->game[x].signed_members[y] = chan->game[x].signed_members[chan->game[x].n_signed];
							char msg[1000];
							snprintf(msg,sizeof(msg),"%s signed out of game %s.",session[session_id]->get_name(),chan->game[x].game_name);
							msg_channel_as_admin(msg, strlen(msg)+1, 0, channel_id);
						}
						update_game(channel_id, x);
						session[session_id]->unsign();
						return 1;
					} else {
						const char *errmsg = "Can't sign out of a game in progress.";
						session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
						return 0;
					}
				}
			}
		}
		if (!found && !silent) {
			const char *errmsg = "You aren't signed into any games.";
			session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
		}
	} else {
		if (!silent) {
			const char *errmsg = "Unknown channel";
			session[session_id]->server_msg(errmsg, strlen(errmsg)+1);
		}
	}
	return 1;
}
int Server::get_random(void *output, const int len) {
	return fread(output, 1, len, rngstream);
}

void Server::result_game(const uint16_t channel, const uint32_t game, const uint8_t result) {
	Channel *chan = find_channel(channel);
	if (chan) {
		Game *g = &chan->game[game];
		const uint8_t n_players = (chan->game_id == GAME_DOTA) ? 10 : 6;
		if (result == 3) {
			char msg[2000];
			snprintf(msg, sizeof(msg), "Game %i resulted in a draw.", g->game_id);
			msg_channel(msg, strlen(msg)+1, 0, channel);
			return;
		}
		
		startquery:
		try {
			mysqlpp::Query query = sql_connection.query();
			query << "REPLACE INTO games (game_id, league_id, winner, rating_change) VALUES (" << g->game_id << ", " << channel << ", " << (int)result << ", " << g->rating_change << ")";
			//printf("query: %s\n",query.str().c_str());fflush(stdout);
			query.exec();
			for (int x = 0; x < n_players; ++x) {
				mysqlpp::Query query_result = sql_connection.query();
				const int team = (x >= 5) ? 2 : 1;
				query_result << "REPLACE INTO game_members (game_id, league_id, user_id, team, result) VALUES (" << g->game_id << ", " << channel << ", " << g->signed_members[x] << ", " << team << ", " << (int)result << ")";
				query_result.exec();
			}
			const int radiant_change = ((result == 1) ? 25 : -25) + g->rating_change;
			const int dire_change = -radiant_change;
			for (int x = 0; x < n_players; ++x) {
				mysqlpp::Query rating = sql_connection.query();
				const int change = (x < 5) ? radiant_change : dire_change;
				if (change >= 0) rating << "UPDATE league_members SET rating = rating + " << change << ", n_games = n_games + 1, n_wins = n_wins + 1 WHERE league_id = " << channel << " AND user_id = " << g->signed_members[x];
				else rating << "UPDATE league_members SET rating = rating + " << change << ", n_games = n_games + 1 WHERE league_id = " << channel << " AND user_id = " << g->signed_members[x];
				//printf("query: %s\n",rating.str().c_str());fflush(stdout);
				rating.exec();
				for (int y = 0; y < chan->n_members; ++y) { 
					if (chan->members[y].id == g->signed_members[x]) {
						if ((result == 1 && x < 5) || (result == 2 && x >= 5)) chan->members[y].rating += g->rating_change;
						else chan->members[y].rating -= g->rating_change;
						y = chan->n_members;
					}
				}
			}
		}  catch (mysqlpp::BadQuery error) {
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
			}
		}
		
		const char *result_str_dota[2] = { "Radiant", "Dire" };
		const char *result_str_blc[2] = { "Cold", "Warm" };
		char msg[2000];
		if (chan->game_id == GAME_DOTA) {
			snprintf(msg, sizeof(msg), "Game %i ended. %s victory.", g->game_id, result_str_dota[result-1]);
		} else if (chan->game_id == GAME_BLC) {
			snprintf(msg, sizeof(msg), "Game %i ended. %s victory.", g->game_id, result_str_blc[result-1]);
		}
		msg_channel(msg, strlen(msg)+1, 0, channel);
		
		for (int x = 0; x < n_players; ++x) {
			for (int y = 0; y < n_connections; ++y) {
				if (session[y]->get_user_id() == g->signed_members[x]) {
					session[y]->unsign();
					y = n_connections;
				}
			}
		}
		
		g->start_time = 0;
		g->n_signed = 0;
		update_game(channel, game);
		
		
		mysqlpp::Query get_ratings = sql_connection.query();
		try {
			rank_state rank;
			uint8_t rank_count = 0;
			unsigned char msg[(1+sizeof(rank_state))*100];
			get_ratings << "SELECT * FROM league_members WHERE league_id = " << channel << " ORDER BY rating DESC LIMIT 25";
			mysqlpp::StoreQueryResult result = get_ratings.store();
			rank_count = result.num_rows();
			if (!rank_count) return;
			for (int x = 0; x < result.num_rows(); ++x) {
				mysqlpp::Query get_user = sql_connection.query();
				get_user << "SELECT * FROM users WHERE id = " << result[x]["user_id"];
				mysqlpp::StoreQueryResult user = get_user.store();
				memset(&rank, 0, sizeof(rank));
				rank.position = x;
				rank.rating = result[x]["rating"];
				rank.channel = channel;
				if (user.num_rows()) snprintf(rank.name, sizeof(rank.name), std::string(user[0]["username"]).c_str());
				else strcpy(rank.name,"Unknown");
				msg[(1+sizeof(rank_state))*x] = SERVER_MESSAGE_CHANNEL_UPDATE_LEADERBOARDS;
				memcpy(&msg[1+((1+sizeof(rank_state))*x)], &rank, sizeof(rank));
			}
			for (int x = 0; x < chan->n_members; ++x) {
				chan->members[x].ptr->send_raw(msg, (1+sizeof(rank))*rank_count);
			}
		} catch (mysqlpp::BadQuery error) {
			char buffer[2048];
			snprintf(buffer,sizeof(buffer),"SQL reported bad query: %s. Attempting reconnect...",sql_connection.error());
			puts(buffer);
		}
	}
}

void Server::command(const int session_id, unsigned char *msg, const int msglen) {
	unsigned char chunk[RECV_MAXLEN*2];
	// check if we have buffered data from a previous recv() call.
	int buffer_len, remaining_length = msglen;
	unsigned char *buffer = session[session_id]->get_buffer(&buffer_len);
	if (buffer_len) {
		memcpy(chunk, buffer, buffer_len);
		memcpy(&chunk[buffer_len], msg, msglen);
		msg = chunk;
		remaining_length = buffer_len + msglen;
	}
	unsigned char *position = msg;
	while (remaining_length) {
		switch (position[0]) {
			case CLIENT_MESSAGE_CONNECT:
				if (remaining_length > sizeof(user_login_msg)) {
					user_login_msg authinfo;
					memcpy(&authinfo, &position[1], sizeof(user_login_msg));
					
					auth_user(session_id, authinfo);
					remaining_length -= (1 + sizeof(user_login_msg));
					position = &position[1+sizeof(user_login_msg)];
				} else {
					goto buffer_remaining_data;
				}
				break;
				
			case CLIENT_MESSAGE_REQUEST_USER_INFO:
				if (remaining_length > sizeof(uint32_t)) {
					uint32_t requested_id = *(uint32_t*)&position[1];
					
					remaining_length -= (1 + sizeof(uint32_t));
					position = &position[1+sizeof(uint32_t)];
				} else {
					goto buffer_remaining_data;
				}
				break;
			case CLIENT_MESSAGE_CHAT_MESSAGE:
				if (remaining_length > sizeof(chat_message_header)) {
					// check header
					chat_message_header chat_header;
					memcpy(&chat_header, &position[1], sizeof(chat_header));
					if (chat_header.msg_length > remaining_length - sizeof(chat_message_header)) {
						goto buffer_remaining_data;
					} else {
						char message[RECV_MAXLEN];
						// make sure there's a null-terminator here.
						position[1+sizeof(chat_header)+chat_header.msg_length-1] = '\0';
						memcpy(message, &position[1+sizeof(chat_header)], chat_header.msg_length);
						
						interpret_message(session_id, chat_header, message);
						
						remaining_length -= (1 + sizeof(chat_message_header) + chat_header.msg_length);
						position = &position[1+sizeof(chat_message_header)+chat_header.msg_length];
					}
				} else {
					goto buffer_remaining_data;
				}
				break;
			case CLIENT_MESSAGE_PONG:
				session[session_id]->touch(server_time);
				remaining_length--;
				position = &position[1];
				break;
			default:
				// Unhandled packet type. Either a protocol error or someone's doing something fishy.
				puts("unhandled packet");fflush(stdout);
				disconnect_session(session_id, SERVER_MESSAGE_DISCONNECT_PROTOCOL_ERROR);
				return;
		}
	}
	session[session_id]->touch(server_time);
	// it's like im an openssl contributor or something
	if (0) {
		buffer_remaining_data:
		printf("Buffering remaining %i bytes. header is %i\n",remaining_length, position[0]);
		if (remaining_length > RECV_MAXLEN) {
			disconnect_session(session_id, SERVER_MESSAGE_DISCONNECT_PROTOCOL_ERROR);
		} else {
			session[session_id]->buffer_data(position, remaining_length);
		}
	}
}

void Server::force_shutdown() {
	puts("Forcing shutdown");
	fflush(stdout);
	quit = true;
}

int Server::find_session(const char *name, uint32_t *out_session) {
	for (int x = 0; x < n_connections; ++x) {
		if (session[x]) {
			if (strcmp(name, session[x]->get_name()) == 0) {
				*out_session = x;
				return 1;
			}
		}
	}
	return 0;
}