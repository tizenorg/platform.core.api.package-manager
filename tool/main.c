/*
 * package-manager
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>, Sewook Park <sewook7.park@samsung.com>,
 * Jaeho Lee <jaeho81.lee@samsung.com>, Shobhit Srivastava <shobhit.s@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <glib.h>

#include <package_manager.h>



static void _print_help(const char *cmd)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "[Get packageinfo value]\n");
	fprintf(stderr, "    %s <package>\n", cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "Ex) %s org.tizen.memo\n", cmd);
	fprintf(stderr, "\n");
}


static bool _cert_info_cb(package_info_h handle, package_cert_type_e cert_type,
						const char *cert_value, void *user_data)
{
	fprintf(stderr, "cert_info[%d] \t= [%s]\n", cert_type, cert_value);

	return true;
}

static int _get_packageinfo(const char *package)
{
	package_info_h package_info;
	int ret;
	char *pkg = NULL;
	char *label = NULL;
	char *icon = NULL;
	char *version = NULL;
	char *type = NULL;
	package_info_installed_storage_type_e storage;
	bool system;
	bool removable;
	bool preload;


	ret = package_manager_get_package_info(package, &package_info);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;
	}

	package_info_get_package(package_info, &pkg);
	package_info_get_label(package_info, &label);
	package_info_get_icon(package_info, &icon);
	package_info_get_version(package_info, &version);
	package_info_get_type(package_info, &type);
	package_info_get_installed_storage(package_info, &storage);
	package_info_is_system_package(package_info, &system);
	package_info_is_removable_package(package_info, &removable);
	package_info_is_preload_package(package_info, &preload);

	fprintf(stderr, "pkg \t= [%s]\n", pkg);
	fprintf(stderr, "label \t= [%s]\n", label);
	fprintf(stderr, "icon \t= [%s]\n", icon);
	fprintf(stderr, "version \t= [%s]\n", version);
	fprintf(stderr, "type \t= [%s]\n", type);
	fprintf(stderr, "storage \t= [%d]\n", storage);
	fprintf(stderr, "system \t= [%d]\n", system);
	fprintf(stderr, "removable \t= [%d]\n", removable);
	fprintf(stderr, "preload \t= [%d]\n", preload);

	package_info_foreach_cert_info(package_info, _cert_info_cb, NULL);

	ret = package_info_destroy(package_info);
	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}

static int _get_appinfo(const char *appid)
{
	char *package_id = NULL;
	package_manager_get_package_id_by_app_id(appid, &package_id);

	fprintf(stderr, "app_id \t= [%s]\n", appid);
	fprintf(stderr, "package_id \t= [%s]\n", package_id);

	return PACKAGE_MANAGER_ERROR_NONE;
}

static void _global_event_cb(uid_t target_uid, const char *type,
		const char *package, package_manager_event_type_e event_type,
		package_manager_event_state_e event_state, int progress,
		package_manager_error_e error, void *user_data)
{
	fprintf(stderr, "uid[%u] type[%s] pkgid[%s] event_type[%d] "
			"event_state[%d] progress[%d] error[%d]\n",
			target_uid, type, package, event_type, event_state,
			progress, error);
}

static int _listen_event(void)
{
	int ret;
	package_manager_h manager;

	ret = package_manager_create(&manager);
	if (ret != PACKAGE_MANAGER_ERROR_NONE)
		return ret;

	ret = package_manager_set_global_event_cb(manager, _global_event_cb,
			NULL);
	if (ret != PACKAGE_MANAGER_ERROR_NONE)
		return ret;

	return PACKAGE_MANAGER_ERROR_NONE;
}

int main(int argc, char** argv)
{
	GMainLoop *loop;
	int ret = PACKAGE_MANAGER_ERROR_NONE;

	if (1 == argc) {
		if (_listen_event()) {
			fprintf(stderr, "Register event listener failed\n");
			return EXIT_FAILURE;
		}
	} else if (2 == argc) {
		ret = _get_packageinfo(argv[1]);
	} else if (3 == argc) {
		ret = _get_appinfo(argv[1]);
	}

	else {
		_print_help(argv[0]);
		return EXIT_FAILURE;
	}

	if (ret != PACKAGE_MANAGER_ERROR_NONE) {
		fprintf(stderr, "There are some problems\n");
		return EXIT_FAILURE;
	}

	if (1 == argc) {
		loop = g_main_loop_new(FALSE, NULL);
		g_main_loop_run(loop);
	}

	return EXIT_SUCCESS;
}


