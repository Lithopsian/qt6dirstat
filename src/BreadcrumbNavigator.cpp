/*
 *   File name: BreadcrumbNavigator.cpp
 *   Summary:   Breadcrumb widget for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

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


namespace
{
#if VERBOSE_BREADCRUMBS
    /**
     * With verbose logging, record each time a breadcrumb is clicked.
     **/
    void logPathClicked( const QString & path )
    {
        logInfo() << "Clicked path " << path << Qt::endl;
    }
#endif

    /**
     * Split a path up into its base path (everything up to the last path
     * component) and its base name (the last path component).
     *
     * Both 'basePath_ret' and 'name_ret' are return parameters and will be
     * modified by this function. If nonempty, a trailing path separator
     * ("/") is added to 'basePath_ret'.
     **/
    void splitBasePath( const QString & path,
                        QString       & basePath_ret, // return parameter
                        QString       & name_ret )    // return parameter
    {
        basePath_ret = "";
        name_ret = path;

        if ( path != "/"_L1 && path.contains( u'/' ) )
        {
            QStringList components = path.split( u'/', Qt::SkipEmptyParts );

            if ( !components.empty() )
                name_ret = components.takeLast();

            if ( !components.empty() )
                basePath_ret = components.join( u'/' ) % '/';

            if ( path.startsWith( u'/' ) )
                basePath_ret.prepend( u'/' );
        }
    }
} // namespace


BreadcrumbNavigator::BreadcrumbNavigator( QWidget * parent ):
    QLabel { parent }
{
    clear();

#if VERBOSE_BREADCRUMBS
    connect( this, &BreadcrumbNavigator::linkActivated,
             this, []( const QString & path ) { logPathClicked( path); } );
#endif

    connect( this, &BreadcrumbNavigator::linkActivated,
             this, &BreadcrumbNavigator::pathClicked );
}


void BreadcrumbNavigator::setPath( const FileInfo * item )
{
    fillBreadcrumbs( item );
    shortenBreadcrumbs();
    setText( html() );

#if VERBOSE_BREADCRUMBS
    logBreadcrumbs();
#endif
}


void BreadcrumbNavigator::fillBreadcrumbs( const FileInfo * item )
{
    // Since 5.7, the list will keep its capacity, so it will grow to the longest path used
    // This is still probably better than trying to squeeze it and re-allocate every time
    _breadcrumbs.clear();

    if ( item && !item->isDirInfo() )
        item = item->parent();

    if ( !item || !item->tree() )
        return;

    // logDebug() << item->debugUrl() << Qt::endl;
    int depth = item->treeLevel();
    _breadcrumbs = BreadcrumbList( depth + 1, Breadcrumb() );

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

    if ( _breadcrumbs.first().pathComponent.isEmpty() )
        _breadcrumbs.removeFirst();
}


QString BreadcrumbNavigator::html() const
{
    QString html;

    for ( const Breadcrumb & crumb : asConst( _breadcrumbs ) )
    {
        QString name = crumb.displayName;
        if ( name.isEmpty() )
            name = crumb.pathComponent;

        if ( !name.isEmpty() )
        {
            if ( crumb.url.isEmpty() )
                html += name.toHtmlEscaped();
            else
                html += "<a href=\""_L1 % crumb.url % "\">"_L1 % name.toHtmlEscaped() % "</a>"_L1;

            if ( !name.endsWith( u'/' ) )
                html += u'/';
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
#if VERBOSE_BREADCRUMBS
        logDebug() << "Shortened from " << longestCrumb->pathComponent.length()
                   << " to " << longestCrumb->displayName.length()
                   << ": " << longestCrumb->displayName << Qt::endl;
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

    for ( const Breadcrumb & crumb : asConst( _breadcrumbs ) )
    {
        const QString & name = crumb.displayName.isEmpty() ? crumb.pathComponent : crumb.displayName;

        len += name.length();

        if ( !name.endsWith( u'/' ) )
            ++len;      // For the "/" delimiter
    }

    return len;
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
                   << "\" url: \"" << crumb.url << '"'
                   << Qt::endl;
    }

    logNewline();
}
#endif
