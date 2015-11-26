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

#include <glib.h>
#include <gio/gio.h>

#include "package_manager.h"
#include "package_manager_internal.h"

#define PKGMGR_DBUS_SERVICE "org.tizen.pkgmgr"
#define PKGMGR_DBUS_OBJECT_PATH "/org/tizen/pkgmgr"
#define PKGMGR_DBUS_INTERFACE "org.tizen.pkgmgr"
#define PKGMGR_DBUS_METHOD_GENERATE_LICENSE_REQUEST "generate_license_request"
#define PKGMGR_DBUS_METHOD_REGISTER_LICENSE "register_license"
#define PKGMGR_DBUS_METHOD_DECRYPT_PACKAGE "decrypt_package"

#define RETRY_MAX 3
#define RETRY_WAIT_USEC (1000000 / 2) /* 0.5 sec */

static int _send_request_to_server(const char *method, GVariant *params,
		GVariant **result)
{
	int ret = PACKAGE_MANAGER_ERROR_IO_ERROR;
	GError *error = NULL;
	GDBusConnection *conn;
	GDBusProxy *proxy;
	int retry = 0;

	g_variant_ref_sink(params);

	conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (conn == NULL) {
		_LOGE("g_bus_get_sync error: %s", error->message);
		g_error_free(error);
		return PACKAGE_MANAGER_ERROR_IO_ERROR;
	}

	do {
		proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE,
				NULL, PKGMGR_DBUS_SERVICE,
				PKGMGR_DBUS_OBJECT_PATH, PKGMGR_DBUS_INTERFACE,
				NULL, &error);
		if (proxy == NULL) {
			_LOGE("failed to get proxy object, sleep and retry: %s",
					error->message);
			g_error_free(error);
			error = NULL;
			usleep(RETRY_WAIT_USEC);
			retry++;
			continue;
		}

		*result = g_dbus_proxy_call_sync(proxy, method, params,
				G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
		g_object_unref(proxy);
		if (*result) {
			ret = PACKAGE_MANAGER_ERROR_NONE;
			break;
		}

		if (error->code == G_IO_ERROR_PERMISSION_DENIED) {
			g_object_unref(conn);
			return PACKAGE_MANAGER_ERROR_PERMISSION_DENIED;
		}

		_LOGE("failed to send request, sleep and retry: %s",
				error->message);
		g_error_free(error);
		error = NULL;
		usleep(RETRY_WAIT_USEC);
		retry++;
	} while (retry < RETRY_MAX);

	g_variant_unref(params);
	g_object_unref(conn);

	return ret;
}

API int package_manager_drm_generate_license_request(const char *resp_data,
		char **req_data, char **license_url)
{
	int ret;
	GVariant *result;

	if (resp_data == NULL || req_data == NULL || license_url == NULL)
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_INVALID_PARAMETER,
				__FUNCTION__, NULL);

	ret = _send_request_to_server(
			PKGMGR_DBUS_METHOD_GENERATE_LICENSE_REQUEST,
			g_variant_new("(s)", resp_data),
			&result);
	if (ret != PACKAGE_MANAGER_ERROR_NONE)
		return ret;

	g_variant_get(result, "(&s&si)", req_data, license_url, &ret);
	g_variant_unref(result);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_drm_register_license(const char *resp_data)
{
	int ret;
	GVariant *result;

	if (resp_data == NULL)
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_INVALID_PARAMETER,
				__FUNCTION__, NULL);

	ret = _send_request_to_server(
			PKGMGR_DBUS_METHOD_REGISTER_LICENSE,
			g_variant_new("(s)", resp_data),
			&result);
	if (ret != PACKAGE_MANAGER_ERROR_NONE)
		return ret;

	g_variant_get(result, "(i)", &ret);
	g_variant_unref(result);

	if (ret != 0) {
		_LOGE("drm_tizen_register_license failed: %d", ret);
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_drm_decrypt_package(const char *drm_file_path,
		const char *decrypted_file_path)
{
	int ret;
	GVariant *result;

	if (drm_file_path == NULL || decrypted_file_path == NULL)
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_INVALID_PARAMETER,
				__FUNCTION__, NULL);

	ret = _send_request_to_server(
			PKGMGR_DBUS_METHOD_DECRYPT_PACKAGE,
			g_variant_new("(ss)", drm_file_path,
				decrypted_file_path),
			&result);
	if (ret != PACKAGE_MANAGER_ERROR_NONE)
		return ret;

	g_variant_get(result, "(i)", &ret);
	g_variant_unref(result);

	if (ret != 0) {
		_LOGE("drm_tizen_decrypt_package failed: %d", ret);
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}
