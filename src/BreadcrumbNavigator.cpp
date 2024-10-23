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
#include "SysUtil.h"


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
     * Return the total display length in pixels of all breadcrumbs
     * plus delimiters.
     *
     * Note that this is not treated as rich text, but the font width
     * should be comparable to the html used for the final label.
     **/
    int breadcrumbsLen( const BreadcrumbList & breadcrumbs, const QFont & font )
    {
	QString plainText;

	for ( const Breadcrumb & crumb : breadcrumbs )
	{
	    const QString & name = crumb.displayName();
	    if ( !name.isEmpty() )
		plainText += name.endsWith( u'/' ) ? name : name % '/';
	}

	return horizontalAdvance( font, plainText );
    }


    /**
     * Return the longest breadcrumb that can still be elided (ie. more
     * than one character) or 0 if there are no more.  Crumbs with
     * three characters or less cannot (should not) be elided any
     * further.  The first (root) breadcrumb has leading and trailing
     * slashes (might be just "/", but that is less than 3 anyway) that
     * are ignored in this comparison.
     *
     * Note that this isn't exact because it is just picking the most
     * characters rather than an actual text width, but that's good
     * enough for this purpose.
     **/
    Breadcrumb * pickLongBreadcrumb( BreadcrumbList & breadcrumbs )
    {
	int maxLen = 1;
	Breadcrumb * longestCrumb = nullptr;

	for ( Breadcrumb & crumb : breadcrumbs )
	{
	    const int nameLength = crumb.displayName().size();
	    if ( nameLength > maxLen )
	    {
		longestCrumb = &crumb;
		maxLen = nameLength;
	    }
	}

	return longestCrumb;
    }


    /**
     * Set the elidedText of breadcrumbs to "", causing them to be
     * ignored when rendering the final html.  This is the last
     * resort; start at the first crumb and continue until the
     * breadcrumbs will fit in maxLength.
     **/
    void truncateBreadcrumbs( BreadcrumbList & breadcrumbs, const QFont & font, int maxLength )
    {
	for ( Breadcrumb & crumb : breadcrumbs )
	{
	    crumb.elidedName = "";
	    if ( breadcrumbsLen( breadcrumbs, font ) <= maxLength )
		return;
	}
    }


    /**
     * Return an elided copy of 'name', attempting to fit the whole
     * breadcrumb label within maxLength.  Only shorten up to a half
     * in this pass.  Special-case very short (possibly already-
     * elided) names to an ellipsis, to avoid any possible rounding
     * issues and over-shortening, and because it is faster.
     **/
    QString shortenCrumb( const QFont & font, const QString & name, int currentLength, int maxLength )
    {
	if ( name.size() < 4 )
	    return "…"; // ellipsis

	const int crumbLength = horizontalAdvance( font, name );
	const int elideLength = qMax( crumbLength - ( currentLength - maxLength ), crumbLength / 2 );

#if VERBOSE_BREADCRUMBS
	logDebug() << name
	           << " shortened from " << crumbLength
	           << " to " << elideLength << Qt::endl;
#endif

	return elidedText( font, name, elideLength );
    }


    /**
     * Shorten excessively long breadcrumbs so they fit in the main
     * window.
     *
     * This iterative process keeps eliding the longest name by up to a
     * half until the path as a whole fits in the available space.  This
     * avoids excessively shortening one component and leaving others
     * much longer, or eliding to a stump when there is much more space
     * available.  As a last resort, crumbs will be removed (hidden)
     * from the start of the list after all components have been elided
     * down to just an ellipsis.
     **/
    void shortenBreadcrumbs( BreadcrumbList & breadcrumbs, const QFont & font, int maxLength )
    {
	// Keep eliding until the text fits
	int currentLength = breadcrumbsLen( breadcrumbs, font );
	while ( currentLength > maxLength )
	{
	    Breadcrumb * longestCrumb = pickLongBreadcrumb( breadcrumbs );
	    if ( !longestCrumb )
	    {
		// Can't elide any more, chop crumbs completely from the start of the label
		truncateBreadcrumbs( breadcrumbs, font, maxLength );
		return;
	    }

	    // Elide this crumb
	    longestCrumb->elidedName =
		shortenCrumb( font, longestCrumb->displayName(), currentLength, maxLength );

	    currentLength = breadcrumbsLen( breadcrumbs, font );
	}
    }


    /**
     * Generate HTML from a BreadcrumbList.  Carriage returns and
     * linefeeds have already been replaced by spaces, but escape
     * special characters and wrap to prevent whitespace being
     * collapsed.  If severe elision means some components are not
     * being displayed, start with "…/".
     **/
    QString toHtml( const BreadcrumbList & breadcrumbs )
    {
	if ( breadcrumbs.size() == 0 )
	    return QString{};

	const QString crumbTemplate{ "<a href=\"%1\">%2</a>" };

	QString html;

	if ( breadcrumbs.first().displayName() == ""_L1 )
	    html = "…/";

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

	// Loop through the descendants of the root, starting at the leaf level
	while ( depth > 1 )
	{
	    if ( item->isDirInfo() )
	    {
		breadcrumbs[ depth ].pathComponent = replaceCrLf( item->name() );
		breadcrumbs[ depth ].url           = item->debugUrl();
	    }

	    item = item->parent();
	    --depth;
	}

	// Add the root directory as the 2nd crumb
	QString name;
	QString path;
	SysUtil::splitPath( toplevel->name(), path, name );
	breadcrumbs[ 1 ].pathComponent = replaceCrLf( name );
	breadcrumbs[ 1 ].url           = toplevel->debugUrl();

	// Add the whole tree above the root, if any, as the 1st crumb
	if ( path.isEmpty() )
	    breadcrumbs.removeFirst();
	else
	    breadcrumbs[ 0 ].pathComponent = replaceCrLf( path );

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
