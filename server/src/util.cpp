#include <regex>
#include "server.hpp"

static inline size_t unicode_clen(const unsigned char input) {
	static const char trailing_byte_index[256] = {
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,6,6,6,6
	};
	return trailing_byte_index[input];
}

static inline void left_shift_str(char *str) {
	int len = strlen(str);
	for (int i = 0; i < len; ++i) { // need to shift null-terminator too
		str[i] = str[i+1];
	}
}

std::string Server::sanitize_string(char *msg) {
	int len = strlen(msg);
	// check for invalid unicode
	/*
	for (int x = len-1; x > len-6 && x > 0; --x) {
		if (unicode_clen(msg[x]) > len - x) {
			msg[x] = '\0';
			len = x-1;
			x = len;
		}
	}*/
	// strip double spaces and unprintable characters
	char prev = 0;
	for (int x = 0; x < len; ++x) {
		if ((msg[x] != 0 && msg[x] < 32) || (msg[x] == ' ' && prev == ' ')) {
			left_shift_str(&msg[x]);
			len--;
			x--;
		}
		prev = msg[x];
	}
	std::string str = msg; // convert to std::string for return
	// escape potential user formatting
	for (uint32_t x = 0; x < str.length(); ++x) {
		// this doesnt work
		
		if (str.at(x) == '<') {
			str = str.substr(0, x) + "&lt" + str.substr(x+1, str.length());
			x += 2;
		} else if (str.at(x) == '>') {
			str = str.substr(0, x) + "&gt" + str.substr(x+1, str.length());
			x += 2;
		}
	}
	
	return str;
}

void Server::lower(const char *source, char *output) {
	const int len = strlen(source);
	for (int x = 0; x <= len; ++x) { // copy null terminator
		if (source[x] >= 'A' && source[x] <= 'Z') {
			output[x] = source[x] - 'A' + 'a';
		} else output[x] = source[x];
	}
}