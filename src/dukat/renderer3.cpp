#include "stdafx.h"
#include "renderer3.h"
#include "buffers.h"
#include "log.h"
#include "meshbuilder2.h"
#include "perfcounter.h"
#include "shadercache.h"
#include "shaderprogram.h"
#include "mesh.h"
#include "meshgroup.h"

namespace dukat
{
	Renderer3::Renderer3(Window* window, ShaderCache* shader_cache, TextureCache* textures)
		: Renderer(window, shader_cache), effects_enabled(true)
	{
		// Enable transparency
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// Enable depth buffer
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		light.position = { 0.0f, 0.0f, 100.0f };
		light.color = { 1.0f, 1.0f, 0.66f, 1.0f };
		light.attenuation = 0.00001f; // TODO: investigate non-linear attenuation
		light.ambient = 0.5f;

		fb0 = std::make_unique<FrameBuffer>(window->get_width(), window->get_height(), true);
		fb1 = std::make_unique<FrameBuffer>(fbo_size, fbo_size, false);
		fb2 = std::make_unique<FrameBuffer>(fbo_size, fbo_size, false);

		MeshBuilder2 builder;
		quad = builder.build_textured_quad();
		
		composite_program = shader_cache->get_program("fx_default.vsh", "fx_composite.fsh");
	}

	void Renderer3::switch_fbo(void)
	{
		// set input texture
		glActiveTextureARB(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, frame_buffer->texture);

		if (frame_buffer == fb1.get())
		{
			fb2->bind();
			frame_buffer = fb2.get();
		}
		else
		{
			fb1->bind();
			frame_buffer = fb1.get();
		}

		glClear(GL_COLOR_BUFFER_BIT);
	}

	void Renderer3::render(const std::vector<MeshGroup*>& meshes)
	{
		update_uniforms();

        if (effects_enabled)
		{
			fb0->bind();
			frame_buffer = fb0.get();
		}

		// Scene pass
		glEnable(GL_DEPTH_TEST);
		window->clear();
		for (auto& it : meshes)
		{
			if (it->visible && it->stage == RenderStage::SCENE)
			{
				it->render(this);
			}
		}

		if (effects_enabled)
		{
			// Effects passes
			for (auto it = effects.begin(); it != effects.end(); ++it)
			{
				switch_fbo();
				switch_shader(it->program);
				glUniform1i(it->program->attr(Renderer::uf_tex0), 0);
				for (auto it2 : it->parameters)
				{
					switch (it2.second.count)
					{
					case 1:
						glUniform1fv(it->program->attr(it2.first), 1, it2.second.values);
						break;
					case 2:
						glUniform2fv(it->program->attr(it2.first), 1, it2.second.values);
						break;
					case 3:
						glUniform3fv(it->program->attr(it2.first), 1, it2.second.values);
						break;
					case 4:
						glUniform4fv(it->program->attr(it2.first), 1, it2.second.values);
						break;
					}
				}
				quad->render(it->program);
			}

			// Composite pass
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, window->get_width(), window->get_height());
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glDisable(GL_DEPTH_TEST);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fb0->texture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, frame_buffer->texture);

			switch_shader(composite_program);
			glUniform1i(composite_program->attr(Renderer::uf_tex0), 0);
			glUniform1i(composite_program->attr(Renderer::uf_tex1), 1);
			glUniform1f(composite_program->attr("u_scale"), 0.0f);

			quad->render(composite_program);

			// reset texture units
			glActiveTexture(GL_TEXTURE0);
		}

		// Overlay pass
		glDisable(GL_DEPTH_TEST);
		for (auto& it : meshes)
		{
			if (it->visible && it->stage == RenderStage::OVERLAY)
			{
				it->render(this);
			}
		}

        window->present();
    }

	void Renderer3::update_uniforms(void)
	{
		// Update uniform buffers
		glBindBufferBase(GL_UNIFORM_BUFFER, UniformBuffer::CAMERA, uniform_buffers->buffers[0]);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraTransform3), &camera->transform, GL_STREAM_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, UniformBuffer::LIGHT, uniform_buffers->buffers[1]);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(Light), &light, GL_STREAM_DRAW);
	}

	void Renderer3::add_effect(int index, const Effect3& effect)
	{
		int i = 0;
		for (auto it = effects.begin(); it != effects.end(); ++it, i++)
		{
			if (i == index)
			{
				effects.insert(it, effect);
				return;
			}
		}
		effects.push_back(effect);
	}
}