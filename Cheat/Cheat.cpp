#include "Cheat.h"
#include "../ImGui/imgui.h"
#include <mutex>
#include "../Render/Render.h"
#include "../Cache/Cache.h"

void Cheat::Run()
{
	{
		std::lock_guard<std::mutex> lock(Cache::Mutex);
		Render::FrontBuffer = Render::BackBuffer;
	}

	const auto& RenderData = Render::FrontBuffer;

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	for (size_t i = 0; i < RenderData.Count; i++)
	{
		if (!RenderData.VisibleOnScreen[i])
			continue;

		drawList->AddRect({ RenderData.Boxes[i].x, RenderData.Boxes[i].y },
			{ RenderData.Boxes[i].z, RenderData.Boxes[i].w },
			ImColor(0, 0, 0, 255), 0, ImDrawFlags_None, 1.f);

		drawList->AddRect({ RenderData.Boxes[i].x - 1.f, RenderData.Boxes[i].y - 1.f },
			{ RenderData.Boxes[i].z + 1.f, RenderData.Boxes[i].w + 1.f },
			ImColor(255, 255, 255, 255), 0, ImDrawFlags_None, 1.f);

		drawList->AddRect({ RenderData.Boxes[i].x - 2.f, RenderData.Boxes[i].y - 2.f },
			{ RenderData.Boxes[i].z + 2.f, RenderData.Boxes[i].w + 2.f },
			ImColor(0, 0, 0, 255), 0, ImDrawFlags_None, 1.f);
	}
}