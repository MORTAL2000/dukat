#pragma once

#include "scene.h"

namespace dukat
{
	class Game2;

	class Scene2 : public Scene
	{
	protected:
		Game2* game;
		std::queue<std::function<void(void)>> delayed_actions;

	public:
		Scene2(Game2* game) : game(game) { }
		~Scene2(void) { }

		virtual void update(float delta);
		virtual void render(void);
		void delay_action(const std::function<void(void)>& action) { delayed_actions.push(action); }
	};
}
