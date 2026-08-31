#include "HTTP.h"
#include <curl/curl.h>
#include "../Popup/Popup.h"

static std::string ErrorMessage = "";

static size_t DownloadCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	FILE* stream = (FILE*)userp;
	if (!stream)
	{
		ErrorMessage = "No stream";
		return 0;
	}
	return fwrite((FILE*)contents, size, nmemb, stream);
}

bool HTTP::Download(const char* URL, const char* Filename)
{
	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, Filename, "wb");
	if (err != 0 || !fp)
	{
		ErrorMessage = "Failed to create file on the disk: " + std::string(Filename);
		return false;
	}

	CURL* curlCtx = curl_easy_init();
	curl_easy_setopt(curlCtx, CURLOPT_URL, URL);
	curl_easy_setopt(curlCtx, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curlCtx, CURLOPT_WRITEFUNCTION, DownloadCallback);
	curl_easy_setopt(curlCtx, CURLOPT_FOLLOWLOCATION, 1);

	CURLcode rc = curl_easy_perform(curlCtx);
	if (rc)
	{
		ErrorMessage = "Failed to download from URL: " + std::string(URL);
		return false;
	}

	long res_code = 0;
	curl_easy_getinfo(curlCtx, CURLINFO_RESPONSE_CODE, &res_code);
	if (!((res_code == 200 || res_code == 201) && rc != CURLE_ABORTED_BY_CALLBACK))
	{
		ErrorMessage = "Response code: " + res_code;
		return false;
	}

	curl_easy_cleanup(curlCtx);

	fclose(fp);

	return true;
}

std::string HTTP::GetError()
{
	return ErrorMessage;
}