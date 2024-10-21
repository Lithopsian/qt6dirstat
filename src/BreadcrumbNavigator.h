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
	Breadcrumb( const QString & path = QString{} ):
	    pathComponent{ path }
	{}

	QString pathComponent;
	QString elidedName;   // Empty or elided version of pathComponent
	QString url;

	const QString & displayName() const
	    { return elidedName.isEmpty() ? pathComponent : elidedName; }

    };	// struct Breadcrumb


    typedef QVector<Breadcrumb> BreadcrumbList;


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

    public:

	/**
	 * Constructor.
	 **/
	BreadcrumbNavigator( QWidget * parent );

	/**
	 * Explicitly clear the path.
	 **/
	void clear() { setPath( nullptr ); }


    signals:

	/**
	 * Notification that the user activated a path.
	 *
	 * Usually this should be connected to some navigation slot to select
	 * the clicked directory in a view.
	 **/
	void pathClicked( const QString & path );


    public slots:

	/**
	 * Set the path from a FileInfo item.
	 *
	 * A null item clears the path.
	 **/
	void setPath( const FileInfo * item );


    private:

	BreadcrumbList _breadcrumbs;

    };	// class BreadcrumbNavigator

}	// namespace QDirStat

#endif	// BreadcrumbNavigator_h
