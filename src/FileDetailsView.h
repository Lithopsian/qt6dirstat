/*
 *   File name: FileDetailsView.h
 *   Summary:	Details view for the currently selected file or directory
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#ifndef FileDetailsView_h
#define FileDetailsView_h

#include <QStackedWidget>

#include "ui_file-details-view.h"


namespace QDirStat
{
    class AdaptiveTimer;
    class DirInfo;
    class FileInfo;
    class FileInfoSet;
    class PkgInfo;

    /**
     * Details view for the current selection (file, directory, multiple
     * objects).
     *
     * This shows detailed information such as sizes, permissions, mtime
     * etc. depending on what type of object and how many of them are selected.
     **/
    class FileDetailsView: public QStackedWidget
    {
	Q_OBJECT

    public:

	/**
	 * Constructor
	 **/
	FileDetailsView( QWidget * parent = nullptr );

	/**
	 * Destructor
	 **/
	~FileDetailsView() override;

	/**
	 * Show an empty page.
	 **/
	void clear();

	/**
	 * Show a summary of multiple selected items.
	 **/
	void showDetails( const FileInfoSet & selectedItems );

	/**
	 * Show details about a "file": either a package, a
	 * directory, a pseudo-directory, an actual file, or a
	 * summary for the top level of the package view.
	 **/
	void showDetails( FileInfo * fileInfo );

	/**
	 * Return a description of a FileInfo ReadState enum.
	 **/
	static QString readStateMsg( int readState );


    protected slots:

	/**
	 * Notification that the categories have changed in some way
	 * and we may need to update the panel.
	 **/
	void categoriesChanged();


    protected:

	/**
	 * Update package information via the AdaptiveTimer.
	 **/
	void updatePkgInfo( const QString & path );

	/**
	 * Show details about a directory.
	 **/
	void showDetails( DirInfo * dirInfo );

	/**
	 * Show details about a package.
	 **/
	void showDetails( PkgInfo * pkgInfo );

	/**
	 * Show a summary of the current selection.
	 **/
//	void showSelectionSummary( const FileInfoSet & selectedItems );

	/**
	 * Show the packages summary (pkg:/).
	 **/
	void showPkgSummary( PkgInfo * pkgInfo );

	/**
	 * Return the label limit, i.e. the maximum number of characters for
	 * certain fields that can otherwise grow out of bounds.
	 **/
	int labelLimit() const { return _labelLimit; }

	/**
	 * Set the label limit. Notice that if a label needs to be limited, it
	 * will get three characters less than this value to compensate for the
	 * "..." ellipsis that indicates that it was cut off.
	 **/
	void setLabelLimit( int newLimit ) { _labelLimit = newLimit; }

	/**
	 * Activate a page of this widget stack. This is similar to
	 * setCurrentWidget(), but it also hides all the other pages to
	 * minimize the screen space requirements: No extra space is reserved
	 * for any of the other pages which might be larger than this one.
	 **/
	void setCurrentPage( QWidget *page );

	/**
	 * Read parameters from settings file.
	 **/
//	void readSettings();

	/**
	 * Write parameters to settings file.
	 **/
//	void writeSettings();

	/**
	 * Return the MIME category of a file.
	 **/
	static const QString & mimeCategory( const FileInfo * fileInfo );

	/**
	 * Sets the label to the category for this file, and enables
	 * the caption if a category is found.
	 **/
	void setMimeCategory( const FileInfo * fileInfo );

	/**
	 * Return the path of a fileInfo's parent directory.
	 **/
//	static QString parentPath( const FileInfo * fileInfo );

	/**
	 * Set a label with a number and an optional prefix.
	 **/
	static void setLabel( QLabel * label, int number, const QString & prefix = "" );

	/**
	 * Set a file size label with a file size and an optional prefix.
	 **/
	static void setLabel( FileSizeLabel * label, FileSize size, const QString & prefix = "" );

	/**
	 * Set a label with a text of limited size.
	 **/
	void setLabelLimited( QLabel * label, const QString & text );

	/**
	 * Return a stylesheet string to set a label text to the configured
	 * directory read error colour.
	 **/
	QString errorStyleSheet() const;

	/**
	 * Return a stylesheet string to set the color of the directory permissions label.
	 **/
	QString dirColorStyle( const DirInfo * dir ) const;

	// Boilerplate widget setting methods

	static QString subtreeMsg( const DirInfo * dir );
//	static QString pkgMsg( const PkgInfo * pkg );

	void showFileInfo( FileInfo * file );
	void showFilePkgInfo( const FileInfo * file );
	void showSubtreeInfo( DirInfo * dir );
	void showDirNodeInfo( const DirInfo * dir );

	void setSystemFileWarningVisibility( bool visible );
	void setFilePkgBlockVisibility( bool visible );
	void setDirBlockVisibility( bool visible );

	QString formatFilesystemObjectType( const FileInfo * file );


    private:

	// Data members

	Ui::FileDetailsView * _ui;
	AdaptiveTimer	    * _pkgUpdateTimer;
	int		      _labelLimit;
//	QColor		      _dirReadErrColor; // now using the theme-dependant value from the tree model

    };	// class FileDetailsView
}	// namespace QDirStat

#endif // FileDetailsView_h