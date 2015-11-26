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

#include "package_manager.h"
#include "package_manager_internal.h"

API int package_manager_drm_generate_license_request(const char *resp_data,
		char **req_data, char **license_url)
{
	int ret;
	pkgmgr_client *pc;

	if (resp_data == NULL || req_data == NULL || license_url == NULL)
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_INVALID_PARAMETER,
				__FUNCTION__, NULL);

	pc = pkgmgr_client_new(PC_REQUEST);
	if (pc == NULL)
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY,
				__FUNCTION__, NULL);

	ret = pkgmgr_client_generate_license_request(pc, resp_data, req_data,
			license_url);
	pkgmgr_client_free(pc);
	if (ret == PKGMGR_R_EINVAL) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_INVALID_PARAMETER,
				__FUNCTION__, NULL);
	} else if (ret == PKGMGR_R_ENOMEM) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY,
				__FUNCTION__, NULL);
	} else if (ret == PKGMGR_R_EPRIV) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_PERMISSION_DENIED,
				__FUNCTION__, NULL);
	} else if (ret != PKGMGR_R_OK) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_SYSTEM_ERROR,
				__FUNCTION__, NULL);
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_drm_register_license(const char *resp_data)
{
	int ret;
	pkgmgr_client *pc;

	if (resp_data == NULL)
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_INVALID_PARAMETER,
				__FUNCTION__, NULL);


	pc = pkgmgr_client_new(PC_REQUEST);
	if (pc == NULL)
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY,
				__FUNCTION__, NULL);

	ret = pkgmgr_client_register_license(pc, resp_data);
	pkgmgr_client_free(pc);
	if (ret == PKGMGR_R_EINVAL) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_INVALID_PARAMETER,
				__FUNCTION__, NULL);
	} else if (ret == PKGMGR_R_ENOMEM) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY,
				__FUNCTION__, NULL);
	} else if (ret == PKGMGR_R_EPRIV) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_PERMISSION_DENIED,
				__FUNCTION__, NULL);
	} else if (ret != PKGMGR_R_OK) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_SYSTEM_ERROR,
				__FUNCTION__, NULL);
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_drm_decrypt_package(const char *drm_file_path,
		const char *decrypted_file_path)
{
	int ret;
	pkgmgr_client *pc;

	if (drm_file_path == NULL || decrypted_file_path == NULL)
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_INVALID_PARAMETER,
				__FUNCTION__, NULL);

	pc = pkgmgr_client_new(PC_REQUEST);
	if (pc == NULL)
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY,
				__FUNCTION__, NULL);

	ret = pkgmgr_client_decrypt_package(pc, drm_file_path,
			decrypted_file_path);
	pkgmgr_client_free(pc);
	if (ret == PKGMGR_R_EINVAL) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_INVALID_PARAMETER,
				__FUNCTION__, NULL);
	} else if (ret == PKGMGR_R_ENOMEM) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY,
				__FUNCTION__, NULL);
	} else if (ret == PKGMGR_R_EPRIV) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_PERMISSION_DENIED,
				__FUNCTION__, NULL);
	} else if (ret != PKGMGR_R_OK) {
		return package_manager_error(
				PACKAGE_MANAGER_ERROR_SYSTEM_ERROR,
				__FUNCTION__, NULL);
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}
