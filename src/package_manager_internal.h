/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __TIZEN_APPFW_PACKAGE_MANAGER_INTERNAL_H__
#define __TIZEN_APPFW_PACKAGE_MANAGER_INTERNAL_H__

#include <dlog.h>
#include <pkgmgr-info.h>
#include "package_manager.h"

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "CAPI_APPFW_PACKAGE_MANAGER"

#define _LOGE(fmt, arg...) LOGE(fmt, ##arg)
#define _LOGD(fmt, arg...) LOGD(fmt, ##arg)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PRIVILEGE_PACKAGE_MANAGER_INFO,
	PRIVILEGE_PACKAGE_MANAGER_ADMIN,
	PRIVILEGE_PACKAGE_MANAGER_CACHE
} privilege_type;

int check_privilege(privilege_type type);

int package_manager_error(package_manager_error_e error, const char* function, const char *description);

int package_info_get_package_info(const char *package, package_info_h *package_info);

int package_info_foreach_package_info(package_manager_package_info_cb callback,	void *user_data);

int package_info_filter_foreach_package_info(pkgmgrinfo_pkginfo_filter_h handle, package_manager_package_info_cb callback, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_APPFW_PACKAGE_MANAGER_INTERNAL_H__ */
