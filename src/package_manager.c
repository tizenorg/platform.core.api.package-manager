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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>

#include <package-manager.h>
#include <pkgmgr_installer.h>
#include <pkgmgr-info.h>
#include <tzplatform_config.h>

#include "package_manager.h"
#include "package_manager_internal.h"

static GHashTable *__cb_table = NULL;

#define GLOBAL_USER tzplatform_getuid(TZ_SYS_GLOBALAPP_USER)
typedef struct _event_info {
	int req_id;
	package_manager_event_type_e event_type;
	package_manager_event_state_e event_state;
	struct _event_info *next;
} event_info;

struct package_manager_s {
	int handle_id;
	client_type ctype;
	pkgmgr_client *pc;
	pkgmgr_mode mode;
	event_info *head;
	package_manager_event_cb event_cb;
	void *user_data;
};

struct package_manager_request_s {
	int handle_id;
	client_type ctype;
	pkgmgr_client *pc;
	const char *pkg_type;
	const char *pkg_path;
	const char *pkg_name;
	const char *tep_path;
	pkgmgr_mode mode;
	event_info *head;
	package_manager_request_event_cb event_cb;
	bool tep_move;
	void *user_data;
};

typedef struct package_size_info {
	long long data_size;
	long long cache_size;
	long long app_size;

	long long external_data_size;
	long long external_cache_size;
	long long external_app_size;
} package_size_info_t;

struct package_manager_filter_s {
	pkgmgrinfo_pkginfo_filter_h pkgmgrinfo_pkginfo_filter;
};

static int package_manager_request_new_id()
{
	static int request_handle_id = 0;
	return request_handle_id++;
}

static int package_manager_new_id()
{
	static int manager_handle_id = 0;
	return manager_handle_id++;
}

API int package_manager_request_create(package_manager_request_h *request)
{
	struct package_manager_request_s *package_manager_request;

	if (request == NULL) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	package_manager_request =
	    calloc(1, sizeof(struct package_manager_request_s));
	if (package_manager_request == NULL) {
		return
		    package_manager_error(PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY,
					  __FUNCTION__,
					  "failed to create a package_manager handle");
	}

	package_manager_request->ctype = PC_REQUEST;
	package_manager_request->pc = pkgmgr_client_new(PC_REQUEST);
	if (package_manager_request->pc == NULL) {
		free(package_manager_request);
		return
		    package_manager_error(PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY,
					  __FUNCTION__,
					  "failed to create a package_manager client");
	}

	package_manager_request->handle_id = package_manager_request_new_id();

	*request = package_manager_request;

	return PACKAGE_MANAGER_ERROR_NONE;
}

static int package_manager_client_validate_handle(package_manager_request_h
						 request)
{
	if (request == NULL || request->pc == NULL)
		return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_request_destroy(package_manager_request_h request)
{
	if (package_manager_client_validate_handle(request)) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	pkgmgr_client_free(request->pc);
	request->pc = NULL;
	free(request);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_request_set_event_cb(package_manager_request_h request,
					 package_manager_request_event_cb
					 callback, void *user_data)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	if (package_manager_client_validate_handle(request)) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	request->event_cb = callback;
	request->user_data = user_data;

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_request_unset_event_cb(package_manager_request_h request)
{
	if (package_manager_client_validate_handle(request))
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	request->event_cb = NULL;
	request->user_data = NULL;

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_request_set_type(package_manager_request_h request,
				     const char *pkg_type)
{
	if (package_manager_client_validate_handle(request)) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	request->pkg_type = strdup(pkg_type);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_request_set_mode(package_manager_request_h request,
				     package_manager_request_mode_e mode)
{
	if (package_manager_client_validate_handle(request)) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	if (mode == PACKAGE_MANAGER_REQUEST_MODE_QUIET)
		request->mode = PM_QUIET;
	else
		request->mode = PM_DEFAULT;

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_request_set_tep(package_manager_request_h request,
				     const char *tep_path)
{
	int retval = 0;

	if (package_manager_client_validate_handle(request) || tep_path == NULL) {
		return
			package_manager_error
			(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
			 NULL);
	}

	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_ADMIN);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	if (request->tep_path)
		free((void *)request->tep_path);

	request->tep_path = strdup(tep_path);
	request->tep_move = true;

	if (request->tep_path == NULL)
		return PACKAGE_MANAGER_ERROR_SYSTEM_ERROR;

	return PACKAGE_MANAGER_ERROR_NONE;
}

static int package_manager_get_event_type(const char *key,
					  package_manager_event_type_e *
					  event_type)
{
	if (key == NULL)
		return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;

	if (strcasecmp(key, PKGMGR_INSTALLER_INSTALL_EVENT_STR) == 0)
		*event_type = PACKAGE_MANAGER_EVENT_TYPE_INSTALL;
	else if (strcasecmp(key, PKGMGR_INSTALLER_UNINSTALL_EVENT_STR) == 0)
		*event_type = PACKAGE_MANAGER_EVENT_TYPE_UNINSTALL;
	else if (strcasecmp(key, PKGMGR_INSTALLER_UPGRADE_EVENT_STR) == 0)
		*event_type = PACKAGE_MANAGER_EVENT_TYPE_UPDATE;
	else
		return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;

	return PACKAGE_MANAGER_ERROR_NONE;
}

static int __add_event_info(event_info **head, int req_id,
			    package_manager_event_type_e event_type,
			    package_manager_event_state_e event_state)
{
	event_info *evt_info;
	event_info *current;
	event_info *prev;

	evt_info = (event_info *) calloc(1, sizeof(event_info));
	if (evt_info == NULL) {
		_LOGD("calloc failed");
		return -1;
	}
	evt_info->req_id = req_id;
	evt_info->event_type = event_type;
	evt_info->next = NULL;

	if (*head == NULL)
		*head = evt_info;
	else {
		current = prev = *head;
		while (current) {
			prev = current;
			current = current->next;
		}

		prev->next = evt_info;
	}

	return 0;
}

static int __find_event_info(event_info **head, int req_id,
			     package_manager_event_type_e *event_type,
			     package_manager_event_state_e *event_state)
{
	event_info *tmp;

	tmp = *head;

	if (tmp == NULL) {
		_LOGE("tmp is NULL");
		return -1;
	}

	_LOGD("tmp->req_id %d, event_type %d", tmp->req_id, event_type);

	while (tmp) {
		if (tmp->req_id == req_id) {
			*event_type = tmp->event_type;
			return 0;
		}
		tmp = tmp->next;
	}
	return -1;
}

static int __update_event_info(event_info **head, int req_id,
			       package_manager_event_type_e event_type,
			       package_manager_event_state_e event_state)
{
	package_manager_event_type_e evt_type;
	package_manager_event_state_e evt_state;
	event_info *tmp;

	if (__find_event_info(head, req_id, &evt_type, &evt_state) != 0)
		__add_event_info(head, req_id, event_type, event_state);
	else {
		tmp = *head;

		if (tmp == NULL) {
			_LOGE("tmp is NULL");
			return -1;
		}

		while (tmp) {
			if (tmp->req_id == req_id) {
				tmp->event_type = event_type;
				return 0;
			}
			tmp = tmp->next;
		}
	}

	return -1;
}

/*
static int __remove_event_info(event_info **head request, int req_id)
{
	event_info *current;
	event_info *tmp;

	if (* == NULL)
		return -1;

	current = *head;
	while (current) {
		if (current->next) {
			if (current->next->req_id == req_id) {
				tmp = current->next;
				current->next = current->next->next;
				free(tmp);
				return 0;
			}
		}
		tmp = tmp->next;
	}

	return -1;
}
*/

static int request_event_handler(uid_t target_uid, int req_id, const char *pkg_type,
				 const char *pkg_name, const char *key,
				 const char *val, const void *pmsg, void *data)
{
	int ret = -1;
	package_manager_event_type_e event_type = -1;
	package_manager_event_state_e event_state = -1;

	_LOGD("request_event_handler is called");

	package_manager_request_h request = data;

	if (strcasecmp(key, "start") == 0) {
		ret = package_manager_get_event_type(val, &event_type);
		if (ret != PACKAGE_MANAGER_ERROR_NONE)
			return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;

		__add_event_info(&(request->head), req_id, event_type,
				 PACKAGE_MANAGER_EVENT_STATE_STARTED);

		if (request->event_cb)
			request->event_cb(req_id, pkg_type, pkg_name,
					  event_type,
					  PACKAGE_MANAGER_EVENT_STATE_STARTED,
					  0, PACKAGE_MANAGER_ERROR_NONE, request->user_data);

	} else if (strcasecmp(key, "install_percent") == 0
		   || strcasecmp(key, "progress_percent") == 0) {
		if (__find_event_info
		    (&(request->head), req_id, &event_type,
		     &event_state) == 0) {
			__update_event_info(&(request->head), req_id,
					    event_type,
					    PACKAGE_MANAGER_EVENT_STATE_PROCESSING);
			if (request->event_cb)
				request->event_cb(req_id, pkg_type, pkg_name,
						  event_type,
						  PACKAGE_MANAGER_EVENT_STATE_PROCESSING,
						  atoi(val),
						  PACKAGE_MANAGER_ERROR_NONE,
						  request->user_data);
		}

	} else if (strcasecmp(key, "error") == 0) {
		if (strcasecmp(key, "0") != 0) {
			if (__find_event_info
			    (&(request->head), req_id, &event_type,
			     &event_state) == 0) {
				__update_event_info(&(request->head), req_id,
						    event_type,
						    PACKAGE_MANAGER_EVENT_STATE_FAILED);
			}

			if (request->event_cb)
				request->event_cb(req_id, pkg_type,
						  pkg_name, event_type,
						  PACKAGE_MANAGER_EVENT_STATE_FAILED,
						  0,
						  PACKAGE_MANAGER_ERROR_NONE,
						  request->user_data);

		}
	} else if (strcasecmp(key, "end") == 0) {
		if (__find_event_info
		    (&(request->head), req_id, &event_type,
		     &event_state) == 0) {
			if (event_state != PACKAGE_MANAGER_EVENT_STATE_FAILED) {
				if (request->event_cb)
					request->event_cb(req_id, pkg_type,
							  pkg_name, event_type,
							  PACKAGE_MANAGER_EVENT_STATE_COMPLETED,
							  100,
							  PACKAGE_MANAGER_ERROR_NONE,
							  request->user_data);
			}
		} else {
			if (strcasecmp(key, "ok") != 0)
				if (request->event_cb)
					request->event_cb(req_id, pkg_type,
							  pkg_name, event_type,
							  PACKAGE_MANAGER_EVENT_STATE_FAILED,
							  0,
							  PACKAGE_MANAGER_ERROR_NONE,
							  request->user_data);
		}
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_request_install(package_manager_request_h request,
				    const char *path, int *id)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_ADMIN);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	if (package_manager_client_validate_handle(request))
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	if (path == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	int request_id = 0;
	request->pkg_path = path;
	uid_t uid = getuid();
	if (uid != GLOBAL_USER)
		request_id = pkgmgr_client_usr_install(request->pc, request->pkg_type, NULL,
							request->pkg_path, NULL,
							request->mode, request_event_handler,
							request,
							uid);
	else
		request_id = pkgmgr_client_install(request->pc, request->pkg_type, NULL,
							request->pkg_path, NULL,
							request->mode, request_event_handler,
							request);

	if (request_id == PKGMGR_R_EINVAL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
	else if (request_id == PKGMGR_R_ENOPKG)
		return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	else if (request_id == PKGMGR_R_ENOMEM)
		return package_manager_error(PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY, __FUNCTION__, NULL);
	else if (request_id == PKGMGR_R_EIO)
		return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);
	else if (request_id == PKGMGR_R_EPRIV)
		return package_manager_error(PACKAGE_MANAGER_ERROR_PERMISSION_DENIED, __FUNCTION__, NULL);
	else if (request_id == PKGMGR_R_ESYSTEM || request_id == PKGMGR_R_ECOMM || request_id == PKGMGR_R_ERROR)
		return package_manager_error(PACKAGE_MANAGER_ERROR_SYSTEM_ERROR, __FUNCTION__, NULL);

	*id = request_id;

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_request_uninstall(package_manager_request_h request,
				      const char *name, int *id)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_ADMIN);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	if (package_manager_client_validate_handle(request))
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	if (name == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	int request_id = 0;
	request->pkg_name = name;
	uid_t uid = getuid();
	if (uid != GLOBAL_USER)
		request_id = pkgmgr_client_usr_uninstall(request->pc, request->pkg_type,
								request->pkg_name, request->mode,
								request_event_handler, request, uid);
	else
		request_id = pkgmgr_client_uninstall(request->pc, request->pkg_type,
								request->pkg_name, request->mode,
								request_event_handler, request);

	if (request_id == PKGMGR_R_EINVAL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
	else if (request_id == PKGMGR_R_ENOPKG)
		return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	else if (request_id == PKGMGR_R_ENOMEM)
		return package_manager_error(PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY, __FUNCTION__, NULL);
	else if (request_id == PKGMGR_R_EIO)
		return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);
	else if (request_id == PKGMGR_R_EPRIV)
		return package_manager_error(PACKAGE_MANAGER_ERROR_PERMISSION_DENIED, __FUNCTION__, NULL);
	else if (request_id == PKGMGR_R_ESYSTEM || request_id == PKGMGR_R_ECOMM || request_id == PKGMGR_R_ERROR)
		return package_manager_error(PACKAGE_MANAGER_ERROR_SYSTEM_ERROR, __FUNCTION__, NULL);

	*id = request_id;

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_request_move(package_manager_request_h request,
				    const char *name, package_manager_move_type_e move_type)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_ADMIN);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	if (package_manager_client_validate_handle(request))
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	if (name == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	int ret = 0;
	request->pkg_name = name;
	uid_t uid = getuid();
	if (uid != GLOBAL_USER)
		ret = pkgmgr_client_usr_request_service(PM_REQUEST_MOVE, move_type,
				request->pc, request->pkg_type, request->pkg_name,
				uid, NULL, request_event_handler, NULL);
	else
		ret = pkgmgr_client_request_service(PM_REQUEST_MOVE, move_type,
				request->pc, request->pkg_type, request->pkg_name,
				NULL, request_event_handler, NULL);

	if (ret == PKGMGR_R_EINVAL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
	else if (ret == PKGMGR_R_ENOPKG)
		return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	else if (ret == PKGMGR_R_ENOMEM)
		return package_manager_error(PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY, __FUNCTION__, NULL);
	else if (ret == PKGMGR_R_EIO)
		return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);
	else if (ret == PKGMGR_R_EPRIV)
		return package_manager_error(PACKAGE_MANAGER_ERROR_PERMISSION_DENIED, __FUNCTION__, NULL);
	else if (ret == PKGMGR_R_ESYSTEM || ret == PKGMGR_R_ECOMM || ret == PKGMGR_R_ERROR)
		return package_manager_error(PACKAGE_MANAGER_ERROR_SYSTEM_ERROR, __FUNCTION__, NULL);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_create(package_manager_h *manager)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	struct package_manager_s *package_manager = NULL;

	if (manager == NULL) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	package_manager = calloc(1, sizeof(struct package_manager_s));
	if (package_manager == NULL) {
		return
		    package_manager_error(PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY,
					  __FUNCTION__,
					  "failed to create a package_manager handle");
	}

	package_manager->ctype = PC_LISTENING;
	package_manager->pc = pkgmgr_client_new(PC_LISTENING);
	if (package_manager->pc == NULL) {
		free(package_manager);
		return
		    package_manager_error(PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY,
					  __FUNCTION__,
					  "failed to create a package_manager client");
	}

	package_manager->handle_id = package_manager_new_id();

	*manager = package_manager;

	return PACKAGE_MANAGER_ERROR_NONE;
}

static int package_manager_validate_handle(package_manager_h manager)
{
	if (manager == NULL || manager->pc == NULL)
		return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_destroy(package_manager_h manager)
{
	if (package_manager_validate_handle(manager)) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	pkgmgr_client_free(manager->pc);
	manager->pc = NULL;
	free(manager);

	return PACKAGE_MANAGER_ERROR_NONE;
}

static int __add_event(event_info **head, int req_id,
			    package_manager_event_type_e event_type,
			    package_manager_event_state_e event_state)
{
	event_info *evt_info;

	evt_info = (event_info *) calloc(1, sizeof(event_info));
	if (evt_info == NULL) {
		_LOGD("calloc failed");
		return -1;
	}
	evt_info->req_id = req_id;
	evt_info->event_type = event_type;
	evt_info->next = NULL;

	*head = evt_info;

	return 0;
}

static int __find_event(event_info **head, int req_id,
			     package_manager_event_type_e *event_type,
			     package_manager_event_state_e *event_state)
{
	event_info *tmp;

	tmp = *head;

	if (tmp == NULL) {
		_LOGE("tmp is NULL");
		return -1;
	}

	*event_type = tmp->event_type;
	return 0;
}

static int __update_event(event_info **head, int req_id,
			       package_manager_event_type_e event_type,
			       package_manager_event_state_e event_state)
{
	package_manager_event_type_e evt_type;
	package_manager_event_state_e evt_state;
	event_info *tmp;

	if (__find_event_info(head, req_id, &evt_type, &evt_state) != 0)
		__add_event_info(head, req_id, event_type, event_state);
	else {
		tmp = *head;

		if (tmp == NULL) {
			_LOGE("tmp is NULL");
			return -1;
		}

		tmp->event_type = event_type;
		return 0;
	}

	return -1;
}

/* App Event Listening Policy:
 * +----------------+------------+---------------+------------------+
 * |Listener \ Type |Global Event|My User's Event|Other user's Event|
 * +----------------+------------+---------------+------------------+
 * |User Process App|   Grant    |     Grant     |      Deny        |
 * +----------------+------------+---------------+------------------+
 * |Platform module |   Grant    |     Grant     |      Grant       |
 * +----------------+------------+---------------+------------------+
 * UID assignment policy:
 * https://wiki.tizen.org/wiki/Security/User_and_group_ID_assignment_policy
 */
#define REGULAR_USER 5000
static int __validate_event_signal(uid_t target_uid)
{
	uid_t self = getuid();

	if (self == target_uid)
		return 0;

	if (self < REGULAR_USER)
		return 0;

	return -1;
}

static int global_event_handler(uid_t target_uid, int req_id, const char *pkg_type,
				const char *pkg_name, const char *key,
				const char *val, const void *pmsg, void *data)
{
	int ret = -1;
	package_manager_event_type_e event_type = -1;
	package_manager_event_state_e event_state = -1;
	uid_t uid = target_uid;

	_LOGD("global_event_handler is called");

	package_manager_h manager = data;

	if (target_uid == GLOBAL_USER)
		uid = getuid();

	if (__validate_event_signal(uid))
		return PACKAGE_MANAGER_ERROR_NONE;

	if (strcasecmp(key, "start") == 0) {
		ret = package_manager_get_event_type(val, &event_type);
		if (ret != PACKAGE_MANAGER_ERROR_NONE)
			return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;

		__add_event(&(manager->head), req_id, event_type,
				 PACKAGE_MANAGER_EVENT_STATE_STARTED);

		if (manager->event_cb && getuid() == uid)
			manager->event_cb(pkg_type, pkg_name,
					  event_type,
					  PACKAGE_MANAGER_EVENT_STATE_STARTED,
					  0, PACKAGE_MANAGER_ERROR_NONE, manager->user_data);
	} else if (strcasecmp(key, "install_percent") == 0
		   || strcasecmp(key, "progress_percent") == 0) {
		if (__find_event
		    (&(manager->head), req_id, &event_type,
		     &event_state) == 0) {
			__update_event(&(manager->head), req_id,
					    event_type,
					    PACKAGE_MANAGER_EVENT_STATE_PROCESSING);
			if (manager->event_cb && getuid() == uid)
				manager->event_cb(pkg_type, pkg_name,
						  event_type,
						  PACKAGE_MANAGER_EVENT_STATE_PROCESSING,
						  atoi(val),
						  PACKAGE_MANAGER_ERROR_NONE,
						  manager->user_data);
		}

	} else if (strcasecmp(key, "error") == 0) {
		if (strcasecmp(key, "0") != 0) {
			if (__find_event
			    (&(manager->head), req_id, &event_type,
			     &event_state) == 0) {
				__update_event(&(manager->head), req_id,
						    event_type,
						    PACKAGE_MANAGER_EVENT_STATE_FAILED);
			}

			if (manager->event_cb && getuid() == uid)
				manager->event_cb(pkg_type,
						  pkg_name, event_type,
						  PACKAGE_MANAGER_EVENT_STATE_FAILED,
						  0,
						  PACKAGE_MANAGER_ERROR_NONE,
						  manager->user_data);
		}
	} else if (strcasecmp(key, "end") == 0) {
		if (__find_event
		    (&(manager->head), req_id, &event_type,
		     &event_state) == 0) {
			if (event_state != PACKAGE_MANAGER_EVENT_STATE_FAILED) {
				if (manager->event_cb && getuid() == uid)
					manager->event_cb(pkg_type,
							  pkg_name, event_type,
							  PACKAGE_MANAGER_EVENT_STATE_COMPLETED,
							  100,
							  PACKAGE_MANAGER_ERROR_NONE,
							  manager->user_data);
			}
		} else {
			if (strcasecmp(key, "ok") != 0) {
				if (manager->event_cb && getuid() == uid)
					manager->event_cb(pkg_type,
							  pkg_name, event_type,
							  PACKAGE_MANAGER_EVENT_STATE_FAILED,
							  0,
							  PACKAGE_MANAGER_ERROR_NONE,
							  manager->user_data);
			}
		}
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_set_event_status(package_manager_h manager, int status_type)
{
	int retval;

	if (manager == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	retval = pkgmgrinfo_client_set_status_type(manager->pc, status_type);

	if (retval < 0)
		return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_set_event_cb(package_manager_h manager,
				 package_manager_event_cb callback,
				 void *user_data)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	if (package_manager_validate_handle(manager)) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	manager->event_cb = callback;
	manager->user_data = user_data;

	pkgmgr_client_listen_status(manager->pc, global_event_handler, manager);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_unset_event_cb(package_manager_h manager)
{
	if (manager == NULL) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	int retval;
	manager->event_cb = NULL;
	manager->user_data = NULL;

	retval = pkgmgr_client_remove_listen_status(manager->pc);
	if (retval == PKGMGR_R_EINVAL)
		return
			 package_manager_error
			 (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
			  NULL);
	else if (retval == PKGMGR_R_ERROR)
		return
			 package_manager_error
			 (PACKAGE_MANAGER_ERROR_SYSTEM_ERROR, __FUNCTION__,
			  NULL);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_get_package_id_by_app_id(const char *app_id, char **package_id)
{
	pkgmgrinfo_appinfo_h pkgmgrinfo_appinfo;
	int retval;
	char *pkg_id = NULL;
	char *pkg_id_dup = NULL;

	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	if (app_id == NULL || package_id == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	uid_t uid = getuid();
	if (uid != GLOBAL_USER) {
		if (pkgmgrinfo_appinfo_get_usr_appinfo(app_id, uid, &pkgmgrinfo_appinfo) != PMINFO_R_OK)
			return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	} else {
		if (pkgmgrinfo_appinfo_get_appinfo(app_id, &pkgmgrinfo_appinfo) != PMINFO_R_OK)
			return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	}
	retval = pkgmgrinfo_appinfo_get_pkgname(pkgmgrinfo_appinfo, &pkg_id);
	if (retval != PMINFO_R_OK)
		return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);

	pkg_id_dup = strdup(pkg_id);
	if (pkg_id_dup == NULL) {
		pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
		return package_manager_error(PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY, __FUNCTION__, NULL);
	}

	*package_id = pkg_id_dup;

	pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_get_package_info(const char *package_id, package_info_h *package_info)
{
	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	retval = package_info_get_package_info(package_id, package_info);

	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return package_manager_error(retval, __FUNCTION__, NULL);
	else
		return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_foreach_package_info(package_manager_package_info_cb callback,
					void *user_data)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	retval = package_info_foreach_package_info(callback, user_data);

	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return package_manager_error(retval, __FUNCTION__, NULL);
	else
		return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_compare_package_cert_info(const char *lhs_package_id, const char *rhs_package_id, package_manager_compare_result_type_e *compare_result)
{
	pkgmgrinfo_cert_compare_result_type_e result;
	uid_t uid = getuid();

	if (lhs_package_id == NULL || rhs_package_id == NULL || compare_result == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
	if (uid != GLOBAL_USER) {
		if (pkgmgrinfo_pkginfo_compare_usr_pkg_cert_info(lhs_package_id, rhs_package_id, uid, &result) != PKGMGR_R_OK)
			return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);
	} else {
		if (pkgmgrinfo_pkginfo_compare_pkg_cert_info(lhs_package_id, rhs_package_id, &result) != PKGMGR_R_OK)
			return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);
	}
	*compare_result = (package_manager_compare_result_type_e)result;

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_compare_app_cert_info(const char *lhs_app_id, const char *rhs_app_id, package_manager_compare_result_type_e *compare_result)
{
	pkgmgrinfo_cert_compare_result_type_e result;
	uid_t uid = getuid();

	if (lhs_app_id == NULL || rhs_app_id == NULL || compare_result == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
	if (uid != GLOBAL_USER) {
		if (pkgmgrinfo_pkginfo_compare_usr_app_cert_info(lhs_app_id, rhs_app_id, uid, &result) != PKGMGR_R_OK)
			return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);
	} else {
		if (pkgmgrinfo_pkginfo_compare_app_cert_info(lhs_app_id, rhs_app_id, &result) != PKGMGR_R_OK)
			return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);
	}

	*compare_result = (package_manager_compare_result_type_e)result;

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_is_preload_package_by_app_id(const char *app_id, bool *preload)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	pkgmgrinfo_appinfo_h pkgmgrinfo_appinfo = NULL;
	pkgmgrinfo_pkginfo_h pkgmgrinfo_pkginfo = NULL;

	char *pkg_id = NULL;
	bool is_preload = 0;
	uid_t uid = getuid();
	if (uid != GLOBAL_USER) {
		if (pkgmgrinfo_appinfo_get_usr_appinfo(app_id, uid, &pkgmgrinfo_appinfo) != PMINFO_R_OK)
			return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	} else {
		if (pkgmgrinfo_appinfo_get_appinfo(app_id, &pkgmgrinfo_appinfo) != PMINFO_R_OK)
			return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	}

	retval = pkgmgrinfo_appinfo_get_pkgname(pkgmgrinfo_appinfo, &pkg_id);
	if (retval != PMINFO_R_OK) {
		pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
		return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	}
	if (uid != GLOBAL_USER) {
		if (pkgmgrinfo_pkginfo_get_usr_pkginfo(pkg_id, uid, &pkgmgrinfo_pkginfo) != PMINFO_R_OK) {
			pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
			pkgmgrinfo_pkginfo_destroy_pkginfo(pkgmgrinfo_pkginfo);
			return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
		}
	} else {
		if (pkgmgrinfo_pkginfo_get_pkginfo(pkg_id, &pkgmgrinfo_pkginfo) != PMINFO_R_OK) {
			pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
			pkgmgrinfo_pkginfo_destroy_pkginfo(pkgmgrinfo_pkginfo);
			return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
		}
	}
	if (pkgmgrinfo_pkginfo_is_preload(pkgmgrinfo_pkginfo, &is_preload) != PMINFO_R_OK) {
		pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
		pkgmgrinfo_pkginfo_destroy_pkginfo(pkgmgrinfo_pkginfo);
		return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	}

	if (is_preload)
		*preload = 1;
	else
		*preload = 0;

	pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
	pkgmgrinfo_pkginfo_destroy_pkginfo(pkgmgrinfo_pkginfo);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_get_permission_type(const char *app_id, package_manager_permission_type_e *permission_type)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	pkgmgrinfo_appinfo_h pkgmgrinfo_appinfo = NULL;
	pkgmgrinfo_permission_type permission = 0;
	uid_t uid = getuid();
	if (uid != GLOBAL_USER) {
		if (pkgmgrinfo_appinfo_get_usr_appinfo(app_id, uid, &pkgmgrinfo_appinfo) != PMINFO_R_OK)
			return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	} else {
		if (pkgmgrinfo_appinfo_get_appinfo(app_id, &pkgmgrinfo_appinfo) != PMINFO_R_OK)
			return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	}
	retval = pkgmgrinfo_appinfo_get_permission_type(pkgmgrinfo_appinfo, &permission);
	if (retval != PMINFO_R_OK)
		return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);

	if (permission == PMINFO_PERMISSION_NORMAL)
		*permission_type = PACKAGE_MANAGER_PERMISSION_NORMAL;
	else if (permission == PMINFO_PERMISSION_SIGNATURE)
		*permission_type = PACKAGE_MANAGER_PERMISSION_SIGNATURE;
	else if (permission == PMINFO_PERMISSION_PRIVILEGE)
		*permission_type = PACKAGE_MANAGER_PERMISSION_PRIVILEGE;
	else
		*permission_type = PACKAGE_MANAGER_PERMISSION_NORMAL;

	pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo);
	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_clear_cache_dir(const char *package_id)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_CACHE);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	int res = pkgmgr_client_usr_clear_cache_dir(package_id, getuid());
	if (res == PKGMGR_R_EINVAL) {
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
	} else if (res == PKGMGR_R_ENOPKG) {
		return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	} else if (res == PKGMGR_R_ENOMEM) {
		return package_manager_error(PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY, __FUNCTION__, NULL);
	} else if (res == PKGMGR_R_EIO) {
		return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);
	} else if (res == PKGMGR_R_EPRIV) {
		return package_manager_error(PACKAGE_MANAGER_ERROR_PERMISSION_DENIED, __FUNCTION__, NULL);
	} else if (res == PKGMGR_R_ESYSTEM || res == PKGMGR_R_ECOMM || res == PKGMGR_R_ERROR) {
		return package_manager_error(PACKAGE_MANAGER_ERROR_SYSTEM_ERROR, __FUNCTION__, NULL);
	} else if (res != PKGMGR_R_OK) {
		_LOGE("Unexpected error");
		return package_manager_error(PACKAGE_MANAGER_ERROR_SYSTEM_ERROR, __FUNCTION__, NULL);
	}

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_clear_all_cache_dir(void)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_ADMIN);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	return package_manager_clear_cache_dir(PKG_CLEAR_ALL_CACHE);
}

static void __free_client(gpointer data)
{
	pkgmgr_client *pc = (pkgmgr_client *)data;
	pkgmgr_client_free(pc);
}

static void __initialize_cb_table(void)
{
	__cb_table = g_hash_table_new_full(g_int_hash, g_int_equal, __free_client, NULL);
}

static void __result_cb(pkgmgr_client *pc, const char *pkgid, const pkg_size_info_t *result, void *user_data)
{
	package_manager_size_info_receive_cb callback = g_hash_table_lookup(__cb_table, pc);
	if (callback == NULL) {
		_LOGE("callback is null.");
		g_hash_table_remove(__cb_table, pc);
		return;
	}

	package_size_info_t size_info;
	size_info.data_size  = result->data_size;
	size_info.cache_size = result->cache_size;
	size_info.app_size   = result->app_size;
	size_info.external_data_size  = result->ext_data_size;
	size_info.external_cache_size = result->ext_cache_size;
	size_info.external_app_size   = result->ext_app_size;

	callback(pkgid, (package_size_info_h)&size_info, user_data);

	g_hash_table_remove(__cb_table, pc);
}

static void __total_result_cb(pkgmgr_client *pc, const pkg_size_info_t *result, void *user_data)
{
	package_manager_total_size_info_receive_cb callback = g_hash_table_lookup(__cb_table, pc);
	if (callback == NULL) {
		_LOGE("callback is null.");
		g_hash_table_remove(__cb_table, pc);
		return;
	}

	package_size_info_t size_info;
	size_info.data_size  = result->data_size;
	size_info.cache_size = result->cache_size;
	size_info.app_size   = result->app_size;
	size_info.external_data_size  = result->ext_data_size;
	size_info.external_cache_size = result->ext_cache_size;
	size_info.external_app_size   = result->ext_app_size;

	callback((package_size_info_h)&size_info, user_data);

	g_hash_table_remove(__cb_table, pc);
}

static int _get_pkg_size_info(const char *package_id, void *callback, void *user_data)
{
	if (package_id == NULL || callback == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	if (__cb_table == NULL)
		__initialize_cb_table();

	pkgmgr_client *pc = pkgmgr_client_new(PC_REQUEST);
	if (pc == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_SYSTEM_ERROR, __FUNCTION__, NULL);

	int res = 0;
	if (strcmp(package_id, PKG_SIZE_INFO_TOTAL) != 0)
		res = pkgmgr_client_usr_get_package_size_info(pc, package_id, __result_cb, user_data, getuid());
	else
		res = pkgmgr_client_usr_get_total_package_size_info(pc, __total_result_cb, user_data, getuid());

	if (res == PKGMGR_R_EINVAL) {
		pkgmgr_client_free(pc);
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);
	} else if (res == PKGMGR_R_ENOPKG) {
		pkgmgr_client_free(pc);
		return package_manager_error(PACKAGE_MANAGER_ERROR_NO_SUCH_PACKAGE, __FUNCTION__, NULL);
	} else if (res == PKGMGR_R_ENOMEM) {
		pkgmgr_client_free(pc);
		return package_manager_error(PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY, __FUNCTION__, NULL);
	} else if (res == PKGMGR_R_EIO) {
		pkgmgr_client_free(pc);
		return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);
	} else if (res == PKGMGR_R_EPRIV) {
		pkgmgr_client_free(pc);
		return package_manager_error(PACKAGE_MANAGER_ERROR_PERMISSION_DENIED, __FUNCTION__, NULL);
	} else if (res == PKGMGR_R_ESYSTEM || res == PKGMGR_R_ECOMM || res == PKGMGR_R_ERROR) {
		pkgmgr_client_free(pc);
		return package_manager_error(PACKAGE_MANAGER_ERROR_SYSTEM_ERROR, __FUNCTION__, NULL);
	} else if (res != PKGMGR_R_OK) {
		_LOGE("Unexpected error");
		pkgmgr_client_free(pc);
		return package_manager_error(PACKAGE_MANAGER_ERROR_SYSTEM_ERROR, __FUNCTION__, NULL);
	}

	g_hash_table_insert(__cb_table, pc, callback);

	_LOGD("Successful");
	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_get_package_size_info(const char *package_id, package_manager_size_info_receive_cb callback, void *user_data)
{
	return _get_pkg_size_info(package_id, (void *)callback, user_data);
}

API int package_manager_get_total_package_size_info(package_manager_total_size_info_receive_cb callback, void *user_data)
{
	return _get_pkg_size_info(PKG_SIZE_INFO_TOTAL, (void *)callback, user_data);
}

API int package_manager_filter_create(package_manager_filter_h *handle)
{
	int retval;
	pkgmgrinfo_pkginfo_filter_h pkgmgr_filter = NULL;

	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	if (handle == NULL) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	retval = pkgmgrinfo_pkginfo_filter_create(&pkgmgr_filter);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);

	*handle = pkgmgr_filter;

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_filter_destroy(package_manager_filter_h handle)
{
	int retval;

	if (handle == NULL) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	retval = pkgmgrinfo_pkginfo_filter_destroy(handle);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_filter_add_bool(package_manager_filter_h handle,
		const char *property, const bool value)
{
	int retval;

	if ((handle == NULL) || (property == NULL)) {
		return
		    package_manager_error
		    (PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__,
		     NULL);
	}

	retval = pkgmgrinfo_pkginfo_filter_add_bool(handle, property, value);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_filter_count(package_manager_filter_h handle, int *count)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	if ((handle == NULL) || (count == NULL))
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	retval = pkgmgrinfo_pkginfo_filter_count(handle, count);
	if (retval < 0)
		return package_manager_error(PACKAGE_MANAGER_ERROR_IO_ERROR, __FUNCTION__, NULL);

	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_manager_filter_foreach_package_info(package_manager_filter_h handle,
		package_manager_package_info_cb callback, void *user_data)
{

	int retval;
	retval = check_privilege(PRIVILEGE_PACKAGE_MANAGER_INFO);
	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return retval;

	retval = package_info_filter_foreach_package_info(handle, callback, user_data);

	if (retval != PACKAGE_MANAGER_ERROR_NONE)
		return package_manager_error(retval, __FUNCTION__, NULL);
	else
		return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_size_info_get_data_size(package_size_info_h handle, long long *data_size)
{
	if (handle == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	package_size_info_t *size_info = (package_size_info_t *)handle;

	*data_size = (long long)size_info->data_size;
	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_size_info_get_cache_size(package_size_info_h handle, long long *cache_size)
{
	if (handle == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	package_size_info_t *size_info = (package_size_info_t *)handle;

	*cache_size = size_info->cache_size;
	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_size_info_get_app_size(package_size_info_h handle, long long *app_size)
{
	if (handle == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	package_size_info_t *size_info = (package_size_info_t *)handle;
	*app_size = size_info->app_size;
	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_size_info_get_external_data_size(package_size_info_h handle, long long *ext_data_size)
{
	if (handle == NULL)
		return PACKAGE_MANAGER_ERROR_INVALID_PARAMETER;

	package_size_info_t *size_info = (package_size_info_t *)handle;
	*ext_data_size = size_info->external_data_size;
	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_size_info_get_external_cache_size(package_size_info_h handle, long long *ext_cache_size)
{
	if (handle == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	package_size_info_t *size_info = (package_size_info_t *)handle;
	*ext_cache_size = size_info->external_cache_size;
	return PACKAGE_MANAGER_ERROR_NONE;
}

API int package_size_info_get_external_app_size(package_size_info_h handle, long long *ext_app_size)
{
	if (handle == NULL)
		return package_manager_error(PACKAGE_MANAGER_ERROR_INVALID_PARAMETER, __FUNCTION__, NULL);

	package_size_info_t *size_info = (package_size_info_t *)handle;
	*ext_app_size = size_info->external_app_size;
	return PACKAGE_MANAGER_ERROR_NONE;
}
