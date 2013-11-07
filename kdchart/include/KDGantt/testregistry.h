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

#ifndef __KDAB__UNITTEST__TESTREGISTRY_H__
#define __KDAB__UNITTEST__TESTREGISTRY_H__

#ifndef KDAB_NO_UNIT_TESTS

#include "../kdganttglobal.h"

#include <string>
#include <map>
#include <vector>
#include <cassert>

namespace KDAB {
namespace UnitTest {

    class Test;
    class TestFactory;

    class KDCHART_EXPORT TestRegistry {
        friend class ::KDAB::UnitTest::TestFactory;
        static TestRegistry * mSelf;
        TestRegistry();
        ~TestRegistry();
    public:
        static TestRegistry * instance();
        static void deleteInstance();

        void registerTestFactory( const TestFactory * tf, const char * group );

        /*! runs all tests in all groups.
           @return the number of failed tests (if any) */
        unsigned int run() const;
        /*! runs only tests in group \a group
            @return the number of failed tests (if any) */
        unsigned int run( const char * group ) const;

    private:
        std::map< std::string, std::vector<const TestFactory*> > mTests;
    };

    class KDCHART_EXPORT Runner {
    public:
        ~Runner();
        unsigned int run( const char * group=0 ) const;
    };

}
}

#endif // KDAB_NO_UNIT_TESTS

#endif // __KDAB__UNITTEST__TESTREGISTRY_H__
