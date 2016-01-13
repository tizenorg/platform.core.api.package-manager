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


#ifndef __TIZEN_PACKAGE_MANAGER_DOC_H__
#define __TIZEN_PACKAGE_MANAGER_DOC_H__

/**
 * @ingroup CAPI_APPLICATION_FRAMEWORK
 * @defgroup CAPI_PACKAGE_MANAGER_MODULE Package Manager
 * @brief The package manager is one of the core modules of Tizen application framework, and responsible for getting their information.
 *        You can also retrieve information related to the packages that are installed on the device.
 *
 * @section CAPI_PACKAGE_MANAGER_MODULE_HEADER Required Header
 *   \#include <package_manager.h>
 *
 * @section CAPI_PACKAGE_MANAGER_MODULE_OVERVIEW Overview
 * The Package Manager API provides functions to retrieve information related to the packages and also provides event
 * listening function. You can get the status of package using package_manager_set_event_status() and
 * package_manager_set_event_cb(). If you register package_manager_event_cb(), it is invoked when a package is installed,
 * uninstalled or updated.
 */

/**
 * @ingroup CAPI_PACKAGE_MANAGER_MODULE
 * @defgroup CAPI_PACKAGE_INFO_MODULE Package Information
 * @brief Packager Manager infomation API for packaging. It provides functions for getting the package information.
 *
 * @section CAPI_PACKAGE_INFO_MODULE_HEADER Required Header
 *   \#include <package_manager.h>
 *
 * @section CAPI_PACKAGE_INFO_MODULE_OVERVIEW Overview
 * The package manager is used to retrieve detailed information of the installed packages on the device. This information includes package name, label, path to the icon image, version, type, and installed storage.
 * To retrieve package information, you should get the #package_info_h using package_info_create() or package_manager_get_package_info().
 * The package_manager_foreach_package_info() is used to retrieve all the package information of installed packages. And then package_manager_package_info_cb() is invoked. When the callback function is called, you can get the information using the following functions.
 *
 */

/**
 * @ingroup CAPI_PACKAGE_MANAGER_MODULE
 * @defgroup CAPI_PACKAGE_REQUEST_MODULE Package Manager Request
 * @brief Package Manager Request API. It is responsible for installing, uninstalling, and moving packages, and storing their information.
 *
 * @section CAPI_PACKAGE_REQUEST_MODULE_HEADER Required Header
 *   \#include <package_manager.h>
 *
 * @section CAPI_PACKAGE_REQUEST_MODULE_OVERVIEW Overview
 * The Package Manager API provides functions to install and uninstall the package, and also provides event
 * listening function. The package_manager_request_install() method is used to asynchronously install a package.
 * Similarly, the package_manager_request_uninstall() and package_manager_request_move() methods are used to
 * asynchronously uninstall a package. The callback function is called when the progress of the request changes.
 * Use package_manager_request_event_cb() to listen to package installation related events.
 */

#endif /* __TIZEN_PACKAGE_MANAGER_DOC_H__ */


