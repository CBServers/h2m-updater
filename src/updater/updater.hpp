#pragma once

#include "update_cancelled.hpp"

namespace updater
{
	struct config
	{
		bool aw_maps;
		bool cp_maps;
	};

	void run();
}
