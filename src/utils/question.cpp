#include <std_include.hpp>

#include "question.hpp"
#include "string.hpp"

namespace utils::question
{
	bool ask_y_n_question(const std::string& question)
	{
		std::string answer;
		printf("%s (y/n)\n", question.data());
		std::getline(std::cin, answer);

		answer = utils::string::to_lower(answer);

		if (answer == "yes")
			answer = "y";

		if (answer == "no")
			answer = "n";

		return (answer == "y" ? true : false);
	}
}