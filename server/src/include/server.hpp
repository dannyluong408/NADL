#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <mysql++/mysql++.h>
#include <mysql++/mystring.h>
#include <time.h>
#include <openssl/md5.h>

extern "C" {
#include <libscrypt.h>
}

#define LISTENING_PORT 3221

#include "data_types.hpp"
#include "session.hpp"

// millisecond conversions
#define minutes(x) (x * 60000)
#define seconds(x) (x *  1000)

struct Member {
	uint32_t id;
	uint16_t warn_level, rating;
	uint8_t permission;
	
	char voucher[64];
	Session *ptr;
};

struct Game {
	char game_name[64];
	uint32_t hoster_id;
	uint32_t signed_members[10]; // user ids.
	uint8_t results[10]; // 1 = good, 2 = bad, 3 = draw
	uint32_t truant[10];
	
	char *teams_str;
	
	uint8_t n_signed;

	uint32_t game_id; // the id of the game given by the channel. not location.
	
	uint32_t *forbid_list;
	uint32_t n_forbid;
	
	int16_t rating_change;
	time_t start_time;
};

struct Channel {
	char name[64];
	uint8_t flags;

	Member *members;
	uint32_t n_members;
	uint64_t game_number; // which id to start on
	
	Game *game;
	uint64_t n_games;
	
	uint32_t unused[50];
	uint16_t n_unused;
	
	uint16_t channel_id;
};



class Server {
private:
	uint8_t quit;
	
	FILE *rngstream;
	
	int socket4;
	
	uint32_t n_connections;
	// don't store session ids. 
	Session **session;
	
	uint32_t n_games;
	uint16_t current_id;
	
	
	uint64_t server_time;
	std::map<uint16_t,Channel*> channels;
	Channel **channel_data;
	uint32_t n_channels;
	
	std::string sql_db, sql_host, sql_user, sql_pass;
	mysqlpp::Connection sql_connection;
	void disconnect_session(const uint32_t session_id, const uint8_t msg);
	/* session[session_id] must exist. */
	void command(const int session_id, unsigned char *msg, const uint32_t msglen);
	void auth_user(const int session_id, user_login_msg auth_info);
	/* @message may be modified, but not extended. */
	void interpret_message(const int session_id, const chat_message_header header, char *message);
	int get_random(void *output, const int len);
	
	// channel/league chat/control functions
	Channel *find_channel(const uint16_t channel_id);
	void channel_msg(const char *msg, const uint32_t msg_len, const uint32_t sender, const uint16_t channel_id);
	// returns number of users removed from channel
	int remove_user_from_channel(const uint32_t user_id, const uint16_t channel_id);
	void add_user_to_channel(const uint32_t session_id, const uint16_t channel_id, const uint8_t permissions, const uint16_t warn_level, const char *voucher, const uint16_t rating); 
	
	uint8_t get_user_active_channel_permissions(const uint32_t user_id, const uint16_t channel_id);
	int user_channel_lookup(const uint32_t user_id, const uint16_t channel_id, uint8_t *permission_out, uint16_t *warn_level, char *voucher, uint16_t *rating);
	uint16_t search_channel(const char *name);
	void add_channel(const char *name, const uint8_t flags, const uint16_t id, const uint32_t owner_id, const uint32_t number_of_games);

	Member *find_member(const uint32_t user_id, const uint16_t channel_id);
	
	int msg_channel(const char *msg, const uint16_t len, const uint32_t sender, const uint16_t channel);
	int find_session(const char *name, uint32_t *out_session);
	std::string sanitize_string(char *msg);
	// 0 on failure, 1 on success
	int out_game(const uint32_t session_id, const uint32_t channel_id, const bool silent);
	void update_game(const uint16_t league_id, const uint16_t game_id);
	void lower(const char *source, char *output);
	void result_game(const uint16_t channel, const uint32_t game, const uint8_t result);
	// games and matchmaking
public:
	Server();
	~Server();
	int init(const int argc, char **argv);
	void run();
	void force_shutdown();
};