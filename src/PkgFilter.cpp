/*
 *   File name: PkgFilter.h
 *   Summary:   Package manager Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QStringBuilder>

#include "PkgFilter.h"
#include "Logger.h"
#include "PkgInfo.h"


using namespace QDirStat;


QString PkgFilter::normalizedPattern( const QString & pattern )
{
    QString normalizedPattern = pattern;
    const QString pkgPrefix = u'^' % PkgInfo::pkgSummaryUrl() % u'*';
    normalizedPattern.remove( QRegularExpression( pkgPrefix, QRegularExpression::CaseInsensitiveOption ) );
    normalizedPattern.remove( QRegularExpression( "/.*$" ) );

    if ( normalizedPattern != pattern )
        logInfo() << "Normalizing pkg pattern to \"" << normalizedPattern << '"' << Qt::endl;

    return normalizedPattern;
}


QString PkgFilter::url() const
{
    return PkgInfo::pkgSummaryUrl() % pattern();
}
