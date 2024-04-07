/*
 *   File name: BreadcrumbNavigator.cpp
 *   Summary:	Breadcrumb widget for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


//#include "Qt4Compat.h" // qHtmlEscape()

#include "BreadcrumbNavigator.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "Logger.h"

#define VERBOSE_BREADCRUMBS     0

#define MAX_TOTAL_LEN         120
#define SHORTENED_LEN          10

using namespace QDirStat;



BreadcrumbNavigator::BreadcrumbNavigator( QWidget * parent ):
    QLabel ( parent )
{
    clear();
//    setTextFormat( Qt::RichText );

#if VERBOSE_BREADCRUMBS
    connect( this, &BreadcrumbNavigator::linkActivated,
             this, &BreadcrumbNavigator::logPathClicked );
#endif

    connect( this, &BreadcrumbNavigator::linkActivated,
             this, &BreadcrumbNavigator::pathClicked );
}


void BreadcrumbNavigator::setPath( const FileInfo * item )
{
    //logDebug() << Qt::endl;
    fillBreadcrumbs( item );
    shortenBreadcrumbs();
    setText( html() );
}


void BreadcrumbNavigator::fillBreadcrumbs( const FileInfo * item )
{
    _breadcrumbs.clear();

    while ( item && !item->isDirInfo() )
        item = item->parent();

    if ( !item || !item->tree() )
        return;

    // logDebug() << item->debugUrl() << Qt::endl;
    int depth = item->treeLevel();
    _breadcrumbs = QVector<Breadcrumb>( depth + 1, Breadcrumb() );

    QString name;
    QString basePath;

    const FileInfo * toplevel = item->tree()->firstToplevel();
    if ( !toplevel )
        return;

    splitBasePath( toplevel->name(), basePath, name );
    if ( !basePath.isEmpty() )
        _breadcrumbs[ 0 ].pathComponent = basePath;

    while ( item && depth > 0 )
    {
        // Stop at the DirTree's <root> pseudo item
        if ( item->tree() && item == item->tree()->root() )
            break;

        if ( item->isDirInfo() )
        {
            splitBasePath( item->name(), basePath, name );

            _breadcrumbs[ depth ].pathComponent = name;
            _breadcrumbs[ depth ].url = item->debugUrl();
        }

        item = item->parent();
        --depth;
    }

    if ( _breadcrumbs[ 0 ].pathComponent.isEmpty() )
        _breadcrumbs.removeFirst();

#if VERBOSE_BREADCRUMBS
    logBreadcrumbs();
#endif
}


QString BreadcrumbNavigator::html() const
{
    QString html;

    for ( const Breadcrumb & crumb : _breadcrumbs )
    {
        QString name = crumb.displayName;
        if ( name.isEmpty() )
            name = crumb.pathComponent;

        if ( !name.isEmpty() )
        {
            if ( crumb.url.isEmpty() )
                html += name.toHtmlEscaped();
            else
                html += QString( "<a href=\"%1\">%2</a>" ).arg( crumb.url ).arg( name.toHtmlEscaped() );

            if ( !name.endsWith( "/" ) )
                html += "/";
        }
    }

    return html;
}


void BreadcrumbNavigator::shortenBreadcrumbs()
{
    while ( breadcrumbsLen() > MAX_TOTAL_LEN )
    {
        Breadcrumb * longestCrumb = pickLongBreadcrumb();
        if ( !longestCrumb )
            return;

        longestCrumb->displayName = elideMiddle( longestCrumb->pathComponent, SHORTENED_LEN );
#if 1
        logDebug() << "Shortened from " << longestCrumb->pathComponent.length()
                   << " to " << longestCrumb->displayName.length()
                   << ": " << longestCrumb->pathComponent << Qt::endl;
#endif
    }
}


Breadcrumb * BreadcrumbNavigator::pickLongBreadcrumb()
{
    int maxLen = 0;
    Breadcrumb * longestCrumb = nullptr;

    for ( Breadcrumb & crumb : _breadcrumbs )
    {
        if ( crumb.displayName.isEmpty() && crumb.pathComponent.length() > maxLen )
        {
            longestCrumb = &crumb;
            maxLen = crumb.pathComponent.length();
        }
    }

    return longestCrumb;
}


int BreadcrumbNavigator::breadcrumbsLen() const
{
    int len = 0;

    for ( const Breadcrumb & crumb : _breadcrumbs )
    {
        const QString & name = crumb.displayName.isEmpty() ? crumb.pathComponent : crumb.displayName;

        len += name.length();

        if ( !name.endsWith( "/" ) )
            ++len;      // For the "/" delimiter
    }

    return len;
}


void BreadcrumbNavigator::splitBasePath( const QString & path,
                                         QString       & basePath_ret, // return parameter
                                         QString       & name_ret )    // return parameter
{
    basePath_ret = "";
    name_ret = path;

    if ( path != "/" && path.contains( "/" ) )
    {
        QStringList components = path.split( "/", Qt::SkipEmptyParts );

        if ( !components.empty() )
            name_ret = components.takeLast();

        if ( !components.empty() )
            basePath_ret = components.join( "/" ) + "/";

        if ( path.startsWith( "/" ) )
            basePath_ret.prepend( "/" );
    }
}

void BreadcrumbNavigator::logPathClicked( const QString & path )
{
    logInfo() << "Clicked path " << path << Qt::endl;
}


#if VERBOSE_BREADCRUMBS
void BreadcrumbNavigator::logBreadcrumbs() const
{
    logNewline();

    for ( int i=0; i < _breadcrumbs.size(); ++i )
    {
        const Breadcrumb & crumb = _breadcrumbs[ i ];

        logDebug() << "_breadcrumb[ " << i << " ]: "
                   << " pathComponent: \"" << crumb.pathComponent
                   << "\" displayName: \"" << crumb.displayName
                   << "\" url: " << crumb.url << "\""
                   << Qt::endl;
    }

    logNewline();
}
#endif
