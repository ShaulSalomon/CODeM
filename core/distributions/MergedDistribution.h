/****************************************************************************
**
** Copyright (C) 2012-2015 The University of Sheffield (www.sheffield.ac.uk)
**
** This file is part of Liger.
**
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General
** Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
****************************************************************************/
#ifndef MERGEDDISTRIBUTION_H
#define MERGEDDISTRIBUTION_H


#include <core/Distributions/IDistribution.h>

// Qt Includes
#include <QVector>
#include <QString>

class MergedDistribution : public IDistribution
{
public:
    MergedDistribution();
    MergedDistribution(const MergedDistribution& dist);
    virtual ~MergedDistribution();

    MergedDistribution* clone() const;

    void generateZ();
    void generatePDF();

    void appendDistribution(IDistribution* d);
    void appendDistribution(IDistribution* d, double ratio);

    void removeDistribution(IDistribution* d);
    void removeDistribution(int             idx);


    void changeRatio(IDistribution* d, double newRatio);
    void changeRatio(int             idx, double newRatio);

private:
    QVector<IDistribution*> m_distributions;
    QVector<double>             m_ratios;

    void addZSamplesOfOneDistribution(IDistribution* d);
    void addOnePDF(IDistribution* d,       double ratio);
};

#endif // MERGEDDISTRIBUTION_H
