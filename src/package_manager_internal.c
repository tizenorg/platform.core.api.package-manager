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

#include <unistd.h>

#include <package-manager.h>
#include <pkgmgr-info.h>
#include <tzplatform_config.h>

#include "package_info.h"
#include "package_manager.h"
#include "package_manager_internal.h"

#define GLOBAL_USER tzplatform_getuid(TZ_SYS_GLOBALAPP_USER)

typedef struct _foreach_pkg_context_{
	package_manager_package_info_cb callback;
	void *user_data;
} foreach_pkg_context_s;

static const char *package_manager_error_to_string(package_manager_error_e
						   error)
{
	switch (error) {
	case PACKAGE_MANAGER_ERROR_NONE:
		return "NONE";
	case PACKAGE_MANAGER_ERROR_INVALID_PARAMETER:
		return "INVALID_PARAMETER";
	case PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY:
		return "OUT_OF_MEMORY";
	case PACKAGE_MANAGER_ERROR_IO_ERROR:
		return "IO_ERROR";
	case PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE:
		return "NO_SUCH_PACKAGE";
	case PACKAGE_MANAGER_ERROR_PERMISSION_DENIED:
		return "PERMISSION_DENIED";
	case PACKAGE_MANAGER_ERROR_SYSTEM_ERROR:
		return "SEVERE_SYSTEM_ERROR";
	default:
		return "UNKNOWN";
	}
}

int package_manager_error(package_manager_error_e error,
				 const char *function, const char *description)
{
	if (description) {
		_LOGE("[%s] %s(0x%08x) : %s", function,
		     package_manager_error_to_string(error), error,
		     description);
	} else {
		_LOGE("[%s] %s(0x%08x)", function,
		     package_manager_error_to_string(error), error);
	}

	return error;
}

int package_info_get_package_info(const char *package, package_info_h *package_info)
{
	return package_info_create(package, package_info);
}

static int package_info_foreach_package_info_cb(const pkgmgr_pkginfo_h handle, void *user_data)
{
	char *pkg_name = NULL;
	foreach_pkg_context_s *foreach_pkg_context = user_data;
	package_info_h package_info = NULL;

	if (handle == NULL || foreach_pkg_context == NULL)
	{
		package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
		return PKGMGR_R_EINVAL;
	}

	pkgmgr_pkginfo_get_pkgname(handle, &pkg_name);

	if (package_info_create(pkg_name, &package_info) == PACKAGE_MANAGER_ERROR_NONE)
	{
		foreach_pkg_context->callback(package_info, foreach_pkg_context->user_data);
		package_info_destroy(package_info);
	}

	/* pkgmgr_get_info_list() needs to be enhanced to stop and continue callback */
	return PKGMGR_R_OK;
}

int package_info_foreach_package_info(package_manager_package_info_cb callback, void *user_data)
{
	foreach_pkg_context_s foreach_pkg_context = {
		.callback = callback,
		.user_data = user_data,
	};
	int ret = 0;

	if (callback == NULL)
	{
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
	}
	uid_t uid = getuid();
	if (uid != GLOBAL_USER)
		ret = pkgmgr_pkginfo_get_usr_list(package_info_foreach_package_info_cb, &foreach_pkg_context, uid);
	else
		ret = pkgmgr_pkginfo_get_list(package_info_foreach_package_info_cb, &foreach_pkg_context);
	if (ret < 0) {
		return PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE;
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}

int package_info_filter_foreach_package_info(pkgmgrinfo_pkginfo_filter_h handle, package_manager_package_info_cb callback, void *user_data)
{
	foreach_pkg_context_s foreach_pkg_context = {
		.callback = callback,
		.user_data = user_data,
	};
	int ret;
	uid_t uid;

	if ((handle == NULL) || (callback == NULL))
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	uid = getuid();
	if (uid != GLOBAL_USER)
		ret = pkgmgrinfo_pkginfo_usr_filter_foreach_pkginfo(handle, package_info_foreach_package_info_cb, &foreach_pkg_context, uid);
	else
		ret = pkgmgrinfo_pkginfo_filter_foreach_pkginfo(handle, package_info_foreach_package_info_cb, &foreach_pkg_context);

	if (ret < 0)
		return PACKAGE_MANAGER_ERROR_IO_ERROR;

	return PACKAGE_MANAGER_ERROR_NONE;
}

