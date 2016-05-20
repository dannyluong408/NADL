#ifndef __DATA_TYPES_HPP
#define __DATA_TYPES_HPP

#include <stdint.h>

#define NETWORK_PROTOCOL_VERSION 1
#define CONNECT_PORT 8443
#define RECV_MAXLEN 16384

struct __attribute__ ((__packed__)) user_login_msg {
    uint16_t version;
    char username[64];
    /* In the case of a password, auth[] must be zero-padded. */
    char auth[64];
    /* Tokens not currently supported. */
    uint8_t using_token;
    uint8_t region;
};

struct __attribute__ ((__packed__)) token_update {
    char username[64];
    char token[64];
};

struct __attribute__ ((__packed__)) client_settings {
    uint8_t hide_join_leave_messages;
};

struct __attribute__ ((__packed__)) server_connect_msg {
    uint32_t server_version;
    uint64_t current_time;
};

struct __attribute__ ((__packed__)) channel_info {
    /* Channel identifier for chatting and joining. */
    uint16_t channel_id;
    //uint16_t n_channel_members;
    /* User-specific permissions for channel. See: permission_id. This may be overwritten by user_info.type */
    //uint8_t permissions;
    /* Channel flags. See enum channel_flag for more info. */
    uint8_t channel_flags;
    char channel_name[64];
};

struct __attribute__ ((__packed__)) user_info {
    uint16_t channel_id, rating, warn_level;
    uint32_t user_id; // are these used for anything anymore?
    uint8_t permissions;
    char voucher_name[64];
    char username[64];
};

struct __attribute__ ((__packed__)) rank_state {
    uint8_t position;
    uint16_t rating, channel;
    char name[64];
};

struct __attribute__ ((__packed__)) game_state {
    uint16_t league_id;
    uint16_t game_id; // instance id
    char game_name[64];
    char host_name[64];
    int start_time;
    uint8_t n_users;
    uint8_t n_max;
};

/* Chat message header.
 * Message is a null-terminated string of @msg_length bytes appended after this header.
 * Server commands are sent using this header and are interpreted by the server. */
struct __attribute__ ((__packed__)) chat_message_header {
    /* User ID of sender.
     * UINT32_MAX reserved for username SYSTEM.
     * 0 reserved for status messages. */
    uint32_t user_id;
    uint8_t msg_flags;

    uint16_t channel; /* Chat channel. UINT16_MAX is reserved for private messages. */
    uint16_t msg_length; /* msg_length includes the null-terminator. */
};

struct __attribute__ ((__packed__)) chat_user {
    uint32_t user_id;
    uint16_t channel;
};

enum server_message_type {
    SERVER_MESSAGE_AUTH_SUCCESSFUL = 0,
    SERVER_MESSAGE_USER_INFO = 1,

    SERVER_MESSAGE_CHAT_MESSAGE = 3,
    SERVER_MESSAGE_AUTH_ERROR = 4,
    SERVER_MESSAGE_NEW_AUTH_TOKEN = 5,

    SERVER_MESSAGE_PING = 6,
    SERVER_MESSAGE_CONNECT = 7,

    SERVER_MESSAGE_CHANNEL_REMOVE_USER = 8,
    SERVER_MESSAGE_CHANNEL_ADD_USER = 9,
    SERVER_MESSAGE_CHANNEL_UPDATE_USER = 10,

    SERVER_MESSAGE_CHANNEL_JOIN = 11,
    SERVER_MESSAGE_CHANNEL_LEAVE = 12,
    //SERVER_MESSAGE_CHANNEL_UPDATE_CHANINFO = 13,
    SERVER_MESSAGE_CHANNEL_UPDATE_GAMEINFO = 14,
    SERVER_MESSAGE_BAD_AUTH_TOKEN = 15,
    SERVER_MESSAGE_CHANNEL_UPDATE_LEADERBOARDS = 16,

    SERVER_MESSAGE_DISCONNECT = 200, // generic disconnect message
    SERVER_MESSAGE_DISCONNECT_CLIENT_OUT_OF_DATE = 201,
    SERVER_MESSAGE_DISCONNECT_PROTOCOL_ERROR = 202,
    SERVER_MESSAGE_DISCONNECT_SERVER_ERROR = 203,
    SERVER_MESSAGE_DISCONNECT_INVALID_LOGIN = 204,
    SERVER_MESSAGE_DISCONNECT_SERVER_SHUTDOWN = 205,
    SERVER_MESSAGE_DISCONNECT_CONNECTION_TIMEOUT = 206,
    SERVER_MESSAGE_DISCONNECT_USER_LOGGED_IN_ELSEWHERE = 207
};

enum client_message_type {
    CLIENT_MESSAGE_CONNECT = 1,
    CLIENT_MESSAGE_REQUEST_USER_INFO = 2,
    CLIENT_MESSAGE_REQUEST_CHANNEL_INFO = 3,
    CLIENT_MESSAGE_CHAT_MESSAGE = 10,
    CLIENT_MESSAGE_PONG = 11
};

enum channel_permission_id {
    CHANNEL_NO_PERMISSIONS = 0,
    CHANNEL_JOIN_PERMISSIONS = 1,
    CHANNEL_CHAT_PERMISSIONS = 2,
    CHANNEL_SPECTATOR_PERMISSIONS = 3,
    CHANNEL_TRIAL_PERMISSIONS = 4,
    CHANNEL_PLAYER_PERMISSIONS = 5,

    CHANNEL_VOUCHER_PERMISSIONS = 8,
    CHANNEL_MODERATOR_PERMISSIONS = 9,
    CHANNEL_ADMIN_PERMISSIONS = 10,
    CHANNEL_OWNER_PERMISSIONS = 11
};

enum global_permission_levels {
    PERMISSION_DEFAULT = 0,

    PERMISSION_MODERATOR = 2,
    PERMISSION_ADMINISTRATOR = 3
};

enum channel_flag {
    /* The channel is also a league. */
    CHANNEL_FLAG_CHANNEL_IS_LEAGUE = 1,
    /* The channel is listed under public channels. */
    CHANNEL_FLAG_CHANNEL_IS_PUBLIC = 2,
    CHANNEL_FLAG_CHANNEL_IS_GAME = 4,
    CHANNEL_FLAG_CHANNEL_IS_INVITE_ONLY = 8,
};

enum message_flag {
    MSG_FLAG_IMPORTANT = 1, // switch current tab to this tab
    MSG_FLAG_PRINTALL = 2, // print to all chat buffers
};

enum game_ids {
    GAME_DOTA = 0,
    GAME_BLC = 1
};

#endif /* __DATA_TYPES_HPP */
