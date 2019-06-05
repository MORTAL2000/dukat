#pragma once

#include <array>
#include "color.h"
#include "vector2.h"

namespace dukat
{
    class ParticleManager;
    class RenderLayer2;

    // Abstract particle emitter base class.
    struct ParticleEmitter
    {
		struct Recipe
		{
			// Recipe for a particle emitter.
			enum Type
			{
				None,
                Linear,
				Flame,
				Smoke,
				Fountain,
				Explosion
			};

			Type type;
			// Rate of particle emission (particles / second)
			float rate;
			// particle size
			float min_size;
			float max_size;
			// particle ttl
			float min_ttl;
			float max_ttl;
            // particle speed
            Vector2 min_dp;
            Vector2 max_dp;
			// particle colors
			std::array<Color, 4> colors;
            // color reduciton over time
            Color dc;

			Recipe(void) : type(None), rate(0.0f), min_size(0.0f), max_size(0.0f), min_ttl(0.0f), max_ttl(0.0f) { }
			Recipe(Type type) : type(type), rate(0.0f), min_size(0.0f), max_size(0.0f), min_ttl(0.0f), max_ttl(0.0f) { }
			Recipe(Type type, float rate, float min_size, float max_size, float min_ttl, float max_ttl, 
				const Vector2& min_dp, const Vector2& max_dp, const std::array<Color,4> colors, const Color& dc) 
				: type(type), rate(rate), min_size(min_size), max_size(max_size), min_ttl(min_ttl), max_ttl(max_ttl), 
				min_dp(min_dp), max_dp(max_dp), colors(colors), dc(dc) { }
			~Recipe(void) { }

			// Default recipes
			static const Recipe FlameRecipe;
			static const Recipe SmokeRecipe;
			static const Recipe FountainRecipe;
			static const Recipe ExplosionRecipe;
		};

		// Particle recipe
        Recipe recipe;
        // Emitter world pos
        Vector2 pos;
		// Offsets at which to emit particles
		std::vector<Vector2> offsets;
		// offset from pos to horizontal mirror axis
		float mirror_offset;
		// Layer to add particles to
        RenderLayer2* target_layer;
		// Time-to-live for the emitter
		float ttl;
		// Age of the emitter
		float age;
		// Accumulator for particles to be emitted
        float accumulator;
		// Will only generate particles if active
		bool active;

        ParticleEmitter(void) : mirror_offset(0.0f), target_layer(nullptr), ttl(0.0f), age(0.0f), accumulator(0.0f), active(true) { }
        ParticleEmitter(const Recipe& recipe) : recipe(recipe), mirror_offset(0.0f), target_layer(nullptr), ttl(0.0f), age(0.0f), accumulator(0.0f), active(true) { }
        virtual ~ParticleEmitter(void) { }

        virtual void update(ParticleManager* pm, float delta) = 0;
    };

    struct CustomEmitter : public ParticleEmitter 
    {
        std::function<void(ParticleEmitter* em, ParticleManager* pm, float delta)> update_func;

        CustomEmitter(void) : update_func(nullptr) { }
        ~CustomEmitter(void) { }

        void update(ParticleManager* pm, float delta) { if (update_func) update_func(this, pm, delta); };
    };

    // LINEAR
    // - particles are created with direction in +/- dp range
    struct LinearEmitter : public ParticleEmitter
    {
        LinearEmitter(void) { }
        LinearEmitter(const Recipe& recipe) : ParticleEmitter(recipe) { }
        ~LinearEmitter(void) { }

        void update(ParticleManager* pm, float delta);
    };

    // FLAME
	// - particles are emitted with upward direction
    // - flame particles have variable ttl, so that we end up with holes
    // - if using multiple emitters, each emitter should have a current 
    //   direction that swings by random amount; that will cause subsequent 
    //   particles to have similar direction
    struct FlameEmitter : public ParticleEmitter
    {
        // max change to angle per second
        float max_change;
        // current angle 
        float angle;

		FlameEmitter(void) : max_change(0.25f), angle(0.0f) { }
		FlameEmitter(const Recipe& recipe) : ParticleEmitter(recipe), max_change(0.25f), angle(0.0f) { }
        ~FlameEmitter(void) { }

        void update(ParticleManager* pm, float delta);
    };

    // SMOKE
    // - upward direction of particles
    // - position of emitter ocilates on the x axis
	// - single color that fades out over time
    struct SmokeEmitter : public ParticleEmitter
    {
        // max change to angle per second
        float max_change;
        // current angle 
        float angle;

        SmokeEmitter(void) : max_change(0.15f), angle(0.0f) { }
        SmokeEmitter(const Recipe& recipe) : ParticleEmitter(recipe), max_change(0.15f), angle(0.0f) { }
        ~SmokeEmitter(void) { }

        void update(ParticleManager* pm, float delta);
    };

    // FOUNTAIN
    // - initial dx, dy
    // - gravity pulls at dy->reduce and ultimately turn around
    struct FountainEmitter : public ParticleEmitter
    {
        // max change to angle per second
        float max_change;
        // current angle 
        float angle;

        FountainEmitter(void) : max_change(0.2f), angle(0.0f) { }
        FountainEmitter(const Recipe& recipe) : ParticleEmitter(recipe), max_change(0.2f), angle(0.0f) { }
        ~FountainEmitter(void) { }

        void update(ParticleManager* pm, float delta);
    };

    // EXPLOSION
    // - burst of particles
    struct ExplosionEmitter : public ParticleEmitter
    {
        // repeat interval
        float repeat_interval;

        ExplosionEmitter(void) : repeat_interval(5.0f) { }
        ExplosionEmitter(const Recipe& recipe) : ParticleEmitter(recipe), repeat_interval(5.0f) { } 
        ~ExplosionEmitter(void) { }

        void update(ParticleManager* pm, float delta);
    };
}