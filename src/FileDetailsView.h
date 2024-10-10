/*
 *   File name: FileDetailsView.h
 *   Summary:   Details view for the currently selected file or directory
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileDetailsView_h
#define FileDetailsView_h

#include <memory>

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
	 * Show an empty page.
	 **/
	void clear() { setCurrentPage( _ui->emptyPage ); }

	/**
	 * Show a summary of multiple selected items.  The
	 * counts are of a normalized list, so selected children
	 * of a selected parent are not counted.
	 **/
	void showDetails( const FileInfoSet & selectedItems );

	/**
	 * Show details about a single item: either a package,
	 * a directory, a pseudo-directory, an actual file, or
	 * a summary for the top level of the package view.
	 **/
	void showDetails( FileInfo * fileInfo );

	/**
	 * Return a description of a FileInfo ReadState enum.
	 **/
	static QString readStateMsg( int readState );

	/**
	 * Return the label limit, i.e. the maximum number of characters for
	 * certain fields that can otherwise grow out of bounds.
	 **/
	int labelLimit() const { return _labelLimit; }

	/**
	 * Set the label limit.
	 **/
	void setLabelLimit( int newLimit ) { _labelLimit = newLimit; }


    protected slots:

	/**
	 * Notification that the categories have changed in some way
	 * and we may need to update the panel.  Iy is currently only
	 * relevant for a regular file in the file details page.
	 **/
	void categoriesChanged();


    protected:

	/**
	 * Getter for a raw pointer to the UI object.
	 **/
	const Ui::FileDetailsView * ui() { return _ui.get(); }

	/**
	 * Activate a page of this widget stack. This is similar to
	 * setCurrentWidget(), but it also hides all the other pages to
	 * minimize the screen space requirements: No extra space is reserved
	 * for any of the other pages which might be larger than this one.
	 **/
	void setCurrentPage( QWidget * page );


    private:

	std::unique_ptr<Ui::FileDetailsView> _ui;

	AdaptiveTimer * _pkgUpdateTimer;
	int             _labelLimit{ 0 };

    };	// class FileDetailsView

}	// namespace QDirStat

#endif	// FileDetailsView_h
