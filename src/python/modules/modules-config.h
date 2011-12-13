/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VISTK_MODULES_PYTHON_CONFIG_H_
#define VISTK_MODULES_PYTHON_CONFIG_H_

#include <vistk/config.h>

#ifndef VISTK_MODULES_PYTHON_EXPORT
#ifdef MAKE_VISTK_MODULES_PYTHON_LIB
/// Export the symbol if building the library.
#define VISTK_MODULES_PYTHON_EXPORT VISTK_EXPORT
#else
/// Import the symbol if including the library.
#define VISTK_MODULES_PYTHON_EXPORT VISTK_IMPORT
#endif // MAKE_VISTK_MODULES_PYTHON_LIB
/// Hide the symbol from the library interface.
#define VISTK_MODULES_PYTHON_NO_EXPORT VISTK_NO_EXPORT
#endif // VISTK_MODULES_PYTHON_EXPORT

#ifndef VISTK_MODULES_PYTHON_EXPORT_DEPRECATED
/// Mark as deprecated.
#define VISTK_MODULES_PYTHON_EXPORT_DEPRECATED VISTK_DEPRECATED VISTK_MODULES_PYTHON_EXPORT
#endif // VISTK_MODULES_PYTHON_EXPORT_DEPRECATED

#endif // VISTK_MODULES_PYTHON_CONFIG_H_
