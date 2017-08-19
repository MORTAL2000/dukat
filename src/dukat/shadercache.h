#pragma once

#include <string>
#include <map>
#include <memory>

#include "shaderprogram.h"

namespace dukat
{
	class ShaderCache
	{
	private:
		static constexpr const char* pre_include = "#include";
		std::map<std::string,std::unique_ptr<ShaderProgram>> programs;
		std::map<std::string, std::string> sources;
		const std::string resource_dir;
		// Loads the source of a shader from file.
		std::string load_shader(const std::string& filename);
		// Builds a new shader from a source file.
		GLuint build_shader(GLenum shaderType, const std::string& filename);
		// Builds a new program from a set of source files.
		GLuint build_program(const std::string& vertex_file, const std::string& fragement_file,
			const std::string& geometry_file = "");

	public:
		ShaderCache(const std::string& resource_dir) : resource_dir(resource_dir) { }
		~ShaderCache(void) { }

		// Returns a program for a set of shaders. If necessary, will create the program.
		ShaderProgram* get_program(const std::string& vertex_file, const std::string& fragement_file,
			const std::string& geometry_file = "");
	};
}

