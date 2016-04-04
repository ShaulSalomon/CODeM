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
#include <tigon/Representation/Distributions/IDistribution.h>
#include <tigon/Random/RandomGenerator.h>
#include <tigon/Utils/LinearInterpolator.h>
#include <tigon/Utils/TigonUtils.h>

namespace Tigon {
namespace Representation {

IDistribution::IDistribution()
{
    m_type = Tigon::GenericDistType;
    m_nSamples = 0;
    m_lb = 0;
    m_ub = 1;
    m_dz = (m_ub-m_lb)/(Tigon::DistMinNSamples - 1);
    m_pdfInterpolator      = 0;
    m_cdfInterpolator      = 0;
    m_quantileInterpolator = 0;
}

IDistribution::IDistribution(const IDistribution& dist)
{
    m_type = dist.m_type;
    m_dz = dist.m_dz;
    m_lb = dist.m_lb;
    m_ub = dist.m_ub;
    m_z = dist.m_z;
    m_pdf = dist.m_pdf;
    m_cdf = dist.m_cdf;
    m_nSamples = dist.m_nSamples;
    m_pdfInterpolator      = 0;
    m_cdfInterpolator      = 0;
    m_quantileInterpolator = 0;
}

IDistribution::IDistribution(qreal value)
{
    m_type = Tigon::GenericDistType;
    m_nSamples = 0;
    m_lb = 0;
    m_ub = 1;
    m_dz = (m_ub-m_lb)/(Tigon::DistMinNSamples - 1);
    m_pdfInterpolator      = 0;
    m_cdfInterpolator      = 0;
    m_quantileInterpolator = 0;

    defineBoundaries(value,value);
}

IDistribution::~IDistribution()
{
    if(m_pdfInterpolator != 0) {
        delete m_pdfInterpolator;
    }
    if(m_cdfInterpolator != 0) {
        delete m_cdfInterpolator;
    }
    if(m_quantileInterpolator != 0) {
        delete m_quantileInterpolator;
    }
}

IDistribution* IDistribution::clone() const
{
    return (new IDistribution(*this));
}

Tigon::DistributionType IDistribution::type() const
{
    return m_type;
}

QVector<qreal> IDistribution::parameters()
{
    return QVector<qreal>();
}

qreal IDistribution::sample()
{
    if(m_quantileInterpolator == 0) {
        m_quantileInterpolator = new LinearInterpolator(cdf(), zSamples());
    } else {
        m_quantileInterpolator->defineXY(cdf(), zSamples());
    }
    qreal r = TRAND.randUni();
    // A value between 0-1: 0==>lb , 1==>ub
    qreal sample = m_quantileInterpolator->interpolate(r);
    return sample;
}

qreal IDistribution::mean()
{
    if(m_pdf.isEmpty()) {
        calculateCDF();
    }
    qreal sum  = 0.0;
    for(int i=0; i<m_nSamples-1; i++) {
        qreal cur  = m_pdf[i] * m_z[i];
        qreal next = m_pdf[i+1] * m_z[i+1];
        sum += (cur+next)/2 * (m_z[i+1] - m_z[i]);
    }
    return sum;
}

qreal IDistribution::median()
{
    if(m_quantileInterpolator == 0) {
        m_quantileInterpolator = new LinearInterpolator(cdf(),zSamples());
    } else {
        m_quantileInterpolator->defineXY(cdf(),zSamples());
    }
    return m_quantileInterpolator->interpolate(0.5);
}

qreal IDistribution::percentile(qreal p)
{
    if(m_cdf.isEmpty() || m_cdf.size() != m_nSamples) {
        calculateCDF();
    }

    if(p>=1.0) {
        return m_ub;
    } else if(p<=0.0) {
        return m_lb;
    }

    if(m_quantileInterpolator == 0) {
        m_quantileInterpolator = new LinearInterpolator(cdf(),zSamples());
    } else {
        m_quantileInterpolator->defineXY(cdf(),zSamples());
    }
    return m_quantileInterpolator->interpolate(p);
}

qreal IDistribution::variance()
{
    qreal m = mean();
    qreal sum  = 0.0;
    for(int i=0; i<m_nSamples-1; i++) {
        qreal cur  = m_pdf[i] * m_z[i]*m_z[i];
        qreal next = m_pdf[i+1] * m_z[i+1]*m_z[i+1];
        sum += (cur+next)/2 * (m_z[i+1] - m_z[i]);
    }
    return sum - m*m;
}

qreal IDistribution::std()
{
    return sqrt(variance());
}

QVector<qreal> IDistribution::pdf()
{
    if(m_pdf.isEmpty() || m_pdf.size() != m_nSamples) {
        generatePDF();
    }
    return m_pdf;
}

QVector<qreal> IDistribution::cdf()
{
    if(m_cdf.isEmpty() || m_cdf.size() != m_nSamples) {
        calculateCDF();
    }
    return m_cdf;
}

QVector<qreal> IDistribution::pdf(const QVector<qreal> zVec)
{
    QVector<qreal> ret(zVec.size());
    for(int i=0; i< zVec.size(); i++) {
        ret[i] = pdf(zVec[i]);
    }
    return ret;
}

QVector<qreal> IDistribution::cdf(const QVector<qreal> zVec)
{
    QVector<qreal> ret(zVec.size());
    for(int i=0; i< zVec.size(); i++) {
        ret[i] = cdf(zVec[i]);
    }
    return ret;
}

qreal IDistribution::pdf(qreal z)
{
    if(m_pdf.isEmpty() || m_pdf.size() != m_nSamples) {
        generatePDF();
    }

    if(z < m_lb || z > m_ub) {
        return 0.0;
    }

    if(m_pdfInterpolator == 0) {
        m_pdfInterpolator = new LinearInterpolator(zSamples(), pdf());
    } else {
        m_pdfInterpolator->defineXY(zSamples(), pdf());
    }


    return m_pdfInterpolator->interpolate(z);
}

qreal IDistribution::cdf(qreal z)
{
    if(m_cdf.isEmpty() || m_cdf.size() != m_nSamples) {
        calculateCDF();
    }

    if(z <= m_lb) {
        return 0.0;
    } else if(z >= m_ub) {
        return 1.0;
    } else {
        if(m_cdfInterpolator == 0) {
            m_cdfInterpolator = new LinearInterpolator(zSamples(), cdf());
        } else {
            m_cdfInterpolator->defineXY(zSamples(), cdf());
        }
        return m_cdfInterpolator->interpolate(z);
    }
}

void IDistribution::defineResolution(qreal dz)
{
    if(dz > 0) {
        // make sure the range is a multiple of m_dz, and m_dz <= dz
        m_dz = (m_ub-m_lb) / ceil((m_ub-m_lb)/dz);
    }
}

qreal IDistribution::resolution() const
{
    return m_dz;
}

void IDistribution::defineBoundaries(qreal lb, qreal ub)
{
    if(lb >= ub) {
        if(lb == 0) {
            ub = Tigon::DistMinInterval;
        } else if(lb > 0) {
            ub = lb * (1 + Tigon::DistMinInterval);
        } else {
            lb = ub * (1 + Tigon::DistMinInterval);
        }
    }

    // TODO: test scaling

    qreal oldRange = m_ub - m_lb;
    qreal newRange = ub - lb;
    qreal ratio    = newRange / oldRange;

    m_lb = lb;
    m_ub = ub;

    defineResolution(m_dz * ratio);

    if(!m_z.isEmpty()) {
        for(int i=0; i<m_nSamples; i++) {
            m_z[i] = lb + ratio * (m_z[i] - m_lb);
        }
    }

    if(!m_pdf.isEmpty()) {
        normalise();
    }
}

qreal IDistribution::lowerBound() const
{
    return m_lb;
}

qreal IDistribution::upperBound() const
{
    return m_ub;
}

void IDistribution::defineZ(QVector<qreal> z)
{
    std::sort(z.begin(),z.end());
    QMutableVectorIterator<qreal> i(z);
    if(i.hasNext()) {
        i.next();
        while(i.hasNext()) {
            if(i.peekPrevious() == i.next()) {
                i.remove();
            }
        }
    }

    if(m_pdf.size() != z.size()) {
        m_pdf.clear();
    }

    if(z.size() >= 2) {
        m_z = z;
        m_lb = m_z.first();
        m_ub = m_z.last();
        m_nSamples = m_z.size();
    } else {
        m_z.clear();
        m_nSamples = 0;
        defineBoundaries(z[0],z[0]);
    }
}

QVector<qreal> IDistribution::zSamples()
{
    if(m_z.isEmpty()) {
        generateZ();
    }
    return m_z;
}

void IDistribution::generateZ()

{
    generateEquallySpacedZ();
}

void IDistribution::generatePDF()
{
    // uniform distribution
    qreal probability = 1.0/(m_ub - m_lb);
    m_pdf.fill(probability, m_nSamples);
}

void IDistribution::generateEquallySpacedZ()
{
    m_nSamples = (int)((m_ub-m_lb)/m_dz) + 1;
    m_z.resize(m_nSamples);
    qreal zz=m_lb;
    for(int i=0; i<m_z.size()-1; i++) {
        m_z[i] = zz;
        zz += m_dz;
    }
    m_z[m_z.size()-1] = m_ub;
}

void IDistribution::calculateCDF()
{
    if(m_pdf.isEmpty()) {
        generatePDF();
    }
    m_cdf.fill(0.0, m_nSamples);
    qreal cur  = 0.0;
    qreal next = 0.0;
    for(int i=0; i<m_nSamples-1; i++) {
        cur  = m_pdf[i];
        next = m_pdf[i+1];
        m_cdf[i+1] = m_cdf[i] + (cur+next)/2 * (m_z[i+1] - m_z[i]);
    }

    // normalise
    qreal factor = m_cdf.last();
    if(factor == 1.0) {
        return;
    } else if(factor == 0.0) {
        qreal probability = 1.0/(m_ub - m_lb);
        m_pdf = QVector<qreal>(m_nSamples, probability);
        calculateCDF();
    } else {
        for(int i=0; i<m_cdf.size(); i++) {
            m_pdf[i] /= factor;
            m_cdf[i] /= factor;
        }
    }
}

void IDistribution::normalise()
{
    if(m_pdf.isEmpty()) {
        return;
    }

    calculateCDF();
}

void IDistribution::negate()
{
    qreal ub = m_ub;
    m_ub = -m_lb;
    m_lb = -ub;

    if(m_z.isEmpty()) {
        return;
    }
    QVector<qreal> newZ(m_nSamples);
    for(int i=0; i<m_nSamples; i++) {
        newZ[i] = -m_z[m_nSamples-1-i];
    }
    m_z.swap(newZ);

    if(m_pdf.isEmpty()) {
        return;
    }
    QVector<qreal> newPdf(m_nSamples);
    for(int i=0; i<m_nSamples; i++) {
        newPdf[i] = m_pdf[m_nSamples-1-i];
    }
    m_pdf.swap(newPdf);
    calculateCDF();
}

void IDistribution::add(qreal num)
{
    m_ub += num;
    m_lb += num;

    if(m_z.isEmpty()) {
        return;
    }
    for(int i=0; i<m_nSamples; i++) {
        m_z[i] += num;
    }

    if(!m_pdf.isEmpty()) {
        calculateCDF();
    }
}

void IDistribution::add(const IDistributionSPtr other)
{
    qreal lbO = other->lowerBound();
    qreal ubO = other->upperBound();
    qreal lb = m_lb + lbO;
    qreal ub = m_ub + ubO;
    qreal dz = qMax(m_ub-m_lb, ubO-lbO)/Tigon::DistConvNSamples;

    int nSampT = (int)((m_ub-m_lb)/dz) + 1;
    int nSampO = (int)((ubO-lbO)/dz) + 1;
    int nSamples = nSampT + nSampO - 1;

    // corrections to dz
    dz = (ub-lb)/(nSamples-1);
    qreal dzT = (m_ub-m_lb) / (nSampT-1);
    qreal dzO = (ubO - lbO) / (nSampO-1);

    // evenly distributed samples for convoluted distribution
    QVector<qreal> z(nSamples);
    qreal zz = lb;
    for(int i=0; i<nSamples-1; i++) {
        z[i] = zz;
        zz += dz;
    }
    z[nSamples-1] = ub;

    // and for original distributions
    QVector<qreal> pdfT(nSampT);
    zz = m_lb;
    for(int i=0; i<nSampT-1; i++) {
        pdfT[i] = pdf(zz);
        zz += dzT;
    }
    pdfT[nSampT-1] = pdf(m_ub);

    QVector<qreal> pdfO(nSampO);
    zz = lbO;
    for(int i=0; i<nSampO-1; i++) {
        pdfO[i] = other->pdf(zz);
        zz += dzO;
    }
    pdfO[nSampO-1] = other->pdf(ubO);

    // convolute the two pdfs
    m_pdf = conv(pdfT,pdfO);

    // update the distribution
    m_z.swap(z);
    m_ub = ub;
    m_lb = lb;
    m_dz = dz;
    m_nSamples = nSamples;

    normalise();
}

void IDistribution::subtract(qreal num)
{
    add(-num);
}

void IDistribution::subtract(const IDistributionSPtr other)
{
    IDistributionSPtr minusOther(other->clone());
    minusOther->negate();
    add(minusOther);
}

void IDistribution::multiply(qreal num)
{
    if(num == 0)
    {
        m_lb = 0.0;
        m_ub = Tigon::DistMinInterval;
        m_dz = Tigon::DistMinInterval/(Tigon::DistMinNSamples-1);
        m_z.clear();
        m_z << m_lb << (m_lb+m_ub)/2 << m_ub;
        m_pdf.fill(1.0,2);
        normalise();
        return;
    }

    if(num < 0) {
        negate();
        num = -num;
    }

    m_ub *= num;
    m_lb *= num;

    if(m_z.isEmpty()) {
        return;
    }
    for(int i=0; i<m_nSamples; i++) {
        m_z[i] *= num;
    }

    if(!m_pdf.isEmpty()) {
        calculateCDF();
    }
}

void IDistribution::multiply(const IDistributionSPtr other)
{
    qreal lbO = other->lowerBound();
    qreal ubO = other->upperBound();
    qreal lb  = qMin(m_lb*lbO,qMin(m_lb*ubO,qMin(m_ub*lbO,m_ub*ubO)));
    qreal ub  = qMax(m_lb*lbO,qMax(m_lb*ubO,qMax(m_ub*lbO,m_ub*ubO)));

    int nSamples = Tigon::DistMultNSamples;
    qreal dz  = (ub-lb) / (nSamples-1);
    qreal dzT = (m_ub-m_lb) / (nSamples-1);

    // evenly distributed samples for joint multiplied distribution
    QVector<qreal> z(nSamples);
    qreal zz = lb;
    for(int i=0; i<nSamples-1; i++) {
        z[i] = zz;
        zz += dz;
    }
    z[nSamples-1] = ub;

    // and for original distributions
    QVector<qreal> zT(nSamples);
    zz = m_lb;
    for(int i=0; i<nSamples-1; i++) {
        zT[i] = zz;
        zz += dzT;
    }
    zT[nSamples-1] = m_ub;

    // multiply the two distributions
    QVector<qreal> newPDF(nSamples);
    for(int i=0; i<nSamples; i++) {
        for(int j=0; j<nSamples; j++) {
            qreal zt = zT[j];
            if(qAbs(zt) >= dzT/2) {
                qreal zo = z[i]/zt;
                if(zo >= lbO && zo <= ubO) {
                    newPDF[i] += pdf(zt) * other->pdf(zo) / qAbs(zt);
                }
            } else {
                zt = -dzT/2;
                qreal zo = z[i]/zt;
                if(zo >= lbO && zo <= ubO) {
                    newPDF[i] += pdf(zt) * other->pdf(zo) / qAbs(zt) / 2.0;
                }
                zt = dzT/2;
                zo = z[i]/zt;
                if(zo >= lbO && zo <= ubO) {
                    newPDF[i] += pdf(zt) * other->pdf(zo) / qAbs(zt) / 2.0;
                }
            }
        }
        newPDF[i] *= dzT;
    }

    // update the distribution
    m_pdf.swap(newPDF);
    m_z.swap(z);
    m_ub = ub;
    m_lb = lb;
    m_dz = dz;
    m_nSamples = nSamples;

    normalise();
}

void IDistribution::divide(qreal num)
{
    if(num == 0)
    {
        return;
    }

    multiply(1.0/num);
}

void IDistribution::divide(const IDistributionSPtr other)
{
    qreal lbO = other->lowerBound();
    qreal ubO = other->upperBound();
    qreal lb,ub;

    // Division by zero
    if((sgn(lbO) != sgn(ubO)) || (lbO == 0.0) || (ubO == 0.0)) {
        if((sgn(m_lb) != sgn(m_ub)) || (sgn(lbO) != sgn(ubO))){
            lb = Tigon::Lowest;
            ub =  Tigon::Highest;
        } else if(lbO == 0.0) {
            if(m_lb >= 0.0) {
                lb = 0.0;
                ub =  Tigon::Highest;
            } else {
                lb = Tigon::Lowest;
                ub =  0.0;
            }
        } else {
            if(m_lb >= 0.0) {
                lb = Tigon::Lowest;
                ub =  0.0;
            } else {
                lb = 0.0;
                ub =  Tigon::Highest;
            }
        }

        IDistribution::defineBoundaries(lb, ub);
        generateEquallySpacedZ();
        IDistribution::generatePDF();
        return;
    }

    lb  = qMin(m_lb/lbO,qMin(m_lb/ubO,qMin(m_ub/lbO,m_ub/ubO)));
    ub  = qMax(m_lb/lbO,qMax(m_lb/ubO,qMax(m_ub/lbO,m_ub/ubO)));


    int nSamples = Tigon::DistMultNSamples;
    qreal dz  = (ub-lb) / (nSamples-1);
    qreal dzO = (ubO-lbO) / (nSamples-1);

    // evenly distributed samples for joint divided distribution
    QVector<qreal> z(nSamples);
    qreal zz = lb;
    for(int i=0; i<nSamples-1; i++) {
        z[i] = zz;
        zz += dz;
    }
    z[nSamples-1] = ub;

    // and for original distribution
    QVector<qreal> zO(nSamples);
    zz = lbO;
    for(int i=0; i<nSamples-1; i++) {
        zO[i] = zz;
        zz += dzO;
    }
    zO[nSamples-1] = ubO;

    // divide the two distributions
    QVector<qreal> newPDF(nSamples);
    for(int i=0; i<nSamples; i++) {
        for(int j=0; j<nSamples; j++) {
            qreal zo = zO[j];
                qreal zt = z[i]*zo;
                if(zt >= m_lb && zt <= m_ub) {
                    newPDF[i] += other->pdf(zo) * pdf(zt) * qAbs(zo);
                }
        }
        newPDF[i] *= dzO;
    }

    // update the distribution
    m_pdf.swap(newPDF);
    m_z.swap(z);
    m_ub = ub;
    m_lb = lb;
    m_dz = dz;
    m_nSamples = nSamples;

    normalise();
}

void IDistribution::reciprocal()
{
    qreal lb,ub;

    // Division by zero
    if( (sgn(m_lb) != sgn(m_ub)) || (m_lb == 0.0) || (m_ub == 0.0) ) {
        if(sgn(m_lb) != sgn(m_ub)) {
            lb = Tigon::Lowest;
            ub =  Tigon::Highest;
        } else if(m_lb == 0.0) {
            lb = 0.0;
            ub =  Tigon::Highest;
        } else {
            lb = Tigon::Lowest;
            ub =  0.0;
        }

        IDistribution::defineBoundaries(lb, ub);
        generateEquallySpacedZ();
        IDistribution::generatePDF();
        return;
    }

    lb = 1.0 / m_ub;
    ub = 1.0 / m_lb;

    int nSamples = Tigon::DistMultNSamples;
    qreal dz  = (ub-lb) / (nSamples-1);

    // evenly distributed samples for reciprocal distribution
    QVector<qreal> z(nSamples);
    qreal zz = lb;
    for(int i=0; i<nSamples-1; i++) {
        z[i] = zz;
        zz += dz;
    }
    z[nSamples-1] = ub;

//    // and for original distribution
//    QVector<qreal> zT(nSamples);
//    zz = m_lb;
//    for(int i=0; i<nSamples-1; i++) {
//        zT[i] = zz;
//        zz += dzT;
//    }
//    zT[nSamples-1] = m_ub;

    // invert the distribution
    QVector<qreal> newPDF(nSamples);
    for(int i=0; i<nSamples; i++) {
        qreal zt = 1.0 / z[i];
        newPDF[i] = pdf(zt) * qAbs(zt*zt);
    }

    // update the distribution
    m_pdf.swap(newPDF);
    m_z.swap(z);
    m_ub = ub;
    m_lb = lb;
    m_dz = dz;
    m_nSamples = nSamples;

    normalise();
}

} // namespace Representation
} // namespace Tigon