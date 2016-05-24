#include "session.hpp"

Session::Session(const int new_sockfd) {
	user_id = 0;
	sockfd = new_sockfd;
	memset(username, 0, sizeof(username));
	snprintf(username, sizeof(username), "Unknown");
	user_permissions = 0;
	channel = NULL;
	n_channels = 0;
	last_update = 0;
	buffer_pos = 0;
	last_login_attempt = 0;
	last_ping = 0;
	is_signed = false; 
}

Session::~Session() {
	free(channel);
}

char *Session::get_name() {
	return username;
}

uint64_t Session::get_last_update() const {
	return last_update;
}

uint8_t Session::get_permissions() const {
	return user_permissions;
}

void Session::set_permissions(const uint8_t input_permissions) {
	user_permissions = input_permissions;
}

void Session::set_name(const char *input_name) {
	strncpy(username, input_name, sizeof(username));
}

int Session::get_sockfd() {
	return sockfd;
}

void Session::auth(const char *input_name, const uint32_t input_id, const uint8_t input_permissions) {
	set_name(input_name);
	user_id = input_id;
	set_permissions(input_permissions);
}

int Session::server_msg(const char *message, const int len) {
	const char *color = "<span style=\"color:#57623c;\">";
	const char *color2 = "</span>";
	const int maxlen = len + strlen(color) + strlen(color2);
	char *colored_msg = (char*)malloc(maxlen);
	snprintf(colored_msg, maxlen, "%s%s%s", color, message, color2);
	const int ret = msg(colored_msg, maxlen, 0, UINT16_MAX);
	free(colored_msg);
	return ret;
	
}

int Session::msg(const char *message, const int len, const uint32_t sender, const uint16_t channel) {
	chat_message_header header;
	header.user_id = sender;
	header.channel = channel;
	header.msg_length = len;
	header.msg_flags = (channel == UINT16_MAX || channel == 0) ? MSG_FLAG_PRINTALL : 0;
	
	char *data = (char*)malloc(sizeof(chat_message_header) + len + 1);
	if (!data) return 0;
	data[0] = SERVER_MESSAGE_CHAT_MESSAGE;
	memcpy(&data[1], &header, sizeof(header));
	memcpy(&data[1+sizeof(header)], message, len);
	
	send(sockfd, (void*)data, 1 + sizeof(header) + len, 0);
	free(data);
	return 1;
}

void Session::send_raw(const unsigned char *msg, const uint16_t len) {
	send(sockfd, (void*)msg, len, 0);
}

uint32_t Session::get_user_id() const {
	return user_id;
}

void Session::touch(const uint64_t time) {
	last_update = time;
}

void Session::disconnect(const uint8_t disconnect_msg) {
	send(sockfd, (void*)&disconnect_msg, sizeof(disconnect_msg), 0);
	close(sockfd);
}

unsigned char *Session::get_buffer(int *len) {
	*len = buffer_pos;
	if (!buffer_pos) return NULL;
	else return buffer;
}

void Session::buffer_data(unsigned char *data, const int len) {
	if (len + buffer_pos > RECV_MAXLEN) {
		disconnect(SERVER_MESSAGE_DISCONNECT_PROTOCOL_ERROR);
	} else {
		memcpy(&buffer[buffer_pos], data, len);
		buffer_pos += len;
	}
}

void Session::clear_buffer() {
	buffer_pos = 0;
}

int Session::add_channel(const uint16_t channel_id) {
	for (int x = 0; x < n_channels; ++x) {
		if (channel[x] == channel_id) {
			return 0;
		}
	}
	uint16_t *ptr = (uint16_t*)malloc(sizeof(uint16_t)*(n_channels+1));
	memcpy(ptr, channel, sizeof(uint16_t)*n_channels);
	free(channel);
	channel = ptr;
	channel[n_channels] = channel_id;
	n_channels++;
	return 1;
}

void Session::remove_channel(const uint16_t channel_id) {
	for (int x = 0; x < n_channels; ++x) {
		if (channel[x] == channel_id) {
			unsigned char data[100];
			data[0] = SERVER_MESSAGE_CHANNEL_LEAVE;
			memcpy(&data[1], &channel_id, sizeof(uint16_t));
			send_raw(data, 1+sizeof(uint16_t));
			n_channels--;
			channel[x] = channel[n_channels];
			return;
		}
	}
}

void Session::ping(const uint64_t time) {
	// 1 minute
	if (last_ping + 60000 <= time) {
		last_ping = time;
		unsigned char value = SERVER_MESSAGE_PING;
		send_raw(&value, 1);
	}
}

uint16_t *Session::get_channels(uint16_t *num) {
	*num = n_channels;
	return channel;
}

uint64_t Session::get_last_login_attempt() const {
	return last_login_attempt;
}
void Session::log_login_attempt(const uint64_t input_time) {
	last_login_attempt = input_time;
}

void Session::sign() {
	is_signed = true;
}
void Session::unsign() {
	is_signed = false;
}
int Session::sign_state() const {
	return is_signed;
}