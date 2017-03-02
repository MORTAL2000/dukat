// terrain.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "terrainapp.h"

#include <dukat/clipmap.h>
#include <dukat/devicemanager.h>
#include <dukat/firstpersoncamera3.h>
#include <dukat/fixedcamera3.h>
#include <dukat/heightmap.h>
#include <dukat/inputdevice.h>
#include <dukat/log.h>
#include <dukat/meshbuilder2.h>
#include <dukat/meshbuilder3.h>
#include <dukat/renderer3.h>
#include <dukat/settings.h>
#include <dukat/sysutil.h>
#include <dukat/vertextypes3.h>

namespace dukat
{
	const Vector3 camera_offset(0.0f, 200.0f, 0.0f);

	void Game::init(void)
	{
		Game3::init();

		level_size = settings.get_int("renderer.terrain.size");
		max_levels = settings.get_int("renderer.terrain.levels");

		renderer->disable_effects();
		// Sky Blue Sky
		glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
		// Directional Light (light direction stored as position)
		auto& dir_light = renderer->get_light(0);
		dir_light.position = { 0.0f, -0.5f, 0.5f }; 
		dir_light.position.normalize();

		object_meshes.stage = RenderStage::SCENE;
		object_meshes.mat_model.identity();
		object_meshes.visible = true;

		// Origin
		dukat::MeshBuilder3 builder3;
		mesh_cache->put("default-axis", builder3.build_axis());
		auto mi = object_meshes.create_instance();
		mi->set_mesh(mesh_cache->get("default-axis"));
		mi->set_program(shader_cache->get_program("sc_color.vsh", "sc_color.fsh"));
		mi->transform.position.y = 0.5f;
		
		// Observer
		mesh_cache->put("observer", builder3.build_sphere(16, 16));
		observer_mesh = object_meshes.create_instance();
		observer_mesh->set_mesh(mesh_cache->get("observer"));
		observer_mesh->set_program(shader_cache->get_program("sc_texture.vsh", "sc_texture.fsh"));
		observer_mesh->set_texture(texture_cache->get("blank.png"));
		Material m;
		m.ambient = { 1.0f, 0.0f, 1.0f, 1.0f };
		observer_mesh->set_material(m);
		// height map always starts at 0 / 0 for lower left corner of inner level
		observer_mesh->transform.position = { 0.5f * (float)(level_size + 2), 10.0f, 0.5f * (float)(level_size + 2) } ;

		load_pugetsound();

		overlay_meshes.stage = RenderStage::OVERLAY;
		overlay_meshes.visible = true;
		overlay_meshes.mat_model.identity();

		auto info_text = create_text_mesh(1.0f / 20.0f);
		info_text->transform.position = { -1.5f, -0.5f, 0.0f };
		std::stringstream ss;
		ss << "<F1> Toggle Wirframe" << std::endl
		   << "<F2> Toggle Culling" << std::endl
		   << "<F3> Toggle Blending" << std::endl
		   << "<F4> Toggle Stitching" << std::endl
		   << "<F5> Toggle Normals" << std::endl
		   << "<F11> Toggle Info" << std::endl
		   << "<1,2> Switch Terain" << std::endl
		   << "<WASD> Move Camera" << std::endl
		   << "<QE> Change altitude" << std::endl
			<< std::endl;
		info_text->set_text(ss.str());
		info_text->transform.update();
		info_mesh = overlay_meshes.add_instance(std::move(info_text));

		debug_meshes.stage = RenderStage::OVERLAY;
		debug_meshes.visible = debug;
		debug_meshes.mat_model.identity();

		auto debug_text = create_text_mesh(1.0f / 20.0f);
		debug_text->transform.position = { -1.0f, 1.0, 0.0f };
		debug_text->transform.update();
		debug_meshes.add_instance(std::move(debug_text));

		MeshBuilder2 builder2;
		display_level = 0;
		auto quad = mesh_cache->put("quad", builder2.build_textured_quad({ 0.0f, 1.0f, 1.0f, 0.0f}));
		elevation_mesh = debug_meshes.create_instance();
		elevation_mesh->set_mesh(quad);
		elevation_mesh->set_program(shader_cache->get_program("sc_ui_texture.vsh", "sc_clipmap_elevation_texture.fsh"));
		texture = std::make_unique<Texture>();
		texture->id = clip_map->get_elevation_map()->id;
		texture->target = GL_TEXTURE_2D_ARRAY;
		elevation_mesh->set_texture(texture.get());
		debug_mat.custom.r = (float)display_level;
		elevation_mesh->set_material(debug_mat);
		elevation_mesh->transform.position = { 1.0f, -0.5f, 0.0f };
		elevation_mesh->transform.scale = { 0.3f, 0.3f, 1.0f };
		elevation_mesh->transform.update();

		switch_to_first_person_camera();
	}

	void Game::load_mtrainier(void)
	{
		// Mt Rainier data set is 10m horizontal resolution, 102.4m vertical for every 0.1f.
		// Note: the data source acknowledges that the data is "squised" when the max range > 1024, so we 
		// stretch it by a factor of 2.
		height_map = std::make_unique<HeightMap>(max_levels, "../assets/heightmaps/mt_rainier_1k.png", 2.0f * 102.4f);
		clip_map = std::make_unique<ClipMap>(this, max_levels, level_size, height_map.get());
		clip_map->set_program(shader_cache->get_program("sc_clipmap.vsh", "sc_clipmap.fsh"));
		switch_to_first_person_camera();
	}

	void Game::load_pugetsound(void)
	{
		// Puget sound data set: 160m horizontal resolution, 0.1m vertical for every 1/65536
		height_map = std::make_unique<HeightMap>(max_levels, "../assets/heightmaps/ps_elevation_1k.png", 0.1f * 65536.0f / 160.0f);
		//height_map = std::make_unique<HeightMap>(max_levels, "../assets/heightmaps/ps_elevation_4k.png", 0.1f * 65536.0f / 40.0f);
		clip_map = std::make_unique<ClipMap>(this, max_levels, level_size, height_map.get());
		clip_map->set_program(shader_cache->get_program("sc_clipmap.vsh", "sc_clipmap.fsh"));
		switch_to_first_person_camera();
	}

	void Game::switch_to_first_person_camera(void)
    {
		observer_mesh->visible = false;
		first_person_camera = true;
		direct_camera_control = true;
		auto camera = std::make_unique<FirstPersonCamera3>(window.get(), this);
		camera->transform.position = observer_mesh->transform.position;
		camera->set_vertical_fov(settings.get_float("camera.fov"));
		camera->set_clip(settings.get_float("camera.nearclip"), settings.get_float("camera.farclip"));
		camera->refresh();
		camera->set_movement_speed(10.0f);
		renderer->set_camera(std::move(camera));
    }

    void Game::switch_to_fixed_camera(void)
    {
		observer_mesh->visible = true;
		first_person_camera = false;
		direct_camera_control = false;
		Vector3 target(observer_mesh->transform.position.x, 0.0f, observer_mesh->transform.position.z);
        auto camera = std::make_unique<FixedCamera3>(window.get(), target + camera_offset, target, Vector3::unit_z);
        camera->set_vertical_fov(settings.get_float("camera.fov"));
        camera->set_clip(settings.get_float("camera.nearclip"), settings.get_float("camera.farclip"));
        camera->refresh();
        renderer->set_camera(std::move(camera));
    }

	void Game::handle_keyboard(const SDL_Event& e)
	{
		switch (e.key.keysym.sym)
		{
		case SDLK_F1:
			clip_map->wireframe = !clip_map->wireframe;
			break;		
		case SDLK_F2:
			clip_map->culling = !clip_map->culling;
			break;
		case SDLK_F3:
			clip_map->blending = !clip_map->blending;
			break;
		case SDLK_F4:
			clip_map->stitching = !clip_map->stitching;
			break;
		case SDLK_F5:
			clip_map->lighting = !clip_map->lighting;
			break;
		case SDLK_F11:
			info_mesh->visible = !info_mesh->visible;
			break;

		case SDLK_1:
			load_mtrainier();
			break;
		case SDLK_2:
			load_pugetsound();
			break;
		case SDLK_h: // show heightmap texture
			texture->id = clip_map->get_elevation_map()->id;
			elevation_mesh->set_program(shader_cache->get_program("sc_ui_texture.vsh", "sc_clipmap_elevation_texture.fsh"));
			break;
		case SDLK_n: // show normalmap texture 
			texture->id = clip_map->get_normal_map()->id;
			elevation_mesh->set_program(shader_cache->get_program("sc_ui_texture.vsh", "sc_clipmap_normal_texture.fsh"));
			break;
		case SDLK_COMMA:
			display_level = std::max(0, display_level - 1);
			debug_mat.custom.r = (float)display_level;
			elevation_mesh->set_material(debug_mat);
			break;
		case SDLK_PERIOD:
			display_level = std::min(max_levels - 1, display_level + 1);
			debug_mat.custom.r = (float)display_level;
			elevation_mesh->set_material(debug_mat);
			break;

		case SDLK_q:
			{
				auto cam = renderer->get_camera();
				cam->transform.position -= cam->transform.up;	
			}
			break;
		case SDLK_e:
			{
				auto cam = renderer->get_camera();
				cam->transform.position += cam->transform.up;	
			}
			break;

		case SDLK_c:
			if (first_person_camera)
			{
				switch_to_fixed_camera();
			}
			else
			{
				switch_to_first_person_camera();
			}
			break;
		case SDLK_v:
			direct_camera_control = !direct_camera_control;
			break;
			
		default:
			Game3::handle_keyboard(e);
		}
	}

	void Game::update(float delta)
	{
		Game3::update(delta);
		object_meshes.update(delta);
		debug_meshes.update(delta);
		if (first_person_camera)
		{
			auto cam = renderer->get_camera();
			if (direct_camera_control)
			{
				observer_mesh->transform.position = cam->transform.position;
				observer_mesh->transform.dir = cam->transform.dir;
			}
		}
		else
		{
			auto dev = device_manager->active;

			auto speed = 10.0f * delta;
			observer_mesh->transform.position += 
				dev->ly * speed * observer_mesh->transform.dir - dev->lx * speed * observer_mesh->transform.left;
			auto cam = dynamic_cast<FixedCamera3*>(renderer->get_camera());
			cam->transform.position = observer_mesh->transform.position + camera_offset;
			cam->look_at = observer_mesh->transform.position;
		}
		clip_map->update(delta, observer_mesh->transform.position);
	}

	void Game::render(void)
	{
		std::vector<Renderable*> meshes;
		meshes.push_back(&debug_meshes);
		meshes.push_back(&object_meshes);
		meshes.push_back(&overlay_meshes);
		meshes.push_back(clip_map.get());
		renderer->render(meshes);
	}

	void Game::toggle_debug(void)
	{
		Game3::toggle_debug();
		debug_meshes.visible = !debug_meshes.visible;
		if (debug)
		{
			glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
			clip_map->set_program(shader_cache->get_program("sc_clipmap.vsh", "sc_clipmap_debug.fsh"));
		}
		else
		{
			glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
			clip_map->set_program(shader_cache->get_program("sc_clipmap.vsh", "sc_clipmap.fsh"));
		}
	}

	void Game::update_debug_text(void)
	{
		auto cam = renderer->get_camera();
		auto debug_text = dynamic_cast<TextMeshInstance*>(debug_meshes.get_instance(0));
		std::stringstream ss;
		ss << "WIN: " << window->get_width() << "x" << window->get_height()
			<< " FPS: " << get_fps()
			<< " MESH: " << dukat::perfc.avg(dukat::PerformanceCounter::MESHES)
			<< " VERT: " << dukat::perfc.avg(dukat::PerformanceCounter::VERTICES) << std::endl
			<< "FBO: " << dukat::perfc.avg(dukat::PerformanceCounter::FRAME_BUFFERS) 
			<< " CULL: " << clip_map->culling << " BLND: " << clip_map->blending << " LIGH: " << clip_map->lighting
			<< std::endl;
		debug_text->set_text(ss.str());
	}
}

int main(int argc, char** argv)
{
	try
	{
		std::string config = "../assets/terrain.ini";
		if (argc > 1)
		{
			config = argv[1];
		}
		dukat::Settings settings(config);
		dukat::Game app(settings);
		return app.run();
	}
	catch (const std::exception& e)
	{
		dukat::logger << "Application failed with error." << std::endl << e.what() << std::endl;
		return -1;
	}
	return 0;
}