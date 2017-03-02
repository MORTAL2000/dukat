#pragma once

namespace dukat
{
    class Surface;
	class ShaderProgram;

    typedef GLuint TextureId;

	enum TextureFilterProfile
	{
		ProfileNearest,
		ProfileLinear,
		ProfileMipMapped,
		ProfileAnisotropic
	};

	struct Texture
	{
		GLenum target;
		TextureId id;
		TextureFilterProfile profile;
		int w, h;

		// Default constructor 
		Texture(void);
		// Creates a new texture with a predefined width and height.
		Texture(int w, int h, TextureFilterProfile profile = ProfileNearest);
		// Creates a new texture from a surface.
		Texture(const Surface& surface, TextureFilterProfile profile = ProfileNearest);
		// Disallow copying
		Texture(const Texture& rhs) = delete;
		~Texture(void);

		void load_data(const Surface& surface, TextureFilterProfile profile = ProfileNearest);
		void bind(GLenum texture, ShaderProgram* program = nullptr);
		void unbind(void);
	};
}