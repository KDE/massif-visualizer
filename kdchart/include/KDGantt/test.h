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

#ifndef __KDAB__UNITTEST__TEST_H__
#define __KDAB__UNITTEST__TEST_H__

#ifndef KDAB_NO_UNIT_TESTS

#include "kdchart_export.h"
#include "kdganttglobal.h"

#include <QtGlobal>

#include <string>
#include <iostream>

namespace KDAB {
namespace UnitTest {

#define assertNotNull( x ) _assertNotNull( ( x ), #x, __FILE__, __LINE__ )
#define assertNull( x ) _assertNull( ( x ), #x, __FILE__, __LINE__ )
#define assertTrue( x )  _assertTrue( (x), #x, __FILE__, __LINE__ )
#define assertFalse( x ) _assertFalse( (x), #x, __FILE__, __LINE__ )
#define assertEqual( x, y ) _assertEqual( (x), (y), #x, #y, __FILE__, __LINE__ )
#define assertNotEqual( x, y ) _assertNotEqual( (x), (y), #x, #y, __FILE__, __LINE__ )
// to be phased out:
#define assertNearEqual( x, y, z )
#define assertEqualWithEpsilons( x, y, z ) _assertEqualWithEpsilons( (x), (y), (z), #x, #y, #z, __FILE__, __LINE__ )
#if 0
#define assertIsNaN( x ) _assertIsNaN( (x), #x, __FILE__, __LINE__ )
#define assertIsNotNaN( x ) _assertIsNotNaN( (x), #x, __FILE__, __LINE__ )
#endif

#define assertThrowsExceptionWithCode( x, E, code ) \
do {                           \
  try {                        \
    x;                         \
    fail( __FILE__, __LINE__ ) \
      << "\"" #x "\" didn't throw \"" #E "\"" << std::endl; \
  } catch ( E & ppq_ut_thrown ) {                           \
    success();                 \
    ( void )ppq_ut_thrown;     \
    code;                      \
  } catch ( ... ) {            \
    fail( __FILE__, __LINE__ ) \
      << "\"" #x "\" threw something, but it wasn't \"" #E "\"" << std::endl; \
  }                            \
} while ( false )

#define assertThrowsException( x, E ) assertThrowsExceptionWithCode( x, E, do{}while (0) )

#define assertDoesNotThrowException( x, E ) \
do {                           \
  try {                        \
    x;                         \
    success();                 \
  } catch ( E & ) {         \
    fail( __FILE__, __LINE__ ) \
      << "\"" #x "\" threw \"" #E "\", but shouldn't" << std::endl; \
  } catch ( ... ) {            \
    fail( __FILE__, __LINE__ ) \
      << "\"" #x "\" threw something, but it wasn't \"" #E "\"" << std::endl; \
  }                            \
} while ( false )


  class Test {
    const std::string mName;
    unsigned int mFailed, mSucceeded;
  public:
    Test( const std::string & name );
    virtual ~Test() {}

    const std::string & name() const { return mName; }
    unsigned int failed() const { return mFailed; }
    unsigned int succeeded() const { return mSucceeded; }

    virtual void run() = 0;

  protected:
    void _assertNotNull( const void * x, const char * expression, const char * file, unsigned int line );
    void _assertNull( const void * x, const char * expression, const char * file, unsigned int line );
#if 0
    void _assertIsNaN( qreal v, const char * expression, const char * file, unsigned int line );
    void _assertIsNotNaN( qreal v, const char * expression, const char * file, unsigned int line );
#endif
    void _assertTrue( bool x, const char * expression, const char * file, unsigned int line );
    void _assertFalse( bool x, const char * expression, const char * file, unsigned int line );

    void _assertEqualWithEpsilons( float x1, float x2, int prec, const char * expr1, const char * expr2, const char * exprPrec, const char * file, unsigned int line );
    void _assertEqualWithEpsilons( qreal x1, qreal x2, int prec, const char * expr1, const char * expr2, const char * exprPrec, const char * file, unsigned int line );
    void _assertEqualWithEpsilons( long double x1, long double x2, int prec, const char * expr1, const char * expr2, const char * exprPrec, const char * file, unsigned int line );

    template <typename T, typename S>
    void _assertEqual( const T & x1, const S & x2, const char * expr1, const char * expr2, const char * file, unsigned int line ) {
      if ( x1 == x2 ) this->success();
      else {
          this->fail( file, line ) << '"' << expr1 << "\" yielded " << x1 << "; expected: " << x2 << "(\"" << expr2 << "\")" << std::endl;
      }
    }
    template <typename T, typename S>
    void _assertNotEqual( const T & x1, const S & x2, const char * expr1, const char * expr2, const char * file, unsigned int line ) {
      if ( x1 != x2 ) this->success();
      else {
          this->fail( file, line ) << '"' << expr1 << "\" yielded " << x1 << "; expected something not equal to: " << x2 << "(\"" << expr2 << "\")" << std::endl;
      }
    }

  protected:
    std::ostream & fail( const char * file, unsigned int line );
    void success() {
      ++mSucceeded;
    }
  };

  class TestFactory {
  public:
    virtual ~TestFactory() {}
    virtual Test * create() const = 0;
  };

}
}

#include "testregistry.h"

namespace KDAB {
namespace UnitTest {

  template <typename T_Test>
  class GenericFactory : public TestFactory {
  public:
    GenericFactory( const char * group=0 ) {
      TestRegistry::instance()->registerTestFactory( this, group );
    }
    Test * create() const { return new T_Test(); }
  };

}
}

#include "libutil.h"

// Use these macros to export your UnitTest class so that it gets executed by the test runner.
// Use the second macro if your class is within a namespace.
// Arguments :
// - Namespace (unquoted) : the namespace in which the test class in contained
// - Class (unquoted) : the test class, without namespace
// - Group (quoted) : the name of the group this unit test belongs to
#define KDAB_EXPORT_UNITTEST( Class, Group ) \
    static const KDAB::UnitTest::GenericFactory< Class > __##Class##_unittest( Group ); \
    KDAB_EXPORT_STATIC_SYMBOLS( Class )

#define KDAB_EXPORT_SCOPED_UNITTEST( Namespace, Class, Group ) \
    static const KDAB::UnitTest::GenericFactory< Namespace::Class > __##Class##_unittest( Group ); \
    KDAB_EXPORT_STATIC_SYMBOLS( Class )

// Use this macro to import the test explicitly (for windows static libs only)
#define KDAB_IMPORT_UNITTEST( Class ) KDAB_IMPORT_STATIC_SYMBOLS( Class )

// Convenience macros that create a simple test class for a single test and export it.
// Usage : KDAB_UNITTEST_SIMPLE( MyClass, "mygroup" ) { doSomething(); assertEqual(...); }
#define KDAB_UNITTEST_SIMPLE( Class, Group )                 \
    class Class##Test : public KDAB::UnitTest::Test {  \
    public:                                                 \
        Class##Test() : Test( #Class ) {}                   \
        void run();                                         \
    };                                                      \
    KDAB_EXPORT_UNITTEST( Class##Test, Group )               \
    void Class##Test::run()

#define KDAB_SCOPED_UNITTEST_SIMPLE( Namespace, Class, Group )   \
    namespace Namespace {                                       \
        class Class##Test : public KDAB::UnitTest::Test {  \
        public:                                                 \
            Class##Test() : Test( #Namespace "::" #Class ) {}    \
            void run();                                         \
        };                                                      \
    }                                                           \
    KDAB_EXPORT_SCOPED_UNITTEST( Namespace, Class##Test, Group ) \
    void Namespace::Class##Test::run()

#endif // KDAB_NO_UNIT_TESTS

#endif // __KDAB__UNITTEST__TEST_H__
