/*
 *   File name: PathSelector.h
 *   Summary:   Path selection list widget for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PathSelector_h
#define PathSelector_h

#include <QListWidget>
#include <QFileIconProvider>


namespace QDirStat
{
    class MountPoint;
    class PathSelectorItem;

    /**
     * List widget for selecting a path, very much like the common "places"
     * list in file selection boxes. This widget also supports mount points
     * (see MountPoints.h) with more information than just the path.
     **/
    class PathSelector: public QListWidget
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 * Don't forget to populate the widget after creating it!
	 **/
	PathSelector( QWidget * parent = nullptr );

	/**
	 * Convenience function: Add the current user's home directory.
	 **/
	void addHomeDir();

	/**
	 * Add a list of mount points with the appropriate icons.
	 **/
	void addMountPoints( const QList<MountPoint *> & mountPoints );

	/**
	 * Select the item representing the parent mount point of 'path'.
	 **/
//	void selectParentMountPoint( const QString & path );


    signals:

	/**
	 * Emitted when the user selects one of the paths.
	 *
	 * Do not use any of the inherited QListWidget signals; the returned
	 * QListWidgetItem might have a multi-line text from which you would
	 * have to extract the path.
	 **/
	void pathSelected( const QString & path );

	/**
	 * Emitted when the user double-clicks a path.
	 **/
	void pathDoubleClicked( const QString & path );


    protected slots:

	/**
	 * Signal forwarder to translate a selected item into a path.
	 **/
	void slotItemSelected( const QListWidgetItem * widgetItem );

	/**
	 * Signal forwarder to translate a selected item into a path.
	 **/
	void slotItemDoubleClicked( const QListWidgetItem * widgetItem );


    protected:

	/**
	 * Add a path with the specified icon.
	 *
	 * Don't use this for mount points; use addMountPoint() instead which
	 * displays more information.
	 **/
	PathSelectorItem * addPath( const QString & path,
				    const QIcon   & icon = QIcon() );

	/**
	 * Add a mount point with an appropriate icon.
	 *
	 * This displays not only the mount point's path, but also some
	 * additional information like the filesystem type and, if available,
	 * the size of the partition / filesystem.
	 **/
	void addMountPoint( MountPoint * mountPoint );

	/**
	 * Add a mount point with the specified icon.
	 * Notice that you can also specify QIcon() to suppress any icon.
	 **/
//	PathSelectorItem * addMountPoint( MountPoint  * mountPoint,
//					  const QIcon & icon );


    private:

	QFileIconProvider _iconProvider;

    };	// class PathSelector




    /**
     * Item for a PathSelector widget.
     **/
    class PathSelectorItem: public QListWidgetItem
    {
    public:
	/**
	 * Constructor for a simple path list item.
	 * Use QListWidgetItem::setIcon() to set an icon.
	 **/
	PathSelectorItem( const QString & path,
			  PathSelector  * parent = nullptr ):
	    QListWidgetItem ( path, parent ),
	    _path { path }
	{}

	/**
	 * Constructor for a mount point list item.
	 * Use QListWidgetItem::setIcon() to set an icon.
	 **/
	PathSelectorItem( MountPoint   * mountPoint,
			  PathSelector * parent = nullptr );

	/**
	 * Return the path for this item.
	 **/
	const QString & path() const { return _path; }


    private:

	QString      _path;
    };

}	// namespace QDirStat

#endif	// PathSelector_h
