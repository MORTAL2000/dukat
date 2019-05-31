#include "stdafx.h"
#include <dukat/mathutil.h>
#include <dukat/particleemitter.h>
#include <dukat/particle.h>
#include <dukat/particlemanager.h>
#include <dukat/renderlayer2.h>

namespace dukat
{
	const ParticleEmitter::Recipe ParticleEmitter::Recipe::FlameRecipe{
		ParticleEmitter::Recipe::Type::Flame, 1.0f, 6.0f, 1.0f, 5.0f,
		Vector2{ 0, 25 }, Vector2{ 0, 40 }, 	// horizontal speed not used 
		{ 
			Color{ 1.0f, 1.0f, 0.0f, 1.0f },	// Center color
			Color{ 0.0f, 0.25f, 0.0f, 0.0f },	// Color reduction as we move away from center
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f }		// Not used
		},
		Color{ 0.0f, -0.5f, 0.0f, -0.05f }		// Color reduction over time
	};

	const ParticleEmitter::Recipe ParticleEmitter::Recipe::SmokeRecipe{
		ParticleEmitter::Recipe::Type::Smoke, 4.f, 8.f, 2.f, 6.f,
		Vector2{ 0, 15 }, Vector2{ 0, 25 },		// horizontal speed not used 
		{
			Color{ 1.0f, 1.0f, 1.0f, 1.0f },	// Smoke color
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f }		// Not used
		},
		Color{ 0.0f, 0.0f, 0.0f, -0.5f }		// Color reduction over time
	};

	const ParticleEmitter::Recipe ParticleEmitter::Recipe::FountainRecipe{
		ParticleEmitter::Recipe::Type::Fountain, 1.f, 4.f, 4.f, 6.f,
		Vector2{ 0, 40 }, Vector2{ 0, 50 }, 	// horizontal speed not used 
		{
			Color{ 0.47f, 0.945f, 1.0f, 0.8f }, // Each color picked randomly
			Color{ 0.0f, 0.8f, 0.976f, 0.866f },
			Color{ 0.0f, 0.596f, 0.862f, 0.933f },
			Color{ 0.0f, 0.411f, 0.666f, 1.0f }
		},
		Color{ 0.0f, 0.0f, 0.0f, -0.05f }		// Color reduction over time
	};

	const ParticleEmitter::Recipe ParticleEmitter::Recipe::ExplosionRecipe{
		ParticleEmitter::Recipe::Type::Explosion, 1.f, 6.f, 1.f, 5.f,
		Vector2{ 0, 25 }, Vector2{ 0, 35 },		// horizontal speed not used 
		{
			Color{ 1.0f, 1.0f, 0.0f, 1.0f },	// Initial color
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f }		// Not used
		},
		Color{ 0.0f, -0.5f, 0.0f, -0.1f }		// Color reduction over time
	};

	void LinearEmitter::update(ParticleManager* pm, float delta)
	{
        accumulator += rate * delta;
        if (accumulator < 1.0f || target_layer == nullptr)
            return;

		const auto offset_count = offsets.size();
        while (accumulator >= 1.0f)
        {
			auto p = pm->create_particle();
			p->ry = pos.y + mirror_offset;
			
			if (offset_count == 0)
				p->pos = pos;
			else
				p->pos = pos + offsets[rand() % offset_count];

			p->dp = Vector2::random(recipe.min_dp, recipe.max_dp);
            p->size = randf(recipe.min_size, recipe.max_size);
			p->color = recipe.colors[randi(0, recipe.colors.size())];
			p->dc = recipe.dc;
            p->ttl = randf(recipe.min_ttl, recipe.max_ttl);
			
            target_layer->add(p);

            accumulator -= 1.0f;
        }
	}

    void FlameEmitter::update(ParticleManager* pm, float delta)
    {
        angle += randf(-max_change, max_change);
        accumulator += rate * delta;

        if (accumulator < 1.0f || target_layer == nullptr)
            return;

		const auto offset_count = offsets.size();
        while (accumulator >= 1.0f)
        {
			auto p = pm->create_particle();
			p->ry = pos.y + mirror_offset;
			
			auto offset = Vector2{ 0.0f, -range }.rotate(angle);
			offset.y = 0.f;
			if (offset_count == 0)
				p->pos = pos + offset;
			else
				p->pos = pos + offsets[rand() % offset_count] + offset;

            p->dp.x = 2.0f * offset.x;
            p->dp.y = -randf(recipe.min_dp.y, recipe.max_dp.y);

            auto n_size = randf(0.0f, 1.0f);
            p->size = recipe.min_size + n_size * (recipe.max_size - recipe.min_size);

            // determine initial color of particle based on distance from center 
			// TODO: revise this - idea is that for offsets that are further from pos.x, go into red
            auto dist = std::abs(p->pos.x) / 8.0f;
			p->color = recipe.colors[0] - recipe.colors[1] * dist;
			p->dc = recipe.dc;
			p->dc.a -= n_size;

            // The smaller the particle, the longer it will live
            p->ttl = recipe.min_ttl + (1.f - n_size) * (recipe.max_ttl - recipe.min_ttl);

            target_layer->add(p);

            accumulator -= 1.0f;
        }
    }

    void SmokeEmitter::update(ParticleManager* pm, float delta)
    {
        angle += randf(-max_change, max_change);
        accumulator += rate * delta;

        if (accumulator < 1.0f || target_layer == nullptr)
            return;

		const auto offset_count = offsets.size();
		while (accumulator >= 1.0f)
        {
			auto p = pm->create_particle();
			p->ry = pos.y + mirror_offset;
			
			auto offset = Vector2{ 0.0f, -range }.rotate(angle);
			offset.y = 0.f;
			if (offset_count == 0)
				p->pos = pos + offset;
			else
				p->pos = pos + offsets[rand() % offset_count] + offset;
			
            p->dp.x = offset.x;
            p->dp.y = -randf(recipe.min_dp.y, recipe.max_dp.y);

			const auto size = randf(0.0f, 1.0f);
            p->size = recipe.min_size + size * (recipe.max_size - recipe.min_size);
            p->color = recipe.colors[0];
			p->dc = recipe.dc * (0.25f + size); 
			p->ry = pos.y + mirror_offset;

            // The smaller the particle, the longer it will live
            p->ttl = recipe.min_ttl + (1.f - size) * (recipe.max_ttl - recipe.min_ttl);

            target_layer->add(p);

            accumulator -= 1.0f;
        }
    }

    void FountainEmitter::update(ParticleManager* pm, float delta)
    {
        angle += randf(-max_change, max_change);
        accumulator += rate * delta;
        time += delta;

        if (accumulator < 1.0f || target_layer == nullptr)
            return;

		const auto offset_count = offsets.size();
		const auto max_v = recipe.max_dp.y * (1.0f + 0.25f * fast_cos(time));
        while (accumulator >= 1.0f)
        {
			auto p = pm->create_particle();
			p->ry = pos.y + mirror_offset;
			p->flags |= Particle::Gravitational;

			auto offset = Vector2{ 0.0f, -range }.rotate(angle);
			offset.y = 0.f;
			if (offset_count == 0)
				p->pos = pos + offset;
			else
				p->pos = pos + offsets[rand() % offset_count] + offset;

			p->dp.x = offset.x;
            p->dp.y = -randf(recipe.min_dp.y, recipe.max_dp.y);
            
            auto size = randi(1, recipe.colors.size());
            p->size = randf(recipe.min_size, recipe.max_size);
            p->color = recipe.colors[size - 1];
			p->dc = recipe.dc; 
            p->ttl = randf(recipe.min_ttl, recipe.max_ttl);

            target_layer->add(p);

            accumulator -= 1.0f;
        }
    }

    void ExplosionEmitter::update(ParticleManager* pm, float delta)
    {
        time -= delta;
		if (time > 0.0f)
            return;

        if (repeat_interval >= 0.0f)
            time = repeat_interval;

		static const Vector2 base_vector{ 0.0f, -1.0f };

		const auto offset_count = offsets.size();
		for (int i = 0; i < static_cast<int>(rate); i++)
		{
			auto p = pm->create_particle();
			p->ry = pos.y + mirror_offset;
			p->flags |= Particle::Dampened;

			const auto angle = randf(0.0f, two_pi);
			const auto offset = base_vector.rotate(angle);
			if (offset_count == 0)
				p->pos = pos;
			else
				p->pos = pos + offsets[rand() % offset_count];
			p->dp = offset * randf(recipe.min_dp.y, recipe.max_dp.y);

			const auto n_size = randf(0.0f, 1.0f);
			p->size = recipe.min_size + n_size * (recipe.max_size - recipe.min_size);

			p->color = recipe.colors[0];
			p->dc = recipe.dc;
			p->dc.a -= n_size;

			// The smaller the particle, the longer it will live
			p->ttl = recipe.min_ttl + (1.f - n_size) * (recipe.max_ttl - recipe.min_ttl);

			target_layer->add(p);
		}
    }
}