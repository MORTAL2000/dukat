#include "stdafx.h"
#include <dukat/mathutil.h>
#include <dukat/particleemitter.h>
#include <dukat/particle.h>
#include <dukat/particlemanager.h>
#include <dukat/rand.h>
#include <dukat/renderlayer2.h>

#include <dukat/log.h>

namespace dukat
{
	const ParticleEmitter::Recipe ParticleEmitter::Recipe::FlameRecipe{
		ParticleEmitter::Recipe::Type::Flame, 
		Particle::Alive | Particle::Linear,
		400.f, 1.f, 6.f, 1.f, 5.f,
		Vector2{ 2, 25 },	// min_dp.x used to determine initial range
		Vector2{ 2, 40 }, 	// max_dp.x used to scale particle motion 
		{ 
			Color{ 1.0f, 1.0f, 0.0f, 1.0f },	// Center color
			Color{ 0.0f, 0.25f, 0.0f, 0.0f },	// Color reduction as we move away from center
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f }		// Not used
		},
		Color{ 0.0f, -0.5f, 0.0f, -0.05f }		// Color reduction over time
	};

	const ParticleEmitter::Recipe ParticleEmitter::Recipe::SmokeRecipe{
		ParticleEmitter::Recipe::Type::Smoke, 
		Particle::Alive | Particle::Linear,
		100.f, 4.f, 8.f, 2.f, 6.f,
		Vector2{ 4, 15 },	// min_dp.x used to determine initial range
		Vector2{ 1, 25 },	// max_dp.x used to scale particle motion 
		{
			Color{ 1.0f, 1.0f, 1.0f, 1.0f },	// Smoke color
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f }		// Not used
		},
		Color{ 0.0f, 0.0f, 0.0f, -0.5f }		// Color reduction over time
	};

	const ParticleEmitter::Recipe ParticleEmitter::Recipe::FountainRecipe{
		ParticleEmitter::Recipe::Type::Fountain, 
		Particle::Alive | Particle::Linear | Particle::Gravitational,
		200.f, 1.f, 4.f, 4.f, 6.f,
		Vector2{ 0, 40 }, Vector2{ 4, 50 }, 	// horizontal x used for initial range
		{
			Color{ 0.47f, 0.945f, 1.0f, 0.8f }, // Each color picked randomly
			Color{ 0.0f, 0.8f, 0.976f, 0.866f },
			Color{ 0.0f, 0.596f, 0.862f, 0.933f },
			Color{ 0.0f, 0.411f, 0.666f, 1.0f }
		},
		Color{ 0.0f, 0.0f, 0.0f, -0.05f }		// Color reduction over time
	};

	const ParticleEmitter::Recipe ParticleEmitter::Recipe::ExplosionRecipe{
		ParticleEmitter::Recipe::Type::Explosion, 
		Particle::Alive | Particle::Linear | Particle::Dampened,
		100.f, 1.f, 6.f, 1.f, 5.f,
		Vector2{ 0, 25 }, Vector2{ 0, 35 },		// x used to adjust initial position in particle direction 
												// y used to determine particle velocity
		{
			Color{ 1.0f, 1.0f, 0.0f, 1.0f },	// Initial color
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f }		// Not used
		},
		Color{ 0.0f, -1.0f, 0.0f, -0.1f }		// Color reduction over time
	};

	const ParticleEmitter::Recipe ParticleEmitter::Recipe::SpiralRecipe{
		ParticleEmitter::Recipe::Type::Spiral,
		Particle::Alive | Particle::Linear,
		100.f, 1.f, 4.f, 1.f, 5.f,
		Vector2{ 4.f, 3.f },					// x used to scale emit range, y used to define max angular change per second
		Vector2{ 64, 64 },						// Used to scale dp
		{
			Color{ 1.0f, 1.0f, 0.0f, 1.0f },	// Initial color
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f },	// Not used
			Color{ 0.0f, 0.0f, 0.0f, 0.0f }		// Not used
		},
		Color{ 0.0f, -1.0f, 0.0f, -0.1f }		// Color reduction over time
	};

	const ParticleEmitter::Recipe ParticleEmitter::Recipe::SnowRecipe{
		ParticleEmitter::Recipe::Layered,
		Particle::Alive | Particle::Linear,
		40.0f,	
		1.0f, 3.0f,		// size within 1,3
		10.0f, 10.0f,	// ttl 
		Vector2{ -2, 24 },  // particle motion for background (color[2] and color[3])
		Vector2{ 4, 36 },	// particle motion for foreground (color[0] and color[1])
		{
			color_rgba(0xffffffff),
			color_rgba(0xffffffc0),
			color_rgba(0xc7cfddc0),
			color_rgba(0x94fdffc0),
		},
		Color{ 0.f, 0.f, 0.f, -0.005f }
	};

	// LINEAR
	// - particles are created with unique direction in +/- dp range
	void linear_update(ParticleManager& pm, ParticleEmitter& em, float delta)
	{
        em.accumulator += em.recipe.rate * delta;
        if (em.accumulator < 1.0f || em.target_layer == nullptr)
            return;

		const auto offset_count = em.offsets.size();
        while (em.accumulator >= 1.0f)
        {
			auto p = pm.create_particle();
			if (p == nullptr)
				return;
			p->flags = em.recipe.flags;
			p->ry = em.pos.y + em.mirror_offset;
			
			if (offset_count == 0)
				p->pos = em.pos;
			else
				p->pos = em.pos + em.offsets[random(0, offset_count)];

			p->dp = random(em.recipe.min_dp, em.recipe.max_dp);
            p->size = random(em.recipe.min_size, em.recipe.max_size);
			p->color = em.recipe.colors[random(0, em.recipe.colors.size())];
			p->dc = em.recipe.dc;
            p->ttl = random(em.recipe.min_ttl, em.recipe.max_ttl);
			
			em.target_layer->add(p);

			em.accumulator -= 1.0f;
        }
	}

	// UNIFORM
	// - particles are generated within a box based on offset[0] - offset[1]
	// - particles are created with fixed direction within +/- dp range
	void uniform_update(ParticleManager& pm, ParticleEmitter& em, float delta)
	{
		em.accumulator += em.recipe.rate * delta;
		if (em.accumulator < 1.0f || em.target_layer == nullptr)
			return;

		if (em.offsets.size() < 2)
			return; 

		const auto& min_offset = em.offsets[0];
		const auto& max_offset = em.offsets[1];
		while (em.accumulator >= 1.0f)
		{
			auto p = pm.create_particle();
			if (p == nullptr)
				return;
			p->flags = em.recipe.flags;
			p->ry = em.pos.y + em.mirror_offset;
			p->pos = em.pos + random(min_offset, max_offset);

			p->dp = random(em.recipe.min_dp, em.recipe.max_dp);
			p->size = random(em.recipe.min_size, em.recipe.max_size);
			p->color = em.recipe.colors[random(0, em.recipe.colors.size())];
			p->dc = em.recipe.dc;
			p->ttl = random(em.recipe.min_ttl, em.recipe.max_ttl);

			em.target_layer->add(p);

			em.accumulator -= 1.0f;
		}
	}

	// LAYERED
	// - particles are generated within a box based on offset[0] - offset[1]
	// - background particles are created with min_dp and colors 2 and 3
	// - foreground particles are created with max_dp and colors 0 and 1
	void layered_update(ParticleManager& pm, ParticleEmitter& em, float delta)
	{
		em.accumulator += em.recipe.rate * delta;
		if (em.accumulator < 1.0f || em.target_layer == nullptr)
			return;

		if (em.offsets.size() < 2)
			return;

		const auto& min_offset = em.offsets[0];
		const auto& max_offset = em.offsets[1];
		while (em.accumulator >= 1.0f)
		{
			auto p = pm.create_particle();
			if (p == nullptr)
				return;
			p->flags = em.recipe.flags;
			p->ry = em.pos.y + em.mirror_offset;
			p->pos = em.pos + random(min_offset, max_offset);

			const auto z = random(0, em.recipe.colors.size());
			p->dp = (z >= 2) ? em.recipe.min_dp : em.recipe.max_dp;

			p->size = static_cast<float>(random(static_cast<int>(em.recipe.min_size), static_cast<int>(em.recipe.max_size)));
			
			p->color = em.recipe.colors[z];
			p->dc = em.recipe.dc;
			p->ttl = random(em.recipe.min_ttl, em.recipe.max_ttl);

			em.target_layer->add(p);

			em.accumulator -= 1.0f;
		}
	}

	// RADIAL
	// - continuous particles generated at a radius around emitter
	// - particle direction determined by random angle
	// - particle position based min_dp.x along direction vector
	// - particle velocity within min_dp.y and max_dp.y along direction
	void radial_update(ParticleManager& pm, ParticleEmitter& em, float delta)
	{
		em.accumulator += em.recipe.rate * delta;
		if (em.accumulator < 1.0f || em.target_layer == nullptr)
			return;

		static const Vector2 base_vector{ 0.0f, -1.0f };
		const auto offset_count = em.offsets.size();
		while (em.accumulator >= 1.0f)
		{
			auto p = pm.create_particle();
			if (p == nullptr)
				return;
			p->flags = em.recipe.flags;
			p->ry = em.pos.y + em.mirror_offset;

			const auto angle = random(0.0f, two_pi);
			const auto offset = base_vector.rotate(angle);
			if (offset_count == 0)
				p->pos = em.pos;
			else
				p->pos = em.pos + em.offsets[random(0, offset_count)];

			if (em.recipe.min_dp.x != 0.0f)
				p->pos += offset * em.recipe.min_dp.x;

			p->dp = offset * random(em.recipe.min_dp.y, em.recipe.max_dp.y);
			p->size = random(em.recipe.min_size, em.recipe.max_size);
			p->color = em.recipe.colors[random(0, em.recipe.colors.size())];
			p->dc = em.recipe.dc;
			p->ttl = random(em.recipe.min_ttl, em.recipe.max_ttl);

			em.target_layer->add(p);

			em.accumulator -= 1.0f;
		}
	}

	// FLAME
	// - particles are emitted with upward direction
	// - flame particles have variable ttl, so that we end up with holes
	// - if using multiple emitters, each emitter should have a current 
	//   direction that swings by random amount; that will cause subsequent 
	//   particles to have similar direction
	void flame_update(ParticleManager& pm, ParticleEmitter& em, float delta)
    {
		const auto max_change = 0.25f;
        em.value += random(-max_change, max_change);
		em.accumulator += em.recipe.rate * delta;

        if (em.accumulator < 1.0f || em.target_layer == nullptr)
            return;

		const auto range = em.recipe.min_dp.x;
		const auto offset_count = em.offsets.size();
        while (em.accumulator >= 1.0f)
        {
			auto p = pm.create_particle();
			if (p == nullptr)
				break;

			p->flags = em.recipe.flags;
			p->ry = em.pos.y + em.mirror_offset;
			
			auto offset = Vector2{ 0.f, range }.rotate(em.value);
			offset.y = 0.f;

			p->dp.x = em.recipe.max_dp.x * offset.x;
			p->dp.y = -random(em.recipe.min_dp.y, em.recipe.max_dp.y);

			if (offset_count > 0)
				offset += em.offsets[random(0, offset_count)];
			p->pos = em.pos + offset;

            auto n_size = random(0.0f, 1.0f);
            p->size = em.recipe.min_size + n_size * (em.recipe.max_size - em.recipe.min_size);

            // determine initial color of particle based on distance from center 
			// TODO: revise this - idea is that for offsets that are further from pos.x, go into red
            auto dist = std::abs(offset.x) / (4.0f * range);
			p->color = em.recipe.colors[0] - em.recipe.colors[1] * dist;
			p->dc = em.recipe.dc;
			p->dc.a -= n_size;

            // The smaller the particle, the longer it will live
            p->ttl = em.recipe.min_ttl + (1.f - n_size) * (em.recipe.max_ttl - em.recipe.min_ttl);

			em.target_layer->add(p);

			em.accumulator -= 1.0f;
        }
    }

	// SMOKE
	// - upward direction of particles
	// - position of emitter ocilates on the x axis
	// - single color that fades out over time
	void smoke_update(ParticleManager& pm, ParticleEmitter& em, float delta)
    {
		const auto max_change = 0.15f;
		em.value += random(-max_change, max_change);
		em.accumulator += em.recipe.rate * delta;

        if (em.accumulator < 1.0f || em.target_layer == nullptr)
            return;

		const auto offset_count = em.offsets.size();
		while (em.accumulator >= 1.0f)
        {
			auto p = pm.create_particle();
			if (p == nullptr)
				break;

			p->flags = em.recipe.flags;
			p->ry = em.pos.y + em.mirror_offset;
			
			auto offset = Vector2{ 0.f, em.recipe.min_dp.x }.rotate(em.value);
			offset.y = 0.f;
			if (offset_count == 0)
				p->pos = em.pos + offset;
			else
				p->pos = em.pos + em.offsets[rand() % offset_count] + offset;
			
            p->dp.x = offset.x * em.recipe.max_dp.x;
            p->dp.y = -random(em.recipe.min_dp.y, em.recipe.max_dp.y);

			const auto size = random(0.0f, 1.0f);
            p->size = em.recipe.min_size + size * (em.recipe.max_size - em.recipe.min_size);
            p->color = em.recipe.colors[0];
			p->dc = em.recipe.dc * (0.25f + size);
			p->ry = em.pos.y + em.mirror_offset;

            // The smaller the particle, the longer it will live
            p->ttl = em.recipe.min_ttl + (1.f - size) * (em.recipe.max_ttl - em.recipe.min_ttl);

			em.target_layer->add(p);

			em.accumulator -= 1.0f;
        }
    }

	// FOUNTAIN
	// - initial dx, dy
	// - gravity pulls at dy->reduce and ultimately turn around
	void fountain_update(ParticleManager& pm, ParticleEmitter& em, float delta)
    {
		const auto max_change = 0.2f;
		em.value += random(-max_change, max_change);
		em.accumulator += em.recipe.rate * delta;

        if (em.accumulator < 1.0f || em.target_layer == nullptr)
            return;

		const auto offset_count = em.offsets.size();
		const auto max_v = em.recipe.max_dp.y * (1.0f + 0.25f * fast_cos(em.age));
        while (em.accumulator >= 1.0f)
        {
			auto p = pm.create_particle();
			if (p == nullptr)
				break;

			p->flags = em.recipe.flags;
			p->ry = em.pos.y + em.mirror_offset;

			auto offset = Vector2{ 0.f, em.recipe.max_dp.x }.rotate(em.value);
			offset.y = 0.f;
			if (offset_count == 0)
				p->pos = em.pos + offset;
			else
				p->pos = em.pos + em.offsets[random(0, offset_count)] + offset;

			p->dp.x = offset.x;
            p->dp.y = -random(em.recipe.min_dp.y, em.recipe.max_dp.y);
            
            auto size = random(1, em.recipe.colors.size());
            p->size = random(em.recipe.min_size, em.recipe.max_size);
            p->color = em.recipe.colors[size - 1];
			p->dc = em.recipe.dc;
            p->ttl = random(em.recipe.min_ttl, em.recipe.max_ttl);

			em.target_layer->add(p);

			em.accumulator -= 1.0f;
        }
    }

	// EXPLOSION
	// - burst of particles
	// - particle direction determined by random angle
	// - particle position based min_dp.x along direction vector
	// - particle velocity within min_dp.y and max_dp.y along direction
	void explosion_update(ParticleManager& pm, ParticleEmitter& em, float delta)
    {
		if (em.target_layer == nullptr)
            return;

		// value used as repeat interval
		if (em.age > 0.0f)
		{
			if (em.value <= 0.0f)
				return; // no repeat
			else if (em.age < em.value)
				return;
			else
				em.age = 0.0f;
		}

		static const Vector2 base_vector{ 0.0f, -1.0f };
		const auto offset_count = em.offsets.size();
		for (int i = 0; i < static_cast<int>(em.recipe.rate); i++)
		{
			auto p = pm.create_particle();
			if (p == nullptr)
				break;

			p->flags = em.recipe.flags;
			p->ry = em.pos.y + em.mirror_offset;

			const auto angle = random(0.0f, two_pi);
			const auto offset = base_vector.rotate(angle);
			if (offset_count == 0)
				p->pos = em.pos;
			else
				p->pos = em.pos + em.offsets[random(0, offset_count)];

			if (em.recipe.min_dp.x != 0.0f)
				p->pos += offset * em.recipe.min_dp.x;

			p->dp = offset * random(em.recipe.min_dp.y, em.recipe.max_dp.y);

			const auto n_size = random(0.0f, 1.0f);
			p->size = em.recipe.min_size + n_size * (em.recipe.max_size - em.recipe.min_size);

			p->color = em.recipe.colors[0];
			p->dc = em.recipe.dc;
			p->dc.a -= n_size;

			// The smaller the particle, the longer it will live
			p->ttl = em.recipe.min_ttl + (1.f - n_size) * (em.recipe.max_ttl - em.recipe.min_ttl);

			em.target_layer->add(p);
		}
    }

	// SPIRAL
	// - spiralling emitters
	// - angle changes with time, particles are emitted with initial direction based on angle
	void spiral_update(ParticleManager& pm, ParticleEmitter& em, float delta)
	{
		em.value += em.recipe.min_dp.y * delta; 
		em.accumulator += em.recipe.rate * delta;

		if (em.accumulator < 1.0f || em.target_layer == nullptr)
			return;

		auto offset = Vector2{ 0.f, 1.f }.rotate(em.value);

		const auto offset_count = em.offsets.size();
		while (em.accumulator >= 1.0f)
		{
			auto p = pm.create_particle();
			if (p == nullptr)
				break;

			p->flags = em.recipe.flags;
			p->ry = em.pos.y + em.mirror_offset;
			p->dp.x = em.recipe.max_dp.x * offset.x;
			p->dp.y = em.recipe.max_dp.y * offset.y;

			p->pos = em.pos + offset * em.recipe.min_dp.x;
			if (offset_count > 0)
				p->pos += em.offsets[random(0, offset_count)];

			const auto n_size = random(0.0f, 1.0f);
			p->size = em.recipe.min_size + n_size * (em.recipe.max_size - em.recipe.min_size);

			p->color = em.recipe.colors[0];
			p->dc = em.recipe.dc;
			p->dc.a -= n_size;

			// The smaller the particle, the longer it will live
			p->ttl = em.recipe.min_ttl + (1.f - n_size) * (em.recipe.max_ttl - em.recipe.min_ttl);

			em.target_layer->add(p);

			em.accumulator -= 1.0f;
		}
	}

	void init_emitter(ParticleEmitter& emitter, const ParticleEmitter::Recipe& recipe)
	{
		emitter.recipe = recipe;
		switch (recipe.type)
		{
		case ParticleEmitter::Recipe::Linear:
			emitter.update = std::bind(linear_update, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			break;
		case ParticleEmitter::Recipe::Uniform:
			emitter.update = std::bind(uniform_update, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			break;
		case ParticleEmitter::Recipe::Flame:
			emitter.update = std::bind(flame_update, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			break;
		case ParticleEmitter::Recipe::Smoke:
			emitter.update = std::bind(smoke_update, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			break;
		case ParticleEmitter::Recipe::Fountain:
			emitter.update = std::bind(fountain_update, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			break;
		case ParticleEmitter::Recipe::Explosion:
			emitter.update = std::bind(explosion_update, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			break;
		case ParticleEmitter::Recipe::Spiral:
			emitter.update = std::bind(spiral_update, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			break;
		case ParticleEmitter::Recipe::Radial:
			emitter.update = std::bind(radial_update, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			break;
		case ParticleEmitter::Recipe::Layered:
			emitter.update = std::bind(layered_update, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
			break;
		default:
			emitter.update = nullptr;
			break;
		}		
	}
}