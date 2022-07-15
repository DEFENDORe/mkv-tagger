#include <TMDB/API.hpp>

#include <iostream>

using namespace TMDB;

int main()
{
    const std::string KEY = "a6311cf3670eced44b40b55a703ef8f7";

    API tmdbApi(KEY);

    Json::Value value = tmdbApi.MovieSearch("Die Hard");
    
    for (auto &result : value["results"])
    {
        std::cout << result["title"].asString() << std::endl;
    }

    return 0;
}