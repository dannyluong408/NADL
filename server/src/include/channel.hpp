/*
class Channel {
private:
	char name[64];
	uint8_t flags, game_id;
	bool is_league;

	struct Member {
		uint32_t id;
		uint8_t permission;
	};
	Member *members;
	uint32_t n_members;
	
public:
	Channel();
	uint8_t get_permission(const uint32_t user_id);
	void set_permission(const uint32_t user_id, const uint8_t permission_level);
	char *get_name();
};
*/