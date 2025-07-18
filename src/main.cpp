#include <Geode/Geode.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "legacy_stdio_definitions.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winspool.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "kernel32.lib")

extern "C" {
void zcblive_on_wgl_swap_buffers(HDC hdc);
void zcblive_initialize();
void zcblive_uninitialize();
void zcblive_on_action(uint8_t button, bool player2, bool push);
void zcblive_on_reset();
void zcblive_set_is_in_level(bool is_in_level);
void zcblive_set_playlayer_time(double time);
void zcblive_on_init(PlayLayer* playlayer);
void zcblive_on_quit();
void zcblive_on_death();
bool zcblive_do_force_player2_sounds();
bool zcblive_do_use_alternate_hook();
void zcblive_on_update(float dt);
}

class GlHook {
public:
    bool m_inited = false;
    HDC m_deviceContext;

    void setup(CCEGLView* view) {
        if (m_inited)
            return;

        auto* glfwWindow = view->getWindow();
        m_deviceContext = *reinterpret_cast<HDC*>(
            reinterpret_cast<uintptr_t>(glfwWindow) + 632);
        m_inited = true;
    }
};

class $modify(CCEGLView) {
    void swapBuffers() {
        static GlHook glHook = GlHook();
        glHook.setup(this);

        zcblive_on_wgl_swap_buffers(glHook.m_deviceContext);
        CCEGLView::swapBuffers();
    }
};

void onUnload() {
    zcblive_uninitialize();
}

$on_mod(Loaded) {
    zcblive_initialize();
    std::atexit(onUnload);
}

template <class R, class T> inline R& from(T base, intptr_t offset) {
    return *reinterpret_cast<R*>(reinterpret_cast<uintptr_t>(base) + offset);
}

inline double getTime() {
    auto playLayer = PlayLayer::get();
    return playLayer ? from<double>(playLayer, 968) : 0.0;
}

void handleAction(int button, bool player1, bool push, PlayLayer* playLayer) {
    zcblive_on_action(static_cast<uint8_t>(button),
                      !player1 && playLayer &&
                          (playLayer->m_levelSettings->m_twoPlayerMode ||
                           zcblive_do_force_player2_sounds()),
                      push);
}

class $modify(PlayerObject) {
	void handlePushOrRelease(PlayerButton button, bool push) {
		auto playLayer = PlayLayer::get();
		if (playLayer == nullptr && LevelEditorLayer::get() == nullptr) {
			zcblive_set_is_in_level(false);
			return;
		}
		if ((button == PlayerButton::Left || button == PlayerButton::Right) && !this->m_isPlatformer) {
			return;
		}

		zcblive_set_is_in_level(true);
		zcblive_set_playlayer_time(getTime());

		bool player1 = playLayer && this == playLayer->m_player1;
		handleAction(static_cast<int>(button), player1, push, playLayer);
	}

	bool pushButton(PlayerButton button) {
		if (zcblive_do_use_alternate_hook()) {
			handlePushOrRelease(button, true);
		}
		return PlayerObject::pushButton(button);
	}

	bool releaseButton(PlayerButton button) {
		if (zcblive_do_use_alternate_hook()) {
			handlePushOrRelease(button, false);
		}
		return PlayerObject::releaseButton(button);
	}
};

class $modify(GJBaseGameLayer) {
	void handleButton(bool push, int button, bool player1) {
		if (zcblive_do_use_alternate_hook()) {
			GJBaseGameLayer::handleButton(push, button, player1);
			return;
		}
		zcblive_set_is_in_level(true);
		zcblive_set_playlayer_time(getTime());

		auto playLayer = PlayLayer::get();
		bool is_invalid = playLayer && ((button == 2 || button == 3)
                        && !(player1 && playLayer->m_player1->m_isPlatformer)
                        && !(!player1 && playLayer->m_player2->m_isPlatformer));
		if (!is_invalid) {
			handleAction(button, player1, push, playLayer);
		}
		
		GJBaseGameLayer::handleButton(push, button, player1);
	}

	void update(float dt) {
		zcblive_on_update(dt);
		GJBaseGameLayer::update(dt);
		zcblive_set_playlayer_time(getTime());
	}

	bool init() {
		return GJBaseGameLayer::init();
	}
};

class $modify(PlayLayer) {
	void onQuit() {
		zcblive_on_quit();
		PlayLayer::onQuit();
	}

	void resetLevel() {
		zcblive_on_reset();
		PlayLayer::resetLevel();
	}

	void destroyPlayer(PlayerObject* player, GameObject* hit) {
		PlayLayer::destroyPlayer(player, hit);
		if (player->m_isDead) {
			zcblive_on_death();
		}
	}
};

class $modify(LevelEditorLayer) {
	bool init(GJGameLevel* level, bool something) {
		zcblive_on_init(nullptr);
		return LevelEditorLayer::init(level, something);
	}
};
