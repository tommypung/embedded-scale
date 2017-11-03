#include <syslog.h>
#include <curl/curl.h>
#include "api.h"

static CURL *curl;

int api_register_weight(const char *house, const char *weight)
{
	if (!curl) {
		fprintf(stderr, "No curl defined");
		return -1;
	}

	char buffer[1024];
	sprintf(buffer, "%s/weight/%s/register/%s", API_URL, house, weight);
	curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, buffer);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

    syslog(LOG_INFO, "Calling url: %s\n", buffer);
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      return -1;
    }

	return 0;
}

int api_init()
{
	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "Could not initialize curl\n");
		return -1;
	}

	return 0;
}

int api_cleanup()
{
	if (curl)
		curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
}
