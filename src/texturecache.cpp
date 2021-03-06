#include "stdafx.h"
#include <dukat/log.h>
#include <dukat/surface.h>
#include <dukat/texturecache.h>
#include <dukat/dds.h>

namespace dukat
{
	TextureCache::TextureCache(const std::string& resource_dir) : resource_dir(resource_dir), hflip(false), vflip(false), last_id(0)
	{
	}

	TextureCache::~TextureCache(void)
	{
		free_all();
	}

	Surface* TextureCache::load_surface(const std::string& filename, bool hflip, bool vflip)
	{
		if (surfaces.count(filename) == 0)
		{
			auto fqn = resource_dir + "/" + filename;
			auto surface = Surface::from_file(fqn);
			// Perform necessary conversion
 			switch (surface->get_surface()->format->format)
			{
				// 8 bit
			//case SDL_PIXELFORMAT_INDEX8:
			//	logger << "Unexpected 8 bit pixel format, attempting to convert to RGB888." << std::endl;
			//	surface->convert_format(SDL_PIXELFORMAT_RGB888);
			//	break;
				// 24 bit
			case SDL_PIXELFORMAT_BGR24:
			case SDL_PIXELFORMAT_BGR888:
				log->warn("Unexpected 24 bit pixel format, attempting to convert to RGB888.");
				surface->convert_format(SDL_PIXELFORMAT_RGB888);
				break;
				// 32 bit
			case SDL_PIXELFORMAT_ARGB8888:
				log->warn("Unexpected 32 bit pixel format, attempting to convert to RGB888.");
				surface->convert_format(SDL_PIXELFORMAT_RGBA8888);
				break;
			}
			if (hflip)
			{
				// Flip image from SDL to OpenGL orientation
				surface->flip_horizontal();
			}
			if (vflip)
			{
				surface->flip_vertical();
			}
			surfaces[filename] = std::move(surface);
		}
		return surfaces[filename].get();
	}

	std::unique_ptr<Texture> TextureCache::load(const std::string& filename, TextureFilterProfile profile)
	{
		log->debug("Loading texture [{}]", filename);
		auto surface = load_surface(filename, hflip, vflip);
		log->trace("Created {}x{} {} surface.", surface->get_width(), surface->get_height(),
			SDL_GetPixelFormatName(surface->get_surface()->format->format));
		return std::make_unique<Texture>(*surface, profile);
	}

	Texture* TextureCache::get(const std::string& filename, TextureFilterProfile profile)
	{
		const auto id = compute_hash(filename);
		if (textures.count(id) == 0)
		{
			auto ext = get_extension(filename);
			// TODO: base this on file header not extension (use streams if possible)
			if (ext == "dds" || ext == "tga" || ext == "TGA")
			{
#ifdef __ANDROID__
				throw std::runtime_error("Unsupported texture format.");
#else
				textures[id] = load_dds(resource_dir + "/" + filename);
#endif
			}
			else
			{
				textures[id] = load(filename, profile);
			}
		}
		return textures[id].get();
	}

	Texture* TextureCache::get(uint32_t id)
	{
		if (textures.count(id) == 0)
		{
			return nullptr;
		}
		else
		{
			return textures[id].get();
		}
	}

	Texture* TextureCache::find_by_texture_id(const TextureId id) const
	{
		for (auto& t : textures)
		{
			if (t.second->id == id)
			{
				return t.second.get();
			}
		}
		return nullptr;
	}

	void TextureCache::free(uint32_t id)
	{
		if (textures.count(id) != 0)
		{
			textures.erase(id);
		}
	}

	void TextureCache::free_all(void)
	{
		log->debug("Freeing {} texture(s)...", textures.size());
		textures.empty();
	}
}