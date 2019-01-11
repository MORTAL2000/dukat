#include "stdafx.h"
#include <dukat/gamebase.h>
#include <dukat/controller.h>
#include <dukat/devicemanager.h>
#include <dukat/keyboarddevice.h>
#include <dukat/log.h>
#include <dukat/manager.h>
#include <dukat/meshcache.h>
#include <dukat/particlemanager.h>
#include <dukat/scene.h>
#include <dukat/settings.h>
#include <dukat/shadercache.h>
#include <dukat/textmeshinstance.h>
#include <dukat/textmeshbuilder.h>
#include <dukat/texturecache.h>
#include <dukat/timermanager.h>
#include <dukat/window.h>

namespace dukat
{
	GameBase::GameBase(Settings& settings) : Application(settings), controller(nullptr), debug(false)
	{
#ifndef __ANDROID__
		audio_cache = std::make_unique<AudioCache>(settings.get_string("resources.samples"), settings.get_string("resources.music"));
#endif
		shader_cache = std::make_unique<ShaderCache>(settings.get_string("resources.shaders"));
		texture_cache = std::make_unique<TextureCache>(settings.get_string("resources.textures"));
		mesh_cache = std::make_unique<MeshCache>();
		add_manager<ParticleManager>();
		add_manager<TimerManager>();
		add_manager<AnimationManager>();
		add_manager<UIManager>();
		// TODO: need to rebind when devices change
		device_manager->active->on_press(InputDevice::VirtualButton::Debug1, std::bind(&GameBase::toggle_debug, this));
		get<TimerManager>()->create_timer(1.0f, std::bind(&GameBase::update_debug_text, this), true);
	}

	GameBase::~GameBase(void)
	{
		device_manager->active->unbind(InputDevice::VirtualButton::Debug1);
	}

	void GameBase::handle_event(const SDL_Event& e)
	{
		Application::handle_event(e);
		if (controller != nullptr)
			controller->handle_event(e);
	}

	void GameBase::handle_keyboard(const SDL_Event& e)
	{
		Application::handle_keyboard(e);
		if (controller != nullptr)
			controller->handle_keyboard(e);
	}

	void GameBase::update(float delta)
	{
		// Scene first so that managers can operate on updated properties.
		scene_stack.top()->update(delta);
		for (auto& it : managers)
		{
			if (it.second->is_enabled())
				(it.second)->update(delta);
		}
	}

	void GameBase::render(void)
	{
		scene_stack.top()->render();
	}

	void GameBase::toggle_debug(void)
	{
		debug = !debug;
		trigger(Message{Events::ToggleDebug});
	}

	std::unique_ptr<TextMeshInstance> GameBase::build_text_mesh(Texture* texture, ShaderProgram* sp, float size)
	{
		TextMeshBuilder mb;
		auto mesh_instance = std::make_unique<TextMeshInstance>(mb.build_text_mesh());
		mesh_instance->transform.update();
		mesh_instance->set_texture(texture);
		mesh_instance->set_program(sp);
		mesh_instance->set_size(size);
		return mesh_instance;
	}

	void GameBase::add_scene(const std::string& id, std::unique_ptr<Scene> scene)
	{
		scenes[id] = std::move(scene);
	}

	void GameBase::push_scene(const std::string& id)
	{
		if (scenes.count(id) == 0)
		{
			log->warn("Failed to push scene: {}", id);
			return;
		}

		if (!scene_stack.empty())
		{
			scene_stack.top()->deactivate();
		}
		auto scene = scenes.at(id).get();
		scene_stack.push(scene);
		scene->activate();
		// Reset last update whenever scene changes.
		last_update = SDL_GetTicks();
	}

	void GameBase::pop_scene(void)
	{
		if (!scene_stack.empty())
		{
			scene_stack.top()->deactivate();
			scene_stack.pop();
			if (!scene_stack.empty())
			{
				scene_stack.top()->activate();
				// Reset last update whenever scene changes.
				last_update = SDL_GetTicks();
			}
		}
	}

	Scene* GameBase::get_scene(const std::string& id) const
	{
		if (scenes.count(id) > 0)
		{
			return scenes.at(id).get();
		}
		else
		{
			return nullptr;
		}
	}
}