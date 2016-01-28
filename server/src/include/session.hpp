#ifndef __SESSION_HPP 
#define __SESSION_HPP 

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "data_types.hpp"

class Session {
private:
	/* Connection info. user_id = account id*/
	uint32_t user_id;
	int sockfd;
	
	char username[64];
	uint8_t user_permissions, auth_attempts;
	
	uint64_t last_update, last_login_attempt, last_ping;
	uint16_t *channel, n_channels;
	
	unsigned char buffer[RECV_MAXLEN];
	uint32_t buffer_pos;
	int is_signed;
public:
	Session(const int new_sockfd);
	~Session();
	void join_channel(const uint16_t channel_id);
	/* Same as destroying the session. */
	void disconnect(const uint8_t disconnect_msg);
	void auth(const char *input_name, const uint32_t input_id, const uint8_t input_permissions);
	
	void send_raw(const unsigned char *msg, const uint16_t len);

	void buffer_data(unsigned char *data, const int len);
	void clear_buffer();
	
	void set_permissions(const uint8_t input_permissions);
	void set_name(const char *input_name);
	void touch(const uint64_t time);
	
	/* @len is the byte-length of @message incl the null-terminator. */
	int server_msg(const char *message, const int len);
	int msg(const char *message, const int len, const uint32_t sender, const uint16_t channel);
	
	unsigned char *get_buffer(uint32_t *len);
	char *get_name();
	uint64_t get_last_update() const;
	uint32_t get_user_id() const;
	uint8_t get_permissions() const;
	int get_sockfd();
	// returns channels joined
	int add_channel(const uint16_t channel_id);
	void remove_channel(const uint16_t channel_id);
	uint16_t *get_channels(uint16_t *num);
	uint64_t get_last_login_attempt() const;
	void log_login_attempt(const uint64_t input_time);
	void ping(const uint64_t time);
	
	void sign();
	void unsign();
	int sign_state() const;
};


#endif /* __SESSION_HPP  */