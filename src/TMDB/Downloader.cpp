
#include <TMDB/Downloader.hpp>

#include <curl/curl.h>
#include <stdint.h>
#include <string>
#include <memory>

namespace TMDB
{

    size_t Downloader::write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
    {
        size_t written = fwrite(ptr, size, nmemb, stream);
        return written;
    }

    size_t Downloader::callback(const char* in, size_t size, size_t num, std::string* out)
    {
        const size_t totalBytes(size * num);
        out->append(in, totalBytes);
        return totalBytes;
    }

    void Downloader::DownloadFile(std::string url, std::string savePath)
    {
        CURL *curl;
        FILE *fp;

        curl = curl_easy_init();
        fp = fopen(savePath.c_str(),"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        long httpCode(0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);
        fclose(fp);
        if (httpCode != 200)
            throw std::runtime_error("CURL: HTTP request failed: " + url);

    }

    std::vector<uint8_t> Downloader::DownloadFile(std::string url)
    {
        CURL* curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        long httpCode(0);
        std::unique_ptr<std::string> httpData(new std::string());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);
        if (httpCode != 200)
            throw std::runtime_error("CURL: HTTP request failed: " + url);
        std::vector<uint8_t> result;
        std::move(httpData->begin(), httpData->end(), std::back_inserter(result));
        return result;
    }

    Json::Value Downloader::GetJSON(const std::string url)
    {
        CURL* curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        long httpCode(0);
        std::unique_ptr<std::string> httpData(new std::string());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);
        if (httpCode != 200)
            throw std::runtime_error("CURL: HTTP request failed: " + url);
        Json::Value jsonData;
        Json::Reader jsonReader;
        if (jsonReader.parse(*httpData, jsonData))
            return jsonData;
        else
            return Json::Value::null;
    }
}