#pragma once

#include <array>
#include <memory>

#include "buffers.h"
#include "color.h"
#include "mathutil.h"
#include "meshdata.h"
#include "mesh.h"
#include "texture.h"
#include "vector2.h"

namespace dukat
{
	class Game3;
	class MeshInstance;
	class Renderer3;

	// Wave Mesh implementation. (Based on Chapter 1 of GPU Gems)
	// Wave effect is generated by a combination of vertex shader displacement (geo waves)
	// and wave texture rendered via framebuffer (tex waves).
    class WaveMesh : public Mesh
    {
    private:
		static constexpr float grav_constant = 30.0f;
		static constexpr int num_geo_waves = 4;
		static constexpr int num_bump_per_pass = 4;
		static constexpr int num_tex_waves = 16;
		static constexpr int num_bump_passes = num_tex_waves / num_bump_per_pass;
		static constexpr int texture_size = 256;

		struct GeoWaveDesc
		{
			float phase;
			float amp;
			float len;
			float freq;
			float fade;
			float dirx;
			float diry;
			float dirz; // padding
		};

		struct GeoState
		{
			float chop;
			float angle_deviation;
			Vector2 wind_dir;
			float max_length;
			float min_length;
			float amp_over_len;
			float spec_atten;
			float spec_end;
			float spec_trans;
			float env_radius;
			float water_level;
			int trans_idx;
			float trans_del;
		};

		struct TexWaveDesc
		{
			float phase;
			float amp;
			float len;
			float speed;
			float freq;
			Vector2 dir;
			Vector2 rot_scale;
			float fade;
		};

		struct TexState
		{
			float noise;
			float chop;
			float angle_deviation;
			Vector2 wind_dir;
			float max_length;
			float min_length;
			float amp_over_len;
			float ripple_scale;
			float speed_deviation;
			int trans_idx;
			float trans_del;
		};

		Game3* game;
		Texture* env_map; // Environment cube map used for reflection
		Texture* elev_map; // Elevation sampler used to determine wave height above ground
		Color water_tint; // Base water color

		// Geo waves
		GeoState geo_state;
		std::array<GeoWaveDesc, num_geo_waves> geo_waves;
		std::unique_ptr<MeshData> grid_mesh; // grid mesh for geometry waves
		ShaderProgram* grid_program; // program used for geometry waves

		// Tex waves
		TexState tex_state;
		std::array<TexWaveDesc, num_tex_waves> tex_waves;
		std::unique_ptr<FrameBuffer> fbo; // frame buffer object
        ShaderProgram* fb_program; // program used during framebuffer render pass
        std::unique_ptr<MeshData> fb_quad; // quad mesh used during framebuffer render pass
		std::unique_ptr<Texture> fb_texture; // texture written to during framebuffer pass
		std::unique_ptr<Texture> cos_texture; // 1D cosine lookup table
		std::unique_ptr<Texture> noise_texture; // 2D noise texture

		// Creates cosine lookup table.
		void init_cos_table(void);
		void init_geo_wave(int i);
		void init_tex_wave(int i);
		void update_framebuffer(Renderer* renderer);

	public:
		const int grid_size; // width / length of wave grid
		float tile_spacing; // size of grid tiles
		float scale_factor; // elevation scale factor (of elevation map)

        WaveMesh(Game3* game, int size);
		~WaveMesh(void) { }

		void reset_state(void);
		void init_waves(void);
        void update(float delta);
        void render(Renderer* renderer);

		// Water level
		void set_water_level(float value) { geo_state.water_level = value; }
		float get_water_level(void) const { return geo_state.water_level; }
		// Geo wave height
		void set_geo_wave_height(float value) { geo_state.amp_over_len = value; clamp(geo_state.amp_over_len, 0.0f, 0.1f); }
		float get_geo_wave_height(void) const { return geo_state.amp_over_len; }
		// Geo wave choppiness
		void set_geo_wave_chop(float value) { geo_state.chop = value; clamp(geo_state.chop, 0.0f, 4.0f); }
		float get_geo_wave_chop(void) const { return geo_state.chop; }
		// Geo angle deviation
		void set_geo_angle_dev(float value) { geo_state.angle_deviation = value; clamp(geo_state.angle_deviation, 0.0f, 180.0f); }
		float get_geo_angle_dev(void) const { return geo_state.angle_deviation; }
		// Geo env radius
		void set_geo_env_radius(float value) { geo_state.env_radius = value; clamp(geo_state.env_radius, 100.0f, 10000.0f); }
		float get_geo_env_radius(void) const { return geo_state.env_radius; }
		// Texture wave height
		void set_tex_wave_height(float value) { tex_state.amp_over_len = value; clamp(tex_state.amp_over_len, 0.0f, 0.5f); }
		float get_tex_wave_height(void) const { return tex_state.amp_over_len; }
		// Texture scale
		void set_tex_scale(float value) { tex_state.ripple_scale = value; clamp(tex_state.ripple_scale, 5.0f, 50.0f); }
		float get_tex_scale(void) const { return tex_state.ripple_scale; }
		// Texture noise
		void set_tex_noise(float value) { tex_state.noise = value; clamp(tex_state.noise, 0.0f, 1.0f); }
		float get_tex_noise(void) const { return tex_state.noise; }
		// Texture angle deviation
		void set_tex_angle_dev(float value) { tex_state.angle_deviation = value; clamp(tex_state.angle_deviation, 0.0f, 180.0f); }
		float get_tex_angle_dev(void) const { return tex_state.angle_deviation; }
		// Wind direction (geo and texture)
		void set_wind_dir(const Vector2& wind_dir) { geo_state.wind_dir = wind_dir; tex_state.wind_dir = wind_dir; }
		// Wave tint
		void set_water_tint(const Color& color) { this->water_tint = color; } 
		Color get_water_tint(void) const { return water_tint; } 
		
		// Texture access
		Texture* get_wave_texture(void) const { return fb_texture.get(); }
		void set_env_map(Texture* env_map) { this->env_map = env_map; }
		void set_elevation_map(Texture* elev_map) { this->elev_map = elev_map; }

		// Samples normalized elevation at a given set of coordinates.
		float sample(float x, float y) const;
	};
}