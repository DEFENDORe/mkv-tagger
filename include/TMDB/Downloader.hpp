
#include <jsoncpp/json/json.h>

namespace TMDB
{
    class Downloader
    {
        private:
            static const long TIMEOUT = 20L;
            static size_t callback(const char* in, size_t size, size_t num, std::string* out);
            static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
        public:
            static Json::Value GetJSON(const std::string url);
            static void DownloadFile(std::string url, std::string savePath);
            static std::vector<uint8_t> DownloadFile(std::string url);
    };
}