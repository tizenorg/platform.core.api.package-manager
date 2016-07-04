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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <pkgmgr-info.h>
#include <tzplatform_config.h>
#include <cynara-client.h>
#include <cynara-session.h>

#include "package_info.h"
#include "package_manager.h"
#include "package_manager_internal.h"

#define MAX_SMACK_LABEL_LEN 255

typedef struct _foreach_pkg_context_ {
	package_manager_package_info_cb callback;
	void *user_data;
} foreach_pkg_context_s;

int check_privilege(privilege_type type)
{
	cynara *p_cynara;
	int fd;
	int ret;
	char subject_label[MAX_SMACK_LABEL_LEN] = { 0 };
	char uid[10];
	char *session;
	const char *privilege;

	fd = open("/proc/self/attr/current", O_RDONLY);
	if (fd < 0) {
		LOGE("open failed: %d", errno);
		return PACKAGE_MANAGER_ERROR_IO_ERROR;
	}

	ret = read(fd, subject_label, sizeof(subject_label));
	if (ret < 0) {
		LOGE("read failed: %d", errno);
		close(fd);
		return PACKAGE_MANAGER_ERROR_IO_ERROR;
	}
	close(fd);

	ret = cynara_initialize(&p_cynara, NULL);
	if (ret != CYNARA_API_SUCCESS) {
		LOGE("init cynara failed: %d", ret);
		return PACKAGE_MANAGER_ERROR_IO_ERROR;
	}

	snprintf(uid, 10, "%d", getuid());
	session = cynara_session_from_pid(getpid());

	switch (type) {
	case PRIVILEGE_PACKAGE_MANAGER_INFO:
		privilege = "http://tizen.org/privilege/packagemanager.info";
		break;
	case PRIVILEGE_PACKAGE_MANAGER_ADMIN:
		privilege = "http://tizen.org/privilege/packagemanager.admin";
		break;
	case PRIVILEGE_PACKAGE_MANAGER_CACHE:
		privilege =
			"http://tizen.org/privilege/packagemanager.clearcache";
		break;
	default:
		privilege = NULL;
		break;
	}

	ret = cynara_check(p_cynara, subject_label, session, uid, privilege);

	free(session);
	cynara_finish(p_cynara);

	if (ret != CYNARA_API_ACCESS_ALLOWED) {
		LOGE("cynara access check failed: %d", ret);
		return PACKAGE_MANAGER_ERROR_PERMISSION_DENIED;
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}

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

static int package_info_foreach_package_info_cb(const pkgmgrinfo_pkginfo_h handle, void *user_data)
{
	foreach_pkg_context_s *foreach_pkg_context = user_data;
	package_info_h package_info = NULL;
	bool r = false;
	int ret;

	if (handle == NULL || foreach_pkg_context == NULL) {
		package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
		return PMINFO_R_EINVAL;
	}

	ret = package_info_create_with_pkginfo(handle, &package_info);
	if (ret != PACKAGE_MANAGER_ERROR_NONE)
		return package_manager_error(ret, __FUNCTION__, NULL);

	r = foreach_pkg_context->callback(package_info, foreach_pkg_context->user_data);

	package_info_destroy_handle(package_info);

	return (r == true) ? PMINFO_R_OK : PMINFO_R_ERROR;
}

int package_info_foreach_package_info(package_manager_package_info_cb callback, void *user_data)
{
	foreach_pkg_context_s foreach_pkg_context = {
		.callback = callback,
		.user_data = user_data,
	};
	int ret = 0;

	if (callback == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	ret = pkgmgrinfo_pkginfo_get_list(package_info_foreach_package_info_cb, &foreach_pkg_context);

	if (ret < 0)
		return PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE;

	return PACKAGE_MANAGER_ERROR_NONE;
}

int package_info_filter_foreach_package_info(pkgmgrinfo_pkginfo_filter_h handle, package_manager_package_info_cb callback, void *user_data)
{
	foreach_pkg_context_s foreach_pkg_context = {
		.callback = callback,
		.user_data = user_data,
	};
	int ret;

	if ((handle == NULL) || (callback == NULL))
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	ret = pkgmgrinfo_pkginfo_filter_foreach_pkginfo(handle, package_info_foreach_package_info_cb, &foreach_pkg_context);

	if (ret < 0)
		return PACKAGE_MANAGER_ERROR_IO_ERROR;

	return PACKAGE_MANAGER_ERROR_NONE;
}

