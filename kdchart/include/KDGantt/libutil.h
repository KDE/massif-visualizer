/****************************************************************************
** Copyright (C) 2001-2013 Klaralvdalens Datakonsult AB.  All rights reserved.
**
** This file is part of the KD Chart library.
**
** Licensees holding valid commercial KD Chart licenses may use this file in
** accordance with the KD Chart Commercial License Agreement provided with
** the Software.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.GPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Contact info@kdab.com if any conditions of this licensing are not
** clear to you.
**
**********************************************************************/

#ifndef __ASCSHARED_UTIL_LIBUTIL_H__
#define __ASCSHARED_UTIL_LIBUTIL_H__

// These two macros can be used to force the loading of static objects in static libraries.
// Under normal circumstances, the linker automatically discards unused library symbols from the final executable,
// which is a problem for static objects with a constructor performing a specific action such as registration.
// Note that the export macro only needs to be included once per .cpp file (even if there are many static objects in the file).
// Parameters :
// - ID : an file-wide identifier, e.g. the filename without the extension.
//        It shouldn't be quoted, no spaces, and contain only alphanumerical characters.
#define KDAB_EXPORT_STATIC_SYMBOLS( ID ) int __init_##ID##_static_symbols() { return 0; }
#define KDAB_IMPORT_STATIC_SYMBOLS( ID ) extern int __init_##ID##_static_symbols(); \
    static int fake_init##ID = __init_##ID##_static_symbols();

#endif // __ASCSHARED_UTIL_LIBUTIL_H__
