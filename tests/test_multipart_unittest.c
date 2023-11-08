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
#include <msgpack.h>
#include <curl/curl.h>
#include <base64.h>
#include <uuid/uuid.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <CUnit/Basic.h>
#include <wdmp-c.h>

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
#include "../src/webcfg_rbus.h"

rbusHandle_t handle;
extern rbusHandle_t rbus_handle;

#ifdef FEATURE_SUPPORT_AKER
#include "webcfg_aker.h"
#endif

#define WEBCFG_BLOB_PARAM "Device.X_RDK_WebConfig.blobElement"

int numLoops;

#define MAX_HEADER_LEN	4096
#define UNUSED(x) (void )(x)

char device_mac[32] = {'\0'};

char* get_deviceMAC()
{
	char *device_mac = strdup("b42xxxxxxxxx");	
	return device_mac;
}
char * getRebootReason()
{
	char *reason = strdup("factory-reset");
	return reason;
}

char *get_global_systemReadyTime()
{
	char *sTime = strdup("158000123");
	return sTime;
}
char * getDeviceBootTime()
{
	char *bTime = strdup("152200345");
	return bTime;
}
char * getFirmwareVersion()
{
	char *fName = strdup("Firmware.bin");
	return fName;
}

char * getProductClass()
{
	char *pClass = strdup("Product");
	return pClass;
}

char * getModelName()
{
	char *mName = strdup("Model");
	return mName;
}

void test_generate_trans_uuid(){

	char *transaction_uuid = NULL;
	transaction_uuid = generate_trans_uuid();
	CU_ASSERT_FATAL( NULL != transaction_uuid);
}

void test_replaceMac(){
	
	char *configURL= "https://config/{sss}/com";
	char c[] = "{sss}";
	char *webConfigURL = replaceMacWord(configURL, c, get_deviceMAC());
	CU_ASSERT_STRING_EQUAL(webConfigURL,"https://config/b42xxxxxxxxx/com");
	
	
}

void test_createHeader(){
	
	CURL *curl;
	struct curl_slist *list = NULL;
	struct curl_slist *headers_list = NULL;
	char *transID = NULL;
	char *subdoclist = NULL;
	int status=0;
	curl = curl_easy_init();
	CU_ASSERT_PTR_NOT_NULL(curl);
	createCurlHeader(list, &headers_list, status, &transID, &subdoclist);
	CU_ASSERT_PTR_NOT_NULL(transID);
	CU_ASSERT_PTR_NOT_NULL(headers_list);		
}


void test_validateParam()
{	param_t *reqParam = NULL;
	int paramCount = 1;
 	reqParam = (param_t *) malloc(sizeof(param_t) * paramCount);
	reqParam[0].name = strdup("Device.X_RDK_WebConfig.RootConfig.Data");
	reqParam[0].value = strdup("true");
	reqParam[0].type = 2;
	int n=validate_request_param(reqParam, paramCount);
	CU_ASSERT_EQUAL(0,n);
	reqParam_destroy(paramCount, reqParam);
	
}

void test_checkRootUpdate(){
	webconfig_tmp_data_t *tmpData = (webconfig_tmp_data_t *)malloc(sizeof(webconfig_tmp_data_t));
	tmpData->name = strdup("wan");
	tmpData->version = 410448631;
	tmpData->status = strdup("success");
	tmpData->trans_id = 14464;
	tmpData->retry_count = 0;
	tmpData->error_code = 0;
	tmpData->error_details = strdup("none");
	tmpData->next = NULL;
	set_global_tmp_node(tmpData);
	int m=checkRootUpdate();
	CU_ASSERT_EQUAL(0,m);
	if(tmpData)
	{
		WEBCFG_FREE(tmpData->name);
		WEBCFG_FREE(tmpData->status);
		WEBCFG_FREE(tmpData->error_details);
		WEBCFG_FREE(tmpData);
	}

}


void test_checkDBList(){
	char *root_str = strdup("factory-reset");
	uint32_t root_version = 1234;
	webconfig_db_data_t * webcfgdb = NULL;
  	webcfgdb = (webconfig_db_data_t *) malloc (sizeof(webconfig_db_data_t));
	checkDBList("root", root_version, root_str);
	memset( webcfgdb, 0, sizeof( webconfig_db_data_t ) );
	CU_ASSERT_PTR_NOT_NULL(webcfgdb );
	WEBCFG_FREE(webcfgdb);
}
	
void test_updateDBlist(){
	char *rootstr = strdup("factory-reset");
	uint32_t version = 1234;
	int sts = updateDBlist("root", version, rootstr);
	CU_ASSERT_EQUAL(2,sts);
}

void test_appendedDoc(){
	size_t appenddocPackSize = -1;
	size_t embeddeddocPackSize = -1;
	void *appenddocdata = NULL;
	void *embeddeddocdata = NULL;
	appenddoc_t *appenddata = NULL;
        char * finaldocdata = NULL;

	//blob data
	webconfig_db_data_t *blob_data;
    	blob_data = (webconfig_db_data_t *) malloc (sizeof(webconfig_db_data_t));
	blob_data->name = strdup("wan");
	blob_data->version = 410448631;
	blob_data->root_string = strdup("portmapping");
	blob_data->next=NULL;
  	size_t blob_size=175;

	//append data
	appenddata = (appenddoc_t *) malloc(sizeof(appenddoc_t ));
	CU_ASSERT_PTR_NOT_NULL(appenddata);
	memset(appenddata, 0, sizeof(appenddoc_t));
        appenddata->subdoc_name = strdup("Device.Data");
        appenddata->version = 410448631;
	appenddata->transaction_id =22334;

	//pack append doc
	appenddocPackSize = webcfg_pack_appenddoc(appenddata, &appenddocdata);
	CU_ASSERT_FATAL( 0 != appenddocPackSize);
	WEBCFG_FREE(appenddata->subdoc_name);
    	WEBCFG_FREE(appenddata);
	
	//encode data
        embeddeddocPackSize = appendWebcfgEncodedData(&embeddeddocdata, (void *)blob_data, blob_size, appenddocdata, appenddocPackSize);
	CU_ASSERT_FATAL( 0 != embeddeddocPackSize);
	WEBCFG_FREE(appenddocdata);

	//base64 encode
        finaldocdata = base64blobencoder((char *)embeddeddocdata, embeddeddocPackSize);
	CU_ASSERT_FATAL( NULL != finaldocdata);
	WEBCFG_FREE(embeddeddocdata);
        
}

#ifdef FEATURE_SUPPORT_AKER

void test_UpdateErrorsendAkerblob(){
	WDMP_STATUS ret = WDMP_FAILURE;
	char *paramName= strdup("Device.DeviceInfo.X_RDKCENTRAL-COM_Aker.Update");
	char *blob= strdup("true");;
	uint32_t blobSize =2;
	uint16_t docTransId=12345; 
	int version=2.0;
	ret = send_aker_blob(paramName,blob,blobSize,docTransId,version);
	CU_ASSERT_EQUAL(1, ret);

}
void test_DeleteErrorsendAkerblob(){
	WDMP_STATUS ret = WDMP_FAILURE;
	char *paramName= strdup("Device.DeviceInfo.X_RDKCENTRAL-COM_Aker.Delete");
	char *blob= strdup("true");;
	uint32_t blobSize =2;
	uint16_t docTransId=12345; 
	int version=2.0;
	ret = send_aker_blob(paramName,blob,blobSize,docTransId,version);
	CU_ASSERT_EQUAL(1, ret);

}
void test_akerWait(){
	
	int backoffRetryTime = 0;
	int c=2;
	backoffRetryTime = (int) pow(2, c) -1;
	int wait=akerwait__ (backoffRetryTime);
	CU_ASSERT_EQUAL(0,wait);
}
#endif

void test_checkRootDelete(){
	webconfig_tmp_data_t *tmpData = (webconfig_tmp_data_t *)malloc(sizeof(webconfig_tmp_data_t));
	tmpData->name = strdup("moca");
	tmpData->version = 1234;
	tmpData->status = strdup("success");
	tmpData->trans_id = 4104;
	tmpData->retry_count = 0;
	tmpData->error_code = 0;
	tmpData->error_details = strdup("none");
	tmpData->next = NULL;
	set_global_tmp_node(tmpData);
	int m=checkRootDelete();
	CU_ASSERT_EQUAL(0,m);
	if(tmpData)
	{
		WEBCFG_FREE(tmpData->name);
		WEBCFG_FREE(tmpData->status);
		WEBCFG_FREE(tmpData->error_details);
		WEBCFG_FREE(tmpData);
	}

}

void test_checkRootDeleteFailure(){
	webconfig_tmp_data_t *tmpData = (webconfig_tmp_data_t *)malloc(sizeof(webconfig_tmp_data_t));
	tmpData->name = strdup("privatessid");
	tmpData->version = 1234;
	tmpData->status = strdup("pending");
	tmpData->trans_id = 4104;
	tmpData->retry_count = 0;
	tmpData->error_code = 0;
	tmpData->error_details = strdup("failed");
	tmpData->next = NULL;
	set_global_tmp_node(tmpData);
	int m=checkRootDelete();
	CU_ASSERT_EQUAL(1,m);
	if(tmpData)
	{
		WEBCFG_FREE(tmpData->name);
		WEBCFG_FREE(tmpData->status);
		WEBCFG_FREE(tmpData->error_details);
		WEBCFG_FREE(tmpData);
	}
}
void test_updateRootVersionToDB(){
	webconfig_tmp_data_t *tmpData = (webconfig_tmp_data_t *)malloc(sizeof(webconfig_tmp_data_t));
	tmpData->name = strdup("root");
	tmpData->version = 232323;
	tmpData->status = strdup("pending");
	tmpData->trans_id = 4104;
	tmpData->retry_count = 0;
	tmpData->error_code = 0;
	tmpData->error_details = strdup("none");
	tmpData->next = NULL;
	set_global_tmp_node(tmpData);
	set_global_ETAG("123");
	int m=updateRootVersionToDB();
	CU_ASSERT_EQUAL(0,m);
	if(tmpData)
	{
		WEBCFG_FREE(tmpData->name);
		WEBCFG_FREE(tmpData->status);
		WEBCFG_FREE(tmpData->error_details);
		WEBCFG_FREE(tmpData);
	}

}

void test_updateRootVersionToDBNoroot(){
	webconfig_tmp_data_t *tmpData = (webconfig_tmp_data_t *)malloc(sizeof(webconfig_tmp_data_t));
	tmpData->name = strdup("wan");
	tmpData->version = 232323;
	tmpData->status = strdup("pending");
	tmpData->trans_id = 4104;
	tmpData->retry_count = 0;
	tmpData->error_code = 0;
	tmpData->error_details = strdup("none");
	tmpData->next = NULL;
	set_global_tmp_node(tmpData);
	set_global_ETAG("123");
	int m=updateRootVersionToDB();
	CU_ASSERT_EQUAL(0,m);
	if(tmpData)
	{
		WEBCFG_FREE(tmpData->name);
		WEBCFG_FREE(tmpData->status);
		WEBCFG_FREE(tmpData->error_details);
		WEBCFG_FREE(tmpData);
	}

}
void test_set_global_ETAG(){
	set_global_ETAG("1234");
	CU_ASSERT_STRING_EQUAL(get_global_ETAG(),"1234");
}

void test_get_global_ETAG(){
	printf("etag is %s\n", get_global_ETAG());
	CU_ASSERT_FATAL( NULL != get_global_ETAG() );
}

void test_set_global_transID(){
	set_global_transID("3344411a");
	CU_ASSERT_STRING_EQUAL(get_global_transID(),"3344411a");
}

void test_get_global_transID(){
	printf("transID is %s\n", get_global_transID());
	CU_ASSERT_FATAL( NULL != get_global_transID() );
}
void test_set_global_contentLen(){
	set_global_contentLen("1001");
	CU_ASSERT_STRING_EQUAL(get_global_contentLen(),"1001");
}

void test_get_global_contentLen(){
	printf("contentLen is %s\n", get_global_contentLen());
	CU_ASSERT_FATAL( NULL != get_global_contentLen() );
}
void test_set_global_eventFlag(){
	set_global_eventFlag(1);
	CU_ASSERT_EQUAL(get_global_eventFlag(),1);
}

void test_get_global_eventFlag(){
	printf("eventFlag is %d\n", get_global_eventFlag());
	CU_ASSERT_FATAL( 0 != get_global_eventFlag() );
}

void test_reset_global_eventFlag(){
	reset_global_eventFlag();
	CU_ASSERT_EQUAL(get_global_eventFlag(),0);
}

#ifdef WAN_FAILOVER_SUPPORTED
void test_set_global_interface(){
	set_global_interface("eth0");
	CU_ASSERT_STRING_EQUAL(get_global_interface(),"eth0");
}

void test_get_global_interface(){
	printf("interface is %s\n", get_global_interface());
	CU_ASSERT_FATAL( NULL != get_global_interface() );
}
#endif
void test_print_tmp_doc_list(){
	webconfig_tmp_data_t *tmpData = (webconfig_tmp_data_t *)malloc(sizeof(webconfig_tmp_data_t));
	tmpData->name = strdup("wan");
	tmpData->version = 232323;
	tmpData->status = strdup("pending");
	tmpData->trans_id = 4104;
	tmpData->retry_count = 0;
	tmpData->error_code = 0;
	tmpData->error_details = strdup("none");
	tmpData->next = NULL;
	int m=print_tmp_doc_list(1);
	CU_ASSERT_EQUAL(0,m);
	if(tmpData)
	{
		WEBCFG_FREE(tmpData->name);
		WEBCFG_FREE(tmpData->status);
		WEBCFG_FREE(tmpData->error_details);
		WEBCFG_FREE(tmpData);
	}

}
void test_get_global_mp_null(){
	CU_ASSERT_FATAL( NULL == get_global_mp());
}

void test_get_global_mp(){
	multipartdocs_t *multipartdocs = (multipartdocs_t *)malloc(sizeof(multipartdocs_t));
	multipartdocs->name_space = strdup("portforwarding");
	multipartdocs->data = (char* )malloc(64);
	multipartdocs->isSupplementarySync = 0;
	multipartdocs->next = NULL;
	set_global_mp(multipartdocs);
	CU_ASSERT_FATAL( NULL !=get_global_mp());
	if(multipartdocs)
	{
		WEBCFG_FREE(multipartdocs->name_space);
		WEBCFG_FREE(multipartdocs->data);
		multipartdocs->data_size = 0;
		WEBCFG_FREE(multipartdocs);
		set_global_mp(NULL);
	}
}

void test_deleteRootAndMultipartDocs(){
	multipartdocs_t *multipartdocs = (multipartdocs_t *)malloc(sizeof(multipartdocs_t));
	multipartdocs->name_space = strdup("moca");
	multipartdocs->data = (char* )malloc(64);
	multipartdocs->isSupplementarySync = 0;
	multipartdocs->next = NULL;
	set_global_mp(multipartdocs);
	CU_ASSERT_FATAL( NULL !=get_global_mp());
	webconfig_tmp_data_t *tmpData = (webconfig_tmp_data_t *)malloc(sizeof(webconfig_tmp_data_t));
	tmpData->name = strdup("moca");
	tmpData->version = 1234;
	tmpData->status = strdup("success");
	tmpData->trans_id = 4104;
	tmpData->retry_count = 0;
	tmpData->error_code = 0;
	tmpData->error_details = strdup("none");
	tmpData->next = NULL;
	set_global_tmp_node(tmpData);
	int m=deleteRootAndMultipartDocs();
	CU_ASSERT_EQUAL(0,m);
	set_global_tmp_node(NULL);
	set_global_mp(NULL);
}

void test_deleteRootAndMultipartDocs_fail(){
	multipartdocs_t *multipartdocs = (multipartdocs_t *)malloc(sizeof(multipartdocs_t));
	multipartdocs->name_space = strdup("wan");
	multipartdocs->data = (char* )malloc(64);
	multipartdocs->isSupplementarySync = 0;
	multipartdocs->next = NULL;
	set_global_mp(multipartdocs);
	CU_ASSERT_FATAL( NULL !=get_global_mp());
	webconfig_tmp_data_t *tmpData = (webconfig_tmp_data_t *)malloc(sizeof(webconfig_tmp_data_t));
	tmpData->name = strdup("wan");
	tmpData->version = 3456;
	tmpData->status = strdup("pending");
	tmpData->trans_id = 1231;
	tmpData->retry_count = 0;
	tmpData->error_code = 0;
	tmpData->error_details = strdup("failed");
	tmpData->next = NULL;
	set_global_tmp_node(tmpData);
	int m=deleteRootAndMultipartDocs();
	CU_ASSERT_EQUAL(0,m);
	set_global_tmp_node(NULL);
	set_global_mp(NULL);
}

void test_deleteFromMpList(){
	multipartdocs_t *multipartdocs = (multipartdocs_t *)malloc(sizeof(multipartdocs_t));
	multipartdocs->name_space = strdup("wan");
	multipartdocs->data = (char* )malloc(64);
	multipartdocs->isSupplementarySync = 0;
	multipartdocs->next = NULL;
	set_global_mp(multipartdocs);
	int m= deleteFromMpList("wan");
	CU_ASSERT_EQUAL(0,m);
	set_global_mp(NULL);
}

void test_deleteFromMpListFailure(){
	multipartdocs_t *multipartdocs = (multipartdocs_t *)malloc(sizeof(multipartdocs_t));
	multipartdocs->name_space = strdup("wan");
	multipartdocs->data = (char* )malloc(64);
	multipartdocs->isSupplementarySync = 0;
	multipartdocs->next = NULL;
	set_global_mp(multipartdocs);
	int m= deleteFromMpList("moca");
	CU_ASSERT_EQUAL(1,m);
	set_global_mp(NULL);
}

void test_deleteFromMpListInvalidDoc(){
	multipartdocs_t *multipartdocs = (multipartdocs_t *)malloc(sizeof(multipartdocs_t));
	multipartdocs->name_space = strdup("wan");
	multipartdocs->data = (char* )malloc(64);
	multipartdocs->isSupplementarySync = 0;
	multipartdocs->next = NULL;
	set_global_mp(multipartdocs);
	int m= deleteFromMpList(NULL);
	CU_ASSERT_EQUAL(1,m);
	set_global_mp(NULL);
}

void test_deleteFromMpList_2docs(){
	addToMpList(123, "wan", "data1", 10);
	addToMpList(1234, "moca", "data2", 20);
	int m= deleteFromMpList("moca");
	CU_ASSERT_EQUAL(0,m);
	set_global_mp(NULL);
}

void test_addToMpList(){
	addToMpList(123, "wan", "data1", 10);
        CU_ASSERT_STRING_EQUAL(get_global_mp()->name_space, "wan");
	CU_ASSERT_EQUAL(get_global_mp()->etag,123);
	addToMpList(1234, "moca", "data2", 20);
	CU_ASSERT_STRING_EQUAL(get_global_mp()->next->name_space, "moca");
	CU_ASSERT_EQUAL(get_global_mp()->next->etag,1234);
	delete_multipart();
	set_global_mp(NULL);
}

void test_delete_mp_doc(){
	addToMpList(123, "wan", "data1", 10);
	addToMpList(1234, "moca", "data2", 20);
	delete_mp_doc("moca");
	delete_mp_doc("wan");
	CU_ASSERT_FATAL( NULL == get_global_mp() );
}

void test_get_multipartdoc_count(){
	addToMpList(44, "wan", "data1", 10);
	addToMpList(555, "moca", "data2", 20);
	addToMpList(666, "privatessid", "data3", 20);
	addToMpList(7777, "lan", "data4", 30);
	addToMpList(1111, "mesh", "data5", 20);
	CU_ASSERT_EQUAL(5,get_multipartdoc_count());
	delete_multipart();
	CU_ASSERT_FATAL( NULL == get_global_mp() );
}

int getEncodedBlob(char *data, char **encodedData);
static void packJsonString( cJSON *item, msgpack_packer *pk );
static void packJsonNumber( cJSON *item, msgpack_packer *pk );
static void packJsonArray( cJSON *item, msgpack_packer *pk, int isBlob );
static void packJsonObject( cJSON *item, msgpack_packer *pk, int isBlob);
static void __msgpack_pack_string( msgpack_packer *pk, const void *string, size_t n );
static int convertJsonToMsgPack(char *data, char **encodedData, int isBlob);



static void packBlobData(cJSON *item, msgpack_packer *pk )
{
	char *blobData = NULL, *encodedBlob = NULL;
	int len = 0;
	WebcfgDebug("------ %s ------\n",__FUNCTION__);
	blobData = strdup(item->valuestring);
	if(strlen(blobData) > 0)
	{
		WebcfgDebug("%s\n",blobData);
		len = getEncodedBlob(blobData, &encodedBlob);
		WebcfgDebug("%s\n",encodedBlob);
	}
	__msgpack_pack_string(pk, item->string, strlen(item->string));
	__msgpack_pack_string(pk, encodedBlob, len);
	free(encodedBlob);
	free(blobData);
	WebcfgDebug("------ %s ------\n",__FUNCTION__);
}

static int getItemsCount(cJSON *object)
{
	int count = 0;
	while(object != NULL)
	{
		object = object->next;
		count++;
	}
	return count;
}

static void packJsonNumber( cJSON *item, msgpack_packer *pk )
{
	WebcfgDebug("%s:%d\n",__FUNCTION__,item->valueint);
	if(item->string != NULL)
	{
		__msgpack_pack_string(pk, item->string, strlen(item->string));
	}
	msgpack_pack_int(pk, item->valueint);
}
static void packJsonArray(cJSON *item, msgpack_packer *pk, int isBlob)
{
	int arraySize = cJSON_GetArraySize(item);
	WebcfgDebug("%s:%s\n",__FUNCTION__, item->string);
	if(item->string != NULL)
	{
		//printf("packing %s\n",item->string);
		__msgpack_pack_string(pk, item->string, strlen(item->string));
	}
	msgpack_pack_array( pk, arraySize );
	int i=0;
	for(i=0; i<arraySize; i++)
	{
		cJSON *arrItem = cJSON_GetArrayItem(item, i);
		switch((arrItem->type) & 0XFF)
		{
			case cJSON_Object:
				packJsonObject(arrItem, pk, isBlob);
				break;
		}
	}
}

static void packJsonString( cJSON *item, msgpack_packer *pk )
{
	if(item->string != NULL)
	{
		__msgpack_pack_string(pk, item->string, strlen(item->string));
	}
	__msgpack_pack_string(pk, item->valuestring, strlen(item->valuestring));
}

static void __msgpack_pack_string( msgpack_packer *pk, const void *string, size_t n )
{
	WebcfgDebug("%s:%s\n",__FUNCTION__,(char *)string);
    msgpack_pack_str( pk, n );
    msgpack_pack_str_body( pk, string, n );
}

static void packJsonObject( cJSON *item, msgpack_packer *pk, int isBlob )
{
	WebcfgDebug("%s\n",__FUNCTION__);
	cJSON *child = item->child;
	msgpack_pack_map( pk, getItemsCount(child));
	while(child != NULL)
	{
		switch((child->type) & 0XFF)
		{
			case cJSON_String:
				if(child->string != NULL && (strcmp(child->string, "value") == 0) && isBlob == 1)
				{
					packBlobData(child, pk);
				}
				else
				{
					packJsonString(child, pk);
				}
				break;
			case cJSON_Number:
				packJsonNumber(child, pk);
				break;
			case cJSON_Array:
				packJsonArray(child, pk, isBlob);
				break;
		}
		child = child->next;
	}
}

int getEncodedBlob(char *data, char **encodedData)
{
	cJSON *jsonData=NULL;
	int encodedDataLen = 0;
	WebcfgDebug("------- %s -------\n",__FUNCTION__);
	jsonData=cJSON_Parse(data);
	if(jsonData != NULL)
	{
		msgpack_sbuffer sbuf1;
		msgpack_packer pk1;
		msgpack_sbuffer_init( &sbuf1 );
		msgpack_packer_init( &pk1, &sbuf1, msgpack_sbuffer_write );
		packJsonObject(jsonData, &pk1, 1);
		if( sbuf1.data )
		{
		    *encodedData = ( char * ) malloc( sizeof( char ) * sbuf1.size );
		    if( NULL != *encodedData )
			{
		        memcpy( *encodedData, sbuf1.data, sbuf1.size );
			}
			encodedDataLen = sbuf1.size;
		}
		msgpack_sbuffer_destroy(&sbuf1);
		cJSON_Delete(jsonData);
	}
	WebcfgDebug("------- %s -------\n",__FUNCTION__);
	return encodedDataLen;
}

static int convertJsonToMsgPack(char *data, char **encodedData, int isBlob)
{
	cJSON *jsonData=NULL;
	int encodedDataLen = 0;
	jsonData=cJSON_Parse(data);
	if(jsonData != NULL)
	{
		msgpack_sbuffer sbuf;
		msgpack_packer pk;
		msgpack_sbuffer_init( &sbuf );
		msgpack_packer_init( &pk, &sbuf, msgpack_sbuffer_write );
		packJsonObject(jsonData, &pk, isBlob);
		if( sbuf.data )
		{
		    *encodedData = ( char * ) malloc( sizeof( char ) * sbuf.size );
		    if( NULL != *encodedData )
			{
		        memcpy( *encodedData, sbuf.data, sbuf.size );
			}
			encodedDataLen = sbuf.size;
		}
		msgpack_sbuffer_destroy(&sbuf);
		cJSON_Delete(jsonData);
	}
	return encodedDataLen;
}

rbusError_t webcfgBlobElementSetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts) {

	(void) handle;
    	(void) opts;
    	char const* paramName = rbusProperty_GetName(prop);

    	WebcfgInfo("Parameter name is %s \n", paramName);
    	rbusValueType_t type_t;
    	rbusValue_t paramValue_t = rbusProperty_GetValue(prop);
    	if(paramValue_t) {
        	type_t = rbusValue_GetType(paramValue_t);
    	} else {
		WebcfgError("Invalid input to set\n");
        	return RBUS_ERROR_INVALID_INPUT;
    	}
   
    	if(strncmp(paramName, WEBCFG_BLOB_PARAM, maxParamLen) == 0){
        	if(type_t == RBUS_BYTES) {
            		int len;
	    		uint8_t const* data = rbusValue_GetBytes(paramValue_t, &len);
	    		CU_ASSERT_FATAL(NULL != data);
        	}
         	else {
            		WebcfgError("Unexpected value type for property %s\n", paramName);
	    		return RBUS_ERROR_INVALID_INPUT;
        	}
    	}
	else {
		WebcfgError("Unexpected parameter = %s\n", paramName);
		return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
	}   
     	return RBUS_ERROR_SUCCESS;
}


void test_processMsgpackSubdoc()
{
	int status = webconfigRbusInit("consumerComponent"); 
	if(status)
	{
		WebcfgInfo("rbus init success\n");
	}
	else{
		WebcfgError("rbus init failed");
	}

	int res = rbus_open(&rbus_handle, "providerComponent");
    	if(res != RBUS_ERROR_SUCCESS)
	{
		CU_FAIL("rbus_open failed for providerComponent");
	}

	rbusDataElement_t webcfgBlobElement[1] = {
		{WEBCFG_BLOB_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, webcfgBlobElementSetHandler, NULL, NULL, NULL, NULL}}
	};
	rbusError_t ret = rbus_regDataElements(rbus_handle, 1, webcfgBlobElement);
	CU_ASSERT_EQUAL(ret, RBUS_ERROR_SUCCESS);
	
	SubDocSupportMap_t *supportdocs = (SubDocSupportMap_t *)malloc(sizeof(SubDocSupportMap_t));
	if(supportdocs)
	{
		strcpy(supportdocs->name,"value");
		strcpy(supportdocs->support,"true");
		strcpy(supportdocs->rbus_listener,"true");
		strcpy(supportdocs->dest,WEBCFG_BLOB_PARAM);
		supportdocs->next=NULL;
	}
	set_global_sdInfoHead(supportdocs);
	set_global_supplementarySync(0);
	

	char *transaction_id = strdup("1234");
	char *Data = "{\"parameters\": [{\"name\":\"Device.DeviceInfo.Test\",\"value\":\"false\",\"dataType\":12},{\"name\":\"Device.DeviceInfo.Test1\",\"value\":\"true\",\"dataType\":12}]}";

	char *encodedData = NULL;
	int encodedLen = 0;

	encodedLen = convertJsonToMsgPack(Data, &encodedData, 1);
	if(encodedLen)
	{
		multipartdocs_t *node = (multipartdocs_t *)malloc(sizeof(multipartdocs_t));
		if (node != NULL)
    {
			node->etag = 345431215;
			node->name_space = strdup("value"); // Assuming strdup is available
			node->data = (char *)malloc(encodedLen);
			if (node->data != NULL)
			{
				memcpy(node->data, encodedData, encodedLen);
			}
			node->data_size = encodedLen;
			node->isSupplementarySync = 0;
			node->next = NULL;
			WEBCFG_STATUS result;
			set_global_mp(node);
			result = processMsgpackSubdoc(transaction_id);
			CU_ASSERT_EQUAL(result, WEBCFG_FAILURE);
	}
	
	}
	//rbus_close(rbus_handle);
	//webpaRbus_Uninit(); 
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "tests", NULL, NULL );
      CU_add_test( *suite, "test  generate_trans_uuid", test_generate_trans_uuid);
      CU_add_test( *suite, "test  replaceMacWord", test_replaceMac);  
      CU_add_test( *suite, "test  createCurlHeader", test_createHeader);
      CU_add_test( *suite, "test  validateParam", test_validateParam);
      CU_add_test( *suite, "test  checkRootUpdate", test_checkRootUpdate);
      CU_add_test( *suite, "test  checkRootDelete", test_checkRootDelete);
      CU_add_test( *suite, "test  checkRootDeleteFailure", test_checkRootDeleteFailure);
      CU_add_test( *suite, "test  updateRootVersionToDB", test_updateRootVersionToDB);
      CU_add_test( *suite, "test  updateRootVersionToDBNoroot", test_updateRootVersionToDBNoroot);
      CU_add_test( *suite, "test  set_global_ETAG", test_set_global_ETAG);
      CU_add_test( *suite, "test  get_global_ETAG", test_get_global_ETAG);
      CU_add_test( *suite, "test  set_global_transID", test_set_global_transID);
      CU_add_test( *suite, "test  get_global_transID", test_get_global_transID);
      CU_add_test( *suite, "test  set_global_contentLen", test_set_global_contentLen);
      CU_add_test( *suite, "test  get_global_contentLen", test_get_global_contentLen);
      CU_add_test( *suite, "test  set_global_eventFlag", test_set_global_eventFlag);
      CU_add_test( *suite, "test  get_global_eventFlag", test_get_global_eventFlag);
      CU_add_test( *suite, "test  reset_global_eventFlag", test_reset_global_eventFlag);
#ifdef WAN_FAILOVER_SUPPORTED
      CU_add_test( *suite, "test  set_global_interface", test_set_global_interface);
      CU_add_test( *suite, "test  get_global_interface", test_get_global_interface);
#endif
      CU_add_test( *suite, "test  print_tmp_doc_list", test_print_tmp_doc_list);
      CU_add_test( *suite, "test  get_global_mp_null", test_get_global_mp_null);
      CU_add_test( *suite, "test  get_global_mp", test_get_global_mp);
      CU_add_test( *suite, "test  deleteRootAndMultipartDocs", test_deleteRootAndMultipartDocs);
      CU_add_test( *suite, "test  deleteRootAndMultipartDocs_fail", test_deleteRootAndMultipartDocs_fail);
      CU_add_test( *suite, "test  checkDBList", test_checkDBList);
      CU_add_test( *suite, "test  updateDBlist", test_updateDBlist);
      CU_add_test( *suite, "test  pack append doc", test_appendedDoc);
#ifdef FEATURE_SUPPORT_AKER
      CU_add_test( *suite, "test  Aker update blob send", test_UpdateErrorsendAkerblob);
      CU_add_test( *suite, "test  Aker delete blob send", test_DeleteErrorsendAkerblob);
      CU_add_test( *suite, "test  Aker wait", test_akerWait);
#endif
      CU_add_test( *suite, "test  deleteFromMpList", test_deleteFromMpList);
      CU_add_test( *suite, "test  deleteFromMpListFailure", test_deleteFromMpListFailure);
      CU_add_test( *suite, "test  deleteFromMpListInvalidDoc", test_deleteFromMpListInvalidDoc);
      CU_add_test( *suite, "test  deleteFromMpList_2docs", test_deleteFromMpList_2docs);
      CU_add_test( *suite, "test  addToMpList", test_addToMpList);
      CU_add_test( *suite, "test  delete_mp_doc", test_delete_mp_doc);
      CU_add_test( *suite, "test  get_multipartdoc_count", test_get_multipartdoc_count);
	  CU_add_test( *suite, "test  processMsgpackSubdoc", test_processMsgpackSubdoc);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;
 
    (void ) argc;
    (void ) argv;
    
    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}
