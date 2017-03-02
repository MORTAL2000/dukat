#pragma once

namespace dukat
{
    class Renderer3;

	enum RenderStage
	{
		SCENE,
		OVERLAY
	};

    // Abstract base class for objects that can be rendered.
    class Renderable
    {
    public:
		RenderStage stage;
		bool visible;

        Renderable(void) : stage(RenderStage::SCENE), visible(true) { };
        virtual ~Renderable(void) { };

        virtual void render(Renderer3* renderer) = 0;
    };
}