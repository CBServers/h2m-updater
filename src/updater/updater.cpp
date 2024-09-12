#include <std_include.hpp>

#include "updater.hpp"
#include "file_updater.hpp"
#include <utils/nt.hpp>
#include <utils/properties.hpp>
#include <utils/string.hpp>
#include <utils/question.hpp>

namespace updater
{
	void run()
	{
		const utils::nt::library host{};
		const auto root_folder = host.get_folder();

		const auto self = utils::nt::library::get_by_address(run);
		const auto self_file = self.get_path();

		auto cp_maps = utils::properties::load("cp-maps");
		if (!cp_maps)
		{
			const bool answer = utils::question::ask_y_n_question("Do you want to install Campaign Maps? (~30 GB)");

			utils::properties::store("cp-maps", answer);
			cp_maps = { answer };
		}

		auto aw_maps = utils::properties::load("aw-maps");
		if (!aw_maps)
		{
			const bool answer = utils::question::ask_y_n_question("Do you want to install AW Maps? (~40 GB)");

			utils::properties::store("aw-maps", answer);
			aw_maps = { answer };
		}

		config properties = {};
		properties.aw_maps = aw_maps.value();
		properties.cp_maps = cp_maps.value();

		const file_updater file_updater{root_folder, self_file, properties};

		file_updater.run();
	}
}
