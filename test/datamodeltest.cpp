/*
   This file is part of Massif Visualizer

   Copyright 2010 Milian Wolff <mail@milianw.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "datamodeltest.h"

#include "modeltest.h"

#include "massifdata/parser.h"
#include "massifdata/filedata.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"

#include "visualizer/totalcostmodel.h"
#include "visualizer/detailedcostmodel.h"
#include "visualizer/datatreemodel.h"
#include "visualizer/util.h"

#include <QtCore/QFile>
#include <QtTest/QTest>
#include <QtCore/QDebug>

QTEST_MAIN(DataModelTest)

using namespace Massif;

void DataModelTest::parseFile()
{
    const QString path = QString(KDESRCDIR) + "/data/massif.out.kate";
    QFile* file = new QFile(path);
    QVERIFY(file->open(QIODevice::ReadOnly));

    Parser parser;
    FileData* data = parser.parse(file);
    QVERIFY(data);

    {
        TotalCostModel* model = new TotalCostModel(this);
        new ModelTest(model, this);
        model->setSource(data);
        QVERIFY(model->rowCount() == data->snapshots().size());
        for ( int r = 0; r < model->rowCount(); ++r ) {
            for ( int c = 0; c < model->columnCount(); ++c ) {
                qDebug() << r << c << model->data(model->index(r, c));
            }
        }
        // remove data
        model->setSource(0);
    }

    {
        DetailedCostModel* model = new DetailedCostModel(this);
        new ModelTest(model, this);
        model->setSource(data);
        for ( int r = 0; r < model->rowCount(); ++r ) {
            for ( int c = 0; c < model->columnCount(); ++c ) {
                qDebug() << r << c << model->data(model->index(r, c));
            }
            if ( r ) {
                // we want that the snapshots are ordered properly
                QVERIFY(model->data(model->index(r, 0)).toDouble() > model->data(model->index(r - 1, 0)).toDouble());
            }
        }
        // remove data
        model->setSource(0);
    }

    {
        DataTreeModel* model = new DataTreeModel(this);
        new ModelTest(model, this);
        model->setSource(data);
        QVERIFY(model->rowCount() == data->snapshots().size());
        // remove data
        model->setSource(0);
    }

}

void DataModelTest::testUtils()
{
    QString l("0x6F675AB: KDevelop::IndexedIdentifier::IndexedIdentifier(KDevelop::Identifier const&) (identifier.cpp:1050)");
    QCOMPARE(prettyLabel(l), QString("KDevelop::IndexedIdentifier::IndexedIdentifier(KDevelop::Identifier const&) (identifier.cpp:1050)"));
    QCOMPARE(functionInLabel(l), QString("KDevelop::IndexedIdentifier::IndexedIdentifier(KDevelop::Identifier const&)"));
}
