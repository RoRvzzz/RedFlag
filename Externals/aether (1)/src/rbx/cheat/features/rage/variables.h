#include "../../../overlay/ckey/keybind.hpp"
namespace RageGlobals {

	static bool network_spoofing = false;
	static CKeybind network_spoofing_bind = ("keybind");
	static bool network_spoofing_on_join = false;
	static bool network_spoofing_on_leave = false;

	static bool desync_enabled = false;
	static CKeybind desync_key = ("keybinwaddawdd");
	static float desync_strength = 1.0f;
	static float desync_delay = 0.5f;
	static bool desync_jitter = false;
	static bool desync_disable_on_knock = false;
	static bool desync_disable_on_grab = false;
	static bool desync_disable_on_ragdoll = false;
	static bool desync_loop = false;

	static bool hit_notifications = false;
	static bool kill_notifications = false;
	static bool lowhp_alarm = false;
	static float lowhp_threshold = 30.0f;

	static bool auto_weapon_pickup = false;
	static bool auto_ammo_pickup = false;
	static bool auto_revive = false;

	static bool death_logger = false;
	static bool chat_logger = false;
	static bool ss_on_kill = false;
	static bool killer_info_report = false;
	static bool ping_display = false;
	static bool server_age_display = false;
	static bool time_alive_display = false;

	static CKeybind panic_tp_key = ("24323dqwa");
	static CKeybind reset_char_key = ("k2fe2ybind");
	static CKeybind suicide_key = ("keybiawwdsnd");
}