#pragma once
#include <memory>
#include "../../util/avatarmanager/avatarmanager.h"

namespace overlay {
	void load_interface();
	inline bool visible = false;

	inline std::unique_ptr<AvatarManager> avatar_manager;
	inline std::vector<uint64_t> last_known_players; 

	void initialize_avatar_system();
	void update_avatars();
	void update_lobby_players(); 	
	AvatarManager* get_avatar_manager();
	void cleanup_avatar_system();
}

void render_watermark();
void force_rescan();


