#include <std_include.hpp>

#include "updater.hpp"
#include "file_updater.hpp"

#include <utils/cryptography.hpp>
#include <utils/flags.hpp>
#include <utils/http.hpp>
#include <utils/io.hpp>
#include <utils/string.hpp>
#include <utils/concurrency.hpp>
#include <utils/nt.hpp>

#define UPDATE_SERVER "https://cdn.brad.stream/"

#define UPDATE_FILE_MAIN UPDATE_SERVER "h2m.json"
#define UPDATE_FILE_AW UPDATE_SERVER "h2m-aw-maps.json"
#define UPDATE_FILE_CP UPDATE_SERVER "h2m-cp-maps.json"
#define UPDATE_FOLDER_MAIN UPDATE_SERVER "h2m/"
#define UPDATE_HOST_BINARY "h2m-updater.exe"

namespace updater
{
	namespace
	{
		std::string get_update_file()
		{
			return UPDATE_FILE_MAIN;
		}

		std::string get_aw_update_file()
		{
			return UPDATE_FILE_AW;
		}

		std::string get_cp_update_file()
		{
			return UPDATE_FILE_CP;
		}

		std::string get_update_folder()
		{
			return UPDATE_FOLDER_MAIN;
		}

		std::string get_filename(const std::filesystem::path path)
		{
			return path.filename().string();
		}

		bool is_map_file(const std::string& filename)
		{
			return (utils::string::ends_with(filename, ".pak") || utils::string::ends_with(filename, ".ff"));
		}

		std::vector<file_info> parse_file_infos(const std::string& json)
		{
			rapidjson::Document doc{};
			doc.Parse(json.data(), json.size());

			if (!doc.IsArray())
			{
				return {};
			}

			std::vector<file_info> files{};

			for (const auto& element : doc.GetArray())
			{
				if (!element.IsArray())
				{
					continue;
				}

				auto array = element.GetArray();

				file_info info{};
				info.name.assign(array[0].GetString(), array[0].GetStringLength());
				info.size = array[1].GetInt64();
				info.hash.assign(array[2].GetString(), array[2].GetStringLength());

				files.emplace_back(std::move(info));
			}

			return files;
		}

		std::string get_cache_buster()
		{
			return "?" + std::to_string(
				std::chrono::duration_cast<std::chrono::nanoseconds>(
					std::chrono::system_clock::now().time_since_epoch()).count());
		}

		std::vector<file_info> get_file_infos(const config& properties)
		{
			std::vector<updater::file_info> file_info;
			const auto main_json = utils::http::get_data(get_update_file() + get_cache_buster());
			if (main_json && main_json.has_value())
			{
				const auto parsed_main_infos = parse_file_infos(main_json.value());
				file_info.insert(file_info.end(), parsed_main_infos.begin(), parsed_main_infos.end());
			}

			if (properties.aw_maps)
			{
				const auto aw_json = utils::http::get_data(get_aw_update_file() + get_cache_buster());
				if (aw_json && aw_json.has_value())
				{
					const auto parsed_aw_infos = parse_file_infos(aw_json.value());
					file_info.insert(file_info.end(), parsed_aw_infos.begin(), parsed_aw_infos.end());
				}
			}

			if (properties.cp_maps)
			{
				const auto cp_json = utils::http::get_data(get_cp_update_file() + get_cache_buster());
				if (cp_json && cp_json.has_value())
				{
					const auto parsed_cp_infos = parse_file_infos(cp_json.value());
					file_info.insert(file_info.end(), parsed_cp_infos.begin(), parsed_cp_infos.end());
				}
			}
	
			return file_info;
		}

		std::string get_hash(const std::string& data)
		{
			return utils::cryptography::sha1::compute(data, true);
		}

		const file_info* find_host_file_info(const std::vector<file_info>& outdated_files)
		{
			for (const auto& file : outdated_files)
			{
				if (file.name == UPDATE_HOST_BINARY)
				{
					return &file;
				}
			}

			return nullptr;
		}

		size_t get_optimal_concurrent_download_count(const size_t file_count)
		{
			size_t cores = std::thread::hardware_concurrency();
			cores = (cores * 2) / 3;
			return std::max(1ull, std::min(cores, file_count));
		}

		bool is_inside_folder(const std::filesystem::path& file, const std::filesystem::path& folder)
		{
			const auto relative = std::filesystem::relative(file, folder);
			const auto start = relative.begin();
			return start != relative.end() && start->string() != "..";
		}
	}

	file_updater::file_updater(std::filesystem::path base,std::filesystem::path process_file, config properties)
		: base_(std::move(base))
		  , process_file_(std::move(process_file))
		  , dead_process_file_(process_file_)
		  , properties_(std::move(properties))
	{
		this->dead_process_file_.replace_extension(".exe.old");
		this->delete_old_process_file();
	}

	void file_updater::run() const
	{
		const auto files = get_file_infos(this->properties_);

		const auto outdated_files = this->get_outdated_files(files);
		if (outdated_files.empty())
		{
			return;
		}

		const auto update_size = this->get_update_size(outdated_files);
		const auto drive_space = this->get_available_drive_space();
		if (drive_space < update_size)
		{
			double gigabytes = static_cast<double>(update_size) / (1024 * 1024 * 1024);
			throw std::runtime_error(utils::string::va("Not enough space for update! %.2f GB required.", gigabytes));
		}

		this->update_host_binary(outdated_files);
		this->update_files(outdated_files);

		std::this_thread::sleep_for(1s);
	}

	void file_updater::update_file(const file_info& file) const
	{
		const auto url = get_update_folder() + file.name;
		const auto out_file = this->get_drive_filename(file);
		
		std::string data;
		if (!utils::io::write_file(out_file, data, false))
		{
			throw std::runtime_error("Failed to write file: " + out_file.string());
		}

		std::ofstream ofs(out_file, std::ios::binary);
		if (!ofs)
		{
			throw std::runtime_error("Failed to open file: " + out_file.string());
		}

		int currentPercent = 0;
		const auto success = utils::http::get_data_stream(url, {}, [&](size_t progress, size_t total_size)
		{
			auto progressRatio = (total_size > 0 && progress >= 0) ? static_cast<double>(progress) / total_size : 0.0;
			auto progressPercent = int(progressRatio * 100.0);
			if (progressPercent == currentPercent)
				return;

			currentPercent = progressPercent;
			printf("Downloading: %s (%d%%)\n", get_filename(file.name).data(), progressPercent);
		}, 
		[&](const char* chunk, size_t size)
		{
			if (chunk && size > 0)
			{
				ofs.write(chunk, size);
			}
			
		});

		ofs.close();

		if (!success)
		{
			throw std::runtime_error("Failed to download file: " + out_file.string());
		}

		// Verify file size
		std::ifstream ifs(out_file, std::ios::binary | std::ios::ate);
		if (!ifs)
		{
			throw std::runtime_error("Failed to open file for verification: " + out_file.string());
		}

		if (ifs.tellg() != file.size)
		{
			throw std::runtime_error("Downloaded file size mismatch: " + out_file.string());
		}
	}

	std::vector<file_info> file_updater::get_outdated_files(const std::vector<file_info>& files) const
	{
		std::vector<file_info> outdated_files{};

		for (const auto& info : files)
		{
			if (this->is_outdated_file(info))
			{
				outdated_files.emplace_back(info);
			}
		}

		return outdated_files;
	}

	std::size_t file_updater::get_update_size(const std::vector<file_info>& outdated_files) const
	{
		std::size_t total_size = 0;
		for (const auto& file : outdated_files)
		{
			total_size += file.size;
		}

		return total_size;
	}

	std::size_t file_updater::get_available_drive_space() const
	{
		std::filesystem::space_info spaceInfo = std::filesystem::space(this->base_);
		return spaceInfo.available;
	}

	void file_updater::update_host_binary(const std::vector<file_info>& outdated_files) const
	{
		const auto* host_file = find_host_file_info(outdated_files);
		if (!host_file)
		{
			return;
		}

		try
		{
			this->move_current_process_file();
			this->update_files({*host_file});
		}
		catch (...)
		{
			this->restore_current_process_file();
			throw;
		}

		if (!utils::flags::has_flag("norelaunch"))
		{
			utils::nt::relaunch_self();
		}

		throw update_cancelled();
	}

	void file_updater::update_files(const std::vector<file_info>& outdated_files) const
	{
		const auto thread_count = get_optimal_concurrent_download_count(outdated_files.size());

		std::vector<std::thread> threads{};
		std::atomic<size_t> current_index{0};

		utils::concurrency::container<std::exception_ptr> exception{};

		printf("Downloading/updating files\n");

		for (size_t i = 0; i < thread_count; ++i)
		{
			threads.emplace_back([&]()
			{
				while (!exception.access<bool>([](const std::exception_ptr& ptr)
				{
					return static_cast<bool>(ptr);
				}))
				{
					const auto index = current_index++;
					if (index >= outdated_files.size())
					{
						break;
					}

					try
					{
						const auto& file = outdated_files[index];
						//printf("Downloading file: %s\n", get_filename(file.name).data());
						this->update_file(file);
						//printf("Done downloading file: %s\n", get_filename(file.name).data());
					}
					catch (...)
					{
						exception.access([](std::exception_ptr& ptr)
						{
							ptr = std::current_exception();
						});

						return;
					}
				}
			});
		}

		for (auto& thread : threads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}

		exception.access([](const std::exception_ptr& ptr)
		{
			if (ptr)
			{
				std::rethrow_exception(ptr);
			}
		});

		printf("Done downloading/updating files\n");
	}

	bool file_updater::is_outdated_file(const file_info& file) const
	{
#if !defined(NDEBUG)
		if (file.name == UPDATE_HOST_BINARY && !utils::flags::has_flag("update"))
		{
			return false;
		}
#endif

		const auto drive_name = this->get_drive_filename(file);
		if (!utils::io::file_exists(drive_name))
		{
			return true;
		}

		if (utils::io::file_size(drive_name) != file.size)
		{
			return true;
		}

		if (!is_map_file(file.name)) //ignore geting hash for map files since it takes too long
		{
			std::string data{};
			if (!utils::io::read_file(drive_name, &data))
			{
				return true;
			}

			const auto hash = get_hash(data);
			return hash != file.hash;
		}

		return false;
	}

	std::filesystem::path file_updater::get_drive_filename(const file_info& file) const
	{
		if (file.name == UPDATE_HOST_BINARY)
		{
			return this->process_file_;
		}

		return this->base_ / file.name;
	}

	void file_updater::move_current_process_file() const
	{
		utils::io::move_file(this->process_file_, this->dead_process_file_);
	}

	void file_updater::restore_current_process_file() const
	{
		utils::io::move_file(this->dead_process_file_, this->process_file_);
	}

	void file_updater::delete_old_process_file() const
	{
		// Wait for other process to die
		for (auto i = 0; i < 4; ++i)
		{
			utils::io::remove_file(this->dead_process_file_);
			if (!utils::io::file_exists(this->dead_process_file_))
			{
				break;
			}

			std::this_thread::sleep_for(2s);
		}
	}
}
