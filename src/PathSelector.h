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

    /**
     * Item for a PathSelector widget.
     **/
    class PathSelectorItem final : public QListWidgetItem
    {
    public:

	/**
	 * Constructor for a simple path list item.
	 * Use QListWidgetItem::setIcon() to set an icon.
	 **/
	PathSelectorItem( const QString & path,
	                  QListWidget   * parent = nullptr ):
	    QListWidgetItem{ path, parent },
	    _path{ path }
	{}

	/**
	 * Constructor for a mount point list item.
	 *
	 * Use QListWidgetItem::setIcon() to set an icon.
	 **/
	PathSelectorItem( MountPoint  * mountPoint,
	                  QListWidget * parent = nullptr );

	/**
	 * Return the path for this item.
	 **/
	const QString & path() const { return _path; }


    private:

	QString _path;

    };	// class PathSelectorItem



    /**
     * List widget for selecting a path, very much like the common "places"
     * list in file selection boxes. This widget also supports mount points
     * (see MountPoints.h) with more information than just the path.
     **/
    class PathSelector final : public QListWidget
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.  Creates an empty list widget.
	 **/
	PathSelector( QWidget * parent = nullptr );

	/**
	 * Convenience function: add the current user's home directory.
	 **/
	void addHomeDir();

	/**
	 * Add all the normal (non-pseudo, etc.) mount points with the
	 * appropriate icons.
	 **/
	void addNormalMountPoints();


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

    };	// class PathSelector

}	// namespace QDirStat

#endif	// PathSelector_h
