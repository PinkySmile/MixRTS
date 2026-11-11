//
// Created by PinkySmile on 09/11/2026.
//

#include "BaseUI.hpp"
#include "Debug/Profiler.hpp"


void reloadViews(Screen &win, tgui::Gui &gui, Vector2u size, Vector2f offset)
{
	PROFILE_SCOPE("reload views");
	sf::View view{sf::FloatRect{
		offset,
		{
			static_cast<float>(size.x),
			static_cast<float>(size.y)
		}
	}};
	tgui::FloatRect viewGui{
		{0, 0},
		{
			size.x / guiScale,
			size.y / guiScale
		}
	};

	win.setView(view);
	gui.setAbsoluteView(viewGui);
}

void dispFPS(float frameTime)
{
	char details[96];
	auto old = game->screen->getView();
	auto stats = Profiler::get().getFrameStats(60);
	Vector2u size = game->screen->getSize();
	sf::View view{sf::FloatRect{
		{0, 0},
		size.to<float>()
	}};

	snprintf(details, sizeof(details), "%.2FFPS (a %.1f / w%.1f ms)", std::round(1 / frameTime), stats.averageMs, stats.worstMs);
	game->screen->setView(view);
	game->screen->displayElement(sf::String(details), {0, size.y - 35.f}, size.x, Screen::ALIGN_RIGHT);
	game->screen->setView(old);
}