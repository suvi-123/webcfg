/*
 * Copyright 2021 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stdlib.h"
#include "signal.h"
#ifdef WEBCONFIG_HTTP_SUPPORT
#include <curl/curl.h>
#endif
#ifdef INCLUDE_BREAKPAD
#ifndef DEVICE_CAMERA
#include "breakpad_wrapper.h"
#else
#include "breakpadwrap.h"
#endif  //DEVICE_CAMERA
#endif
#include "webcfg.h"
#include "webcfg_log.h"
#include "webcfg_generic.h"
#include "webcfg_rbus.h"
#include "webcfg_privilege.h"
#include <unistd.h>
#include <pthread.h>

#ifdef WEBCONFIG_MQTT_SUPPORT
#include "webcfg_mqtt.h"
#endif
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
#ifndef INCLUDE_BREAKPAD
static void sig_handler(int sig);
#endif

pthread_mutex_t webcfg_mut= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  webcfg_con= PTHREAD_COND_INITIALIZER;
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main()
{
	char RfcEnable[64];
	memset(RfcEnable, 0, sizeof(RfcEnable));
	char* strValue = NULL;
	int ret = 0;
	int systemStatus = -1;
    	struct timespec cTime;
    	char systemReadyTime[32]={'\0'};
#ifdef INCLUDE_BREAKPAD
#ifndef DEVICE_CAMERA
    breakpad_ExceptionHandler();
#else
    /* breakpad handles the signals SIGSEGV, SIGBUS, SIGFPE, and SIGILL */
    BreakPadWrapExceptionHandler eh;
    eh = newBreakPadWrapExceptionHandler();
    if(NULL != eh) {
        WebcfgInfo("Breakpad Initialized\n");
    }
#endif //DEVICE_CAMERA
#else
	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGBUS, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGFPE, sig_handler);
	signal(SIGILL, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGHUP, sig_handler);
	signal(SIGALRM, sig_handler);
#endif
	WebcfgInfo("********** Starting component: %s **********\n ", WEBCFG_COMPONENT_NAME);
	webcfg_drop_root_privilege();
#if !defined (WEBCONFIG_MQTT_SUPPORT) || defined (WEBCONFIG_HTTP_SUPPORT)
	curl_global_init(CURL_GLOBAL_DEFAULT);
#endif
	if(isRbusEnabled())
	{
		registerRbusLogger();
		WebcfgDebug("RBUS mode. webconfigRbusInit\n");
		webconfigRbusInit(WEBCFG_COMPONENT_NAME);
		regWebConfigDataModel();
		#ifdef WAN_FAILOVER_SUPPORTED
		subscribeTo_CurrentActiveInterface_Event();
		#endif
		systemStatus = rbus_waitUntilSystemReady();
		WebcfgDebug("rbus_waitUntilSystemReady systemStatus is %d\n", systemStatus);
    		getCurrent_Time(&cTime);
    		snprintf(systemReadyTime, sizeof(systemReadyTime),"%d", (int)cTime.tv_sec);
    		WebcfgInfo("systemReadyTime is %s\n", systemReadyTime);
		set_global_systemReadyTime(systemReadyTime);
		// wait for upstream subscriber for 5mins
		waitForUpstreamEventSubscribe(5);
		ret = rbus_GetValueFromDB( PARAM_RFC_ENABLE, &strValue );
		if (ret == 0)
		{
			WebcfgDebug("RFC strValue %s\n", strValue);
			if(strValue != NULL)
			{
				webcfgStrncpy(RfcEnable, strValue, sizeof(RfcEnable));
			}
		}
		if(RfcEnable[0] != '\0' && strncmp(RfcEnable, "true", strlen("true")) == 0)
		{
			if(get_global_mpThreadId() == NULL)
			{
				WebcfgInfo("WebConfig Rfc is enabled, starting initWebConfigMultipartTask.\n");
				initWebConfigMultipartTask((unsigned long) systemStatus);
			#ifdef WEBCONFIG_MQTT_SUPPORT
				WebcfgInfo("Starting initWebconfigMqttTask\n");
				initWebconfigMqttTask((unsigned long) systemStatus);
			#else
				WebcfgInfo("mqtt is disabled..\n");
			#endif
			}
			else
			{
				WebcfgInfo("Webconfig is already started, so not starting again.\n");
			}
		}
		else
		{
			WebcfgInfo("WebConfig Rfc Flag is not enabled\n");
		}
	}
	else
	{
		WebcfgInfo("DBUS mode. Webconfig bin is not supported in Dbus\n");
	}

	WebcfgDebug("pthread_mutex_lock webcfg_mut and wait.\n");
	pthread_mutex_lock(&webcfg_mut);
	pthread_cond_wait(&webcfg_con, &webcfg_mut);
	WebcfgDebug("pthread_mutex_unlock webcfg_mut\n");
	pthread_mutex_unlock (&webcfg_mut);
#if !defined (WEBCONFIG_MQTT_SUPPORT) || defined (WEBCONFIG_HTTP_SUPPORT)
	curl_global_cleanup();
#endif
	WebcfgInfo("Exiting webconfig main thread!!\n");
	return 1;
}

const char *rdk_logger_module_fetch(void)
{
    return "LOG.RDK.WEBCONFIG";
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
#ifndef INCLUDE_BREAKPAD
static void sig_handler(int sig)
{

	if ( sig == SIGINT ) 
	{
		signal(SIGINT, sig_handler); /* reset it to this function */
		WebcfgError("SIGINT received!\n");
		exit(0);
	}
	else if ( sig == SIGUSR1 ) 
	{
		signal(SIGUSR1, sig_handler); /* reset it to this function */
		WebcfgError("SIGUSR1 received!\n");
	}
	else if ( sig == SIGUSR2 ) 
	{
		WebcfgError("SIGUSR2 received!\n");
	}
	else if ( sig == SIGCHLD ) 
	{
		signal(SIGCHLD, sig_handler); /* reset it to this function */
		WebcfgError("SIGHLD received!\n");
	}
	else if ( sig == SIGPIPE ) 
	{
		signal(SIGPIPE, sig_handler); /* reset it to this function */
		WebcfgError("SIGPIPE received!\n");
	}
	else if ( sig == SIGALRM ) 
	{
		signal(SIGALRM, sig_handler); /* reset it to this function */
		WebcfgError("SIGALRM received!\n");
	}
	else 
	{
		WebcfgError("Signal %d received!\n", sig);
		exit(0);
	}
	
}
#endif
