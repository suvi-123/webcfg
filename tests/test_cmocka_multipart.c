/**
  * Copyright 2019 Comcast Cable Communications Management, LLC
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
 */
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <CUnit/Basic.h>
#include <cmocka.h>

#include "../src/webcfg_log.h"
#include "../src/webcfg_param.h"
#include "../src/webcfg.h"
#include "../src/webcfg_multipart.h"
#include "../src/webcfg_helpers.h"
#include "../src/webcfg_db.h"
#include "../src/webcfg_notify.h"
#include "../src/webcfg_metadata.h"
#include "../src/webcfg_generic.h"
#include "../src/webcfg_event.h"
#include "../src/webcfg_auth.h"
#include "../src/webcfg_blob.h"
#include "../src/webcfg_timer.h"

#define UNUSED(x) (void )(x)
int numLoops;

typedef void CURL;

#undef curl_easy_setopt
#undef curl_easy_getinfo

long long getRetryExpiryTimeout()
{
    return 0;
}

int get_retry_timer()
{
	return 0;
}

int updateRetryTimeDiff(long long expiry_time)
{
	UNUSED(expiry_time);
	return 0;
}

int checkRetryTimer( long long timestamp)
{
	return timestamp;
}

char* printTime(long long time)
{
	UNUSED(time);
	return NULL;
}

int checkMaintenanceTimer()
{
	return 0;
}

void initMaintenanceTimer()
{	
	return;
}

int getMaintenanceSyncSeconds(int maintenance_count)
{
	UNUSED(maintenance_count);
	return 0;
}

long get_global_retry_timestamp()
{
    return 0;
}

int retrySyncSeconds()
{
	return 0;
}

void set_retry_timer(int value)
{
	UNUSED(value);	
	return;
}

void set_global_retry_timestamp(long value)
{
    	UNUSED(value);	
        return;
}

void set_global_maintenance_time(long value)
{
	UNUSED(value);    
	return;
}

int Get_Webconfig_URL(char *pString)
{
    // Set a non-empty value for configURL
    strcpy(pString, "http://example.com/config.xml");
    return 0; // or whatever the expected return value is
}

CURL *curl_easy_init  ()
{
	function_called();
	return (CURL *) mock();
}

CURLcode curl_easy_perform(CURL *curl)
{
	UNUSED(curl);
	int rtn;

	function_called();
	rtn = (int) mock();
	return rtn;
}

CURLcode curl_easy_setopt(CURL *easy, CURLoption option, ...)
{
    UNUSED (easy);
    UNUSED (option);
    return CURLE_OK;
}

CURLcode curl_easy_getinfo(CURL *easy, CURLINFO info, ... )
{
    UNUSED (easy);
    UNUSED (info);
    //int rtn;

    va_list args;
    va_start(args, info);

    CURLcode result = CURLE_OK;

    function_called();
	result = (int) mock();

    if (info == CURLINFO_RESPONSE_CODE) {
        long* response_code = va_arg(args, long*);
        *response_code = 200; // Always return 200 as the response code
    } else if (info == CURLINFO_CONTENT_TYPE) {
        char** ct = va_arg(args, char**);
        *ct = "multipart/mixed"; // Set the content type as "multipart/mixed"
    }
	return result;
}

void curl_easy_cleanup(CURL *easy)
{
    (void) easy;
}

void test_webcfg_http_request_curl_init_fail()
{
    char *config = NULL; // Initialize with your data
    int r_count = 1; // Set the number of retries as needed
    int status = 0; // Set the status as needed
    long code = 0; // Response code
    char *transaction_id = NULL; // Transaction ID
    char contentType[64] = {0}; // Content type
    size_t dataSize = 0; // Data size
    char docname[64] = "value"; // Document name

	will_return (curl_easy_init, NULL);
    expect_function_calls (curl_easy_init, 1);

	WEBCFG_STATUS result = webcfg_http_request(&config, r_count, status, &code, &transaction_id, contentType, &dataSize, docname);

	WebcfgInfo("The result is %d", result);
    assert_int_equal (result, 1);
    
}

//get_global_supplementarySync() == 0
void test_webcfg_http_request_curl_init_success()
{   
    char *config = NULL; // Initialize with your data
    int r_count = 1; // Set the number of retries as needed
    int status = 0; // Set the status as needed
    long code = 0; // Response code
    char *transaction_id = NULL; // Transaction ID
    char contentType[64] = {0}; // Content type
    size_t dataSize = 0; // Data size
    char docname[64] = {0}; // Document name

    set_global_supplementarySync(0);
	
    will_return (curl_easy_init, 1);
    expect_function_calls (curl_easy_init, 1);
    
    will_return (curl_easy_perform, 0);
    expect_function_calls (curl_easy_perform, 1);
    
    will_return (curl_easy_getinfo, 0);
    expect_function_calls (curl_easy_getinfo, 1);

    will_return (curl_easy_getinfo, 0);
    expect_function_calls (curl_easy_getinfo, 1);

    will_return (curl_easy_getinfo, 1);
    expect_function_calls (curl_easy_getinfo, 1);

	WEBCFG_STATUS result = webcfg_http_request(&config, r_count, status, &code, &transaction_id, contentType, &dataSize, docname);

	WebcfgInfo("The result is %d", result);

    assert_int_equal (result, 0);
}

int Get_Supplementary_URL(char *name, char *pString) {
    // Set a non-empty value for configURL
    strcpy(pString, "http://example.com/config.xml");
    return 0; // or whatever the expected return value is
}

//get_global_supplementarySync() == 1 
void test_webcfg_http_request_supp_sync()
{   
    char *config = NULL; // Initialize with your data
    int r_count = 2; // Set the number of retries as needed
    int status = 0; // Set the status as needed
    long code = 0; // Response code
    char *transaction_id = NULL; // Transaction ID
    char contentType[64] = {0}; // Content type
    size_t dataSize = 0; // Data size
    char docname[64] = "value"; // Document name


    set_global_supplementarySync(1);
	will_return (curl_easy_init, 1);
    expect_function_calls (curl_easy_init, 1);

    will_return (curl_easy_perform, 0);
    expect_function_calls (curl_easy_perform, 1);

    will_return (curl_easy_getinfo, 0);
    expect_function_calls (curl_easy_getinfo, 1);

    will_return (curl_easy_getinfo, 0);
    expect_function_calls (curl_easy_getinfo, 1);

    will_return (curl_easy_getinfo, 0);
    expect_function_calls (curl_easy_getinfo, 1);

	WEBCFG_STATUS result = webcfg_http_request(&config, r_count, status, &code, &transaction_id, contentType, &dataSize, docname);

	WebcfgInfo("The result is %d", result);

    assert_int_equal (result, 0);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
	    cmocka_unit_test(test_webcfg_http_request_curl_init_fail),
        cmocka_unit_test(test_webcfg_http_request_curl_init_success),
        cmocka_unit_test(test_webcfg_http_request_supp_sync)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

