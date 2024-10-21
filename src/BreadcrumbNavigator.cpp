/*
 *   File name: BreadcrumbNavigator.cpp
 *   Summary:   Breadcrumb widget for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QLayout>

#include "BreadcrumbNavigator.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "Logger.h"


#define VERBOSE_BREADCRUMBS 0


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


    /**
     * Write a BreadcrumbList to the log.
     **/
    void logBreadcrumbs( const BreadcrumbList & breadcrumbs )
    {
	logNewline();

	for ( int i=0; i < breadcrumbs.size(); ++i )
	{
	    const Breadcrumb & crumb = breadcrumbs[ i ];

	    logDebug() << "_breadcrumb[ " << i << " ]: "
	               << " pathComponent: \"" << crumb.pathComponent
	               << "\" displayName: \"" << crumb.elidedName
	               << "\" url: \"" << crumb.url << '"'
	               << Qt::endl;
	}

	logNewline();
    }
#endif

    /**
     * Return the total display length of all breadcrumbs plus delimiters.
     **/
    int breadcrumbsLen( const BreadcrumbList & breadcrumbs, const QFont & font )
    {
	const QChar delimiter{ u'/' };
	const int delimiterWidth = horizontalAdvance( font, delimiter );
	int len = 0;

	for ( const Breadcrumb & crumb : breadcrumbs )
	{
	    const QString & name = crumb.displayName();
	    len += horizontalAdvance( font, name );
	    if ( !name.endsWith( delimiter ) )
		len += delimiterWidth;
	}

	return len;
    }


    /**
     * Return the longest breadcrumb that can still be elided (ie. more
     * than one character) or 0 if there are no more.
     **/
    Breadcrumb * pickLongBreadcrumb( BreadcrumbList & breadcrumbs )
    {
	int maxLen = 1;
	Breadcrumb * longestCrumb = nullptr;

	for ( Breadcrumb & crumb : breadcrumbs )
	{
	    const QString & name = crumb.displayName();
	    if ( name.length() > maxLen )
	    {
		longestCrumb = &crumb;
		maxLen = name.length();
	    }
	}

	return longestCrumb;
    }


    /**
     * Shorten excessively long breadcrumbs so they have a better chance to
     * fit on the screen.
     *
     * This iterative process keeps eliding the longest name by up to a
     * half until the path as a whole fits in the available space.  This
     * avoids excessively shortening one component and leaving others
     * much longer, or eliding to a stump when there is much more space
     * available.
     **/
    void shortenBreadcrumbs( BreadcrumbList & breadcrumbs, const QFont & font, int maxLength )
    {
	// Don't try to elide a component to smaller than the ellipsis character
	const int minLength = ellipsisWidth( font );

	while ( true )
	{
	    // Keep shortening until it fits ...
	    const int currentLength = breadcrumbsLen( breadcrumbs, font );
	    if ( currentLength <= maxLength )
		return;

	    // ... or we can't elide anything else
	    Breadcrumb * longestCrumb = pickLongBreadcrumb( breadcrumbs );
	    if ( !longestCrumb )
		return;

	    // Shorten the current longest component by as much as needed, but by no more than half
	    const QString & crumbName = longestCrumb->displayName();
	    const int crumbLength = horizontalAdvance( font, crumbName );
	    const int elideLength = qMax( crumbLength - ( currentLength - maxLength ), crumbLength / 2 );
	    longestCrumb->elidedName = elidedText( font, crumbName, qMax( elideLength, minLength ) );

#if VERBOSE_BREADCRUMBS
	    logDebug() << "Shortened from " << crumbLength
	               << " to " << elideLength
	               << ": " << horizontalAdvance( font, longestCrumb->elidedName ) << Qt::endl;
#endif
	}
    }


    /**
     * Generate HTML from a BreadcrumbList.  Carriage returns and
     * linefeeds have already been replaced by spaces, but escape
     * special characters and wrap to prevent whitespace being
     * collapsed.
     **/
    QString toHtml( const BreadcrumbList & breadcrumbs )
    {
	const QString crumbTemplate{ "<a href=\"%1\">%2</a>" };

	QString html;

	for ( const Breadcrumb & crumb : breadcrumbs )
	{
	    const QString & name = crumb.displayName();
	    if ( !name.isEmpty() )
	    {
		const QString escapedName = name.toHtmlEscaped();
		if ( crumb.url.isEmpty() )
		    html += escapedName;
		else
		    html += crumbTemplate.arg( crumb.url, escapedName );

		if ( !name.endsWith( u'/' ) )
		    html += u'/';
	    }
	}

	return whitespacePre( html );
    }


    /**
     * Split a path up into its base path (everything up to the last path
     * component) and its base name (the last path component).
     *
     * Both 'basePath_ret' and 'name_ret' are return parameters and will be
     * modified by this function. If non-empty, a trailing path separator
     * ("/") is added to 'basePath_ret'.
     **/
    void splitBasePath( const QString & path,
                        QString       & basePath_ret, // return parameter
                        QString       & name_ret )    // return parameter
    {
	basePath_ret = QString();
	name_ret = path;

	if ( path != "/"_L1 && path.contains( u'/' ) )
	{
	    QStringList components{ path.split( u'/', Qt::SkipEmptyParts ) };

	    if ( !components.empty() )
		name_ret = components.takeLast();

	    if ( !components.empty() )
		basePath_ret = components.join( u'/' ) % '/';

	    if ( path.startsWith( u'/' ) )
		basePath_ret.prepend( u'/' );
	}
    }


    /**
     * Create a BreadcrumbList by traversing the tree from
     * 'item' to the toplevel.
     **/
    BreadcrumbList fillBreadcrumbs( const FileInfo * item )
    {
	if ( item && !item->isDirInfo() )
	    item = item->parent();

	if ( !item || !item->tree() )
	    return BreadcrumbList{};

	const DirTree * tree = item->tree();
	const FileInfo * toplevel = tree->firstToplevel();
	if ( !toplevel )
	    return BreadcrumbList{};

	// logDebug() << item->debugUrl() << Qt::endl;
	int depth = item->treeLevel();
	BreadcrumbList breadcrumbs = BreadcrumbList( depth + 1 );

	QString name;
	QString basePath;

	splitBasePath( toplevel->name(), basePath, name );
	if ( !basePath.isEmpty() )
	    breadcrumbs[ 0 ].pathComponent = replaceCrLf( basePath );

	// Redundant loop conditions just as sanity checks
	while ( depth > 0 && item && item != tree->root() )
	{
	    if ( item->isDirInfo() )
	    {
		splitBasePath( item->name(), basePath, name );

		breadcrumbs[ depth ].pathComponent = replaceCrLf( name );
		breadcrumbs[ depth ].url = item->debugUrl();
	    }

	    item = item->parent();
	    --depth;
	}

	if ( breadcrumbs.first().pathComponent.isEmpty() )
	    breadcrumbs.removeFirst();

	return breadcrumbs;
    }

} // namespace


BreadcrumbNavigator::BreadcrumbNavigator( QWidget * parent ):
    QLabel{ parent }
{
    clear();

#if VERBOSE_BREADCRUMBS
    connect( this, &BreadcrumbNavigator::linkActivated,
             []( const QString & path ) { logPathClicked( path); } );
#endif

    connect( this, &BreadcrumbNavigator::linkActivated,
             this, &BreadcrumbNavigator::pathClicked );
}


void BreadcrumbNavigator::setPath( const FileInfo * item )
{
    // Break the item pathname into components
    _breadcrumbs = fillBreadcrumbs( item );

    // Ensure that the layout already properly represents the main window geometry
    QLayout * layout = parentWidget()->layout();
    if ( !layout->geometry().isValid() )
	layout->activate();

    // Elide components until the whole label will fit inside the central widget
    int leftMargin;
    int rightMargin;
    layout->getContentsMargins( &leftMargin, &rightMargin, nullptr, nullptr );
    const int maxWidth = layout->contentsRect().width() - leftMargin - rightMargin;
    shortenBreadcrumbs( _breadcrumbs, font(), maxWidth - ellipsisWidth( font() ) );

#if VERBOSE_BREADCRUMBS
    logBreadcrumbs( _breadcrumbs );
#endif

    setText( toHtml( _breadcrumbs ) );
}
