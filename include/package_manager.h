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

#ifndef __TIZEN_APPFW_PACKAGE_MANAGER_H
#define __TIZEN_APPFW_PACKAGE_MANAGER_H

#include <tizen.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup CAPI_PACKAGE_MANAGER_MODULE
 * @{
 */

/**
 * @brief Enumerations of error code
 */
typedef enum {
	PACKAGE_MANAGER_ERROR_NONE = TIZEN_ERROR_NONE, /**< Successful */
	PACKAGE_MANAGER_ERROR_INVALID_PARAMETER = TIZEN_ERROR_INVALID_PARAMETER, /**< Invalid parameter */
	PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY = TIZEN_ERROR_OUT_OF_MEMORY, /**< Out of memory */
	PACKAGE_MANAGER_ERROR_IO_ERROR = TIZEN_ERROR_IO_ERROR, /**< Internal I/O error */
} package_manager_error_e;

/**
 * @brief Enumeration of event type
 */
typedef enum {
	PACAKGE_MANAGER_EVENT_TYPE_INSTALL,
	PACAKGE_MANAGER_EVENT_TYPE_UNINSTALL,
	PACAKGE_MANAGER_EVENT_TYPE_UPDATE,
} package_manager_event_type_e;

/**
 * @brief Enumeration of event state
 */
typedef enum {
	PACAKGE_MANAGER_EVENT_STATE_STARTED,
	PACAKGE_MANAGER_EVENT_STATE_PROCESSING,
	PACAKGE_MANAGER_EVENT_STATE_COMPLETED,
	PACAKGE_MANAGER_EVENT_STATE_FAILED,
} package_manager_event_state_e;

/**
 * @brief Enumeration of request mode
 */
typedef enum {
	PACAKGE_MANAGER_REQUEST_MODE_DEFAULT,
	PACAKGE_MANAGER_REQUEST_MODE_QUIET,
} package_manager_request_mode_e;

/**
 * @brief Package manager handle
 */
typedef struct package_manager_s *package_manager_h;

/**
 * @brief Package manager request handle
 */
typedef struct package_manager_request_s *package_manager_request_h;

/**
 * @brief Called when the progress of the request to the package manager changes.
 *
 * @param [in] id The ID of the request to the package manager
 * @param [in] type The type of the package to install, uninstall or update
 * @param [in] package The name of the package to install, uninstall or update
 * @param [in] event_type The type of the request to the package manager
 * @param [in] event_state The current state of the request to the package manager
 * @param [in] progress The progress for the request that is being processed by the package manager \n
 * The range of progress is from 0 to 100.
 * @param [in] error The error code when the package manager failed to process the request
 * @param [in] user_data The user data passed from package_manager_request_set_event_cb()
 * @see package_manager_request_set_event_cb()
 * @see package_manager_request_unset_event_cb()
 */
typedef void (*package_manager_request_event_cb) (
			int id,
			const char *type,
			const char *package,
			package_manager_event_type_e event_type,
			package_manager_event_state_e event_state,
			int progress,
			package_manager_error_e error,
			void *user_data);

/**
 * @brief Creates a request handle to the package manager.
 *
 * @remarks The @a request must be released with package_manager_reqeust_destroy() by you.
 * @param [out] request A request handle to be newly created on success
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #PACKAGE_MANAGER_ERROR_IO_ERROR Internal I/O error
 * @see package_manager_reqeust_destroy()
 */
int package_manager_request_create(package_manager_request_h *request);

/**
 * @brief Destroys the request handle to the package manager.
 *
 * @param [in] request The request handle to the package manager
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see package_manager_request_create()
 */
int package_manager_reqeust_destroy(package_manager_request_h request);

/**
 * @brief Registers a callback function to be invoked when the progress of the request changes.
 *
 * @param [in] request The request handle
 * @param [in] callback The callback function to register
 * @param [in] user_data The user data to be passed to the callback function
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
 * @post package_manager_request_event_cb() will be invoked.
 * @see package_manager_request_event_cb()
 * @see package_manager_request_unset_event_cb()
*/
int package_manager_request_set_event_cb(package_manager_request_h request,
					 package_manager_request_event_cb callback, void *user_data);

/**
 * @brief Unregisters the callback function.
 *
 * @param [in] request The request handle
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see package_manager_request_event_cb()
 * @see package_manager_request_set_event_cb()
*/
int package_manager_request_unset_event_cb(package_manager_request_h request);

/**
 * @brief Sets the type of the package to install, uninstall or update.
 *
 * @param [in] request The request handle
 * @param [in] type The type of the package
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
*/
int package_manager_request_set_type(package_manager_request_h request,
				     const char *type);

/**
 * @brief Sets the mode of the request.
 *
 * @param [in] request The request handle
 * @param [in] mode The mode of the request
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
*/
int package_manager_request_set_mode(package_manager_request_h request,
				     package_manager_request_mode_e mode);

/**
 * @brief Installs the package which is located at the given path.
 *
 * @param [in] request The request handle
 * @param [in] path The absolute path to the package to install
 * @param [out] id The ID of the request to the package manager
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see package_manager_request_uninstall()
*/
int package_manager_request_install(package_manager_request_h request,
				    const char *path, int *id);

/**
 * @brief Uninstalls the package with the given name
 *
 * @param [in] request The request handle
 * @param [in] name The name of the package to uninstall
 * @param [out] id The ID of the request to the package manager
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
*/
int package_manager_request_uninstall(package_manager_request_h request,
				      const char *name, int *id);

/**
 * @brief Called when the package is installed, uninstalled or updated, and the progress of the request to the package manager changes.
 *
 * @param [in] type The type of the package to install, uninstall or update
 * @param [in] package The name of the package to install, uninstall or update
 * @param [in] event_type The type of the request to the package manager
 * @param [in] event_state The current state of the request to the package manager
 * @param [in] progress The progress for the request that is being processed by the package manager \n
 * The range of progress is from 0 to 100.
 * @param [in] error The error code when the package manager failed to process the request
 * @param [in] user_data The user data passed from package_manager_set_event_cb()
 * @see package_manager_set_event_cb()
 * @see package_manager_unset_event_cb()
 */
typedef void (*package_manager_event_cb) (
			const char *type,
			const char *package,
			package_manager_event_type_e event_type,
			package_manager_event_state_e event_state,
			int progress,
			package_manager_error_e error,
			void *user_data);

/**
 * @brief Creates a package manager handle.
 *
 * @remarks The @a manager must be released with package_manager_destroy() by you
 * @param [out] manager A package manager handle to be newly created on success
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #PACKAGE_MANAGER_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #PACKAGE_MANAGER_ERROR_IO_ERROR Internal I/O error
 * @see package_manager_destroy()
 */
int package_manager_create(package_manager_h * manager);

/**
 * @brief Destroys the package manager handle.
 *
 * @param [in] manager The package manager handle
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see package_manager_create()
 */
int package_manager_destroy(package_manager_h manager);

/**
 * @brief Registers a callback function to be invoked when the package is installed, uninstalled or updated.
 *
 * @param [in] manager The package manager handle
 * @param [in] callback The callback function to register
 * @param [in] user_data The user data to be passed to the callback function
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
 * @post package_manager_event_cb() will be invoked.
 * @see package_manager_event_cb()
 * @see package_manager_unset_event_cb()
*/
int package_manager_set_event_cb(package_manager_h manager,
				 package_manager_event_cb callback,
				 void *user_data);

/**
 * @brief Unregisters the callback function.
 *
 * @param [in] manager The package manager handle
 * @return 0 on success, otherwise a negative error value.
 * @retval #PACKAGE_MANAGER_ERROR_NONE Successful
 * @retval #PACKAGE_MANAGER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see package_manager_event_cb()
 * @see package_manager_set_event_cb()
*/
int package_manager_unset_event_cb(package_manager_h manager);


#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_APPFW_PACKAGE_MANAGER_H */
