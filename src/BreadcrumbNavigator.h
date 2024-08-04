/*
 *   File name: BreadcrumbNavigator.h
 *   Summary:   Breadcrumb widget for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef BreadcrumbNavigator_h
#define BreadcrumbNavigator_h

#include <QLabel>
#include <QVector>


namespace QDirStat
{
    class FileInfo;

    /**
     * Helper class to represent one single breadcrumb
     **/
    struct Breadcrumb
    {
        Breadcrumb( const QString & path = QString() ):
            pathComponent { path }
	{}

        QString pathComponent;
        QString displayName;   // This may be shortened
        QString url;
    };


    /**
     * Widget for "breadcrumb" navigation in a directory tree:
     *
     * Show the current path with clickable components so the user can easily
     * see where in the tree the currently selected item is and can easily
     * navigate upwards in the tree.
     *
     * Each component is an individual hyperlink.
     *
     * Upwards navigation is limited to the root of the directory tree,
     * i.e. the user can only navigate inside the current tree.
     *
     * This widget does not hang on to any FileInfo or DirTree object; once a
     * current path is set, it deals only with strings internally.
     **/
    class BreadcrumbNavigator: public QLabel
    {
	Q_OBJECT

	typedef QVector<Breadcrumb> BreadcrumbList;

    public:

	/**
	 * Constructor.
	 **/
	BreadcrumbNavigator( QWidget * parent = nullptr );

	/**
	 * Explicitly clear the path.
	 **/
	void clear() { setPath( nullptr ); }


    public slots:

	/**
	 * Set the path from a FileInfo item.
         * A null item clears the path.
	 **/
	void setPath( const FileInfo * item );


    signals:

	/**
	 * Notification that the user activated a path.
	 *
	 * Usually this should be connected to some navigation slot to select
	 * the clicked directory in a view.
	 **/
	void pathClicked( const QString & path );


    protected:

        /**
         * Return the total display length of all breadcrumbs plus delimiters.
         **/
        int breadcrumbsLen() const;

        /**
         * Fill the internal _breadcrumbs with content by traversing up the
         * tree from 'item' to the toplevel.
         **/
        void fillBreadcrumbs( const FileInfo * item );

        /**
         * Generate HTML from _breadcrumbs
         **/
        QString html() const;

        /**
         * Shorten exessively long _breadcrumbs so they have a better chance to
         * fit on the screen.
         **/
        void shortenBreadcrumbs();

        /**
         * Return the longest breadcrumb that has not been
         * shortened yet or 0 if there are no more.
         **/
        Breadcrumb * pickLongBreadcrumb();

        /**
         * Write the internal _breadcrumbs to the log.
         **/
        void logBreadcrumbs() const;


    private:

        //
        // Data members
        //

        BreadcrumbList _breadcrumbs;

    }; // class BreadcrumbNavigator

} // namespace QDirStat

#endif // BreadcrumbNavigator_h
