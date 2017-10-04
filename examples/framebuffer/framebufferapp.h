#pragma once

#include <memory>
#include <dukat/dukat.h>

namespace dukat
{
	class FramebufferScene : public Scene, public Controller
	{
	private:
		static constexpr int texture_size = 256;

		Game3* game;
		MeshGroup overlay_meshes;
		MeshInstance* quad_mesh;
		MeshInstance* info_mesh;

        std::unique_ptr<FrameBuffer> fbo; // frame buffer object
        ShaderProgram* fb_program; // program used during framebuffer render pass
        std::unique_ptr<MeshData> fb_quad; // quad mesh used during framebuffer render pass

	public:
		FramebufferScene(Game3* game);
		~FramebufferScene(void) { }

		void update(float delta);
		void update_framebuffer(float delta);
		void render(void);
		void handle_keyboard(const SDL_Event& e);
	};
}