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
    class FileInfo;

    /**
     * Details view for the current selection (file, directory, package, or
     * multiple objects).
     *
     * This shows detailed information such as sizes, permissions, mtime,
     * etc., depending on what type of object and how many of them are selected.
     **/
    class FileDetailsView final : public QStackedWidget
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
	void clear()
	    { setCurrentPage( _ui->emptyPage ); }

	/**
	 * Show details about the current selection in the panel.
	 **/
	void showDetails();

	/**
	 * Return a description of a FileInfo ReadState enum.
	 **/
	static QString readStateMsg( int readState );

	/**
	 * Return whether to elide the panel labels to fit the
	 * current width.
	 **/
	bool elideToFit() const { return _lastPixel >= 0; }

	/**
	 * Change whether to elide paths to 'elide' and re-display
	 * the panel with the new setting.
	 **/
	void setElideToFit( bool elide )
	    { _lastPixel = elide ? 0 : -1; elide ? resizeEvent( nullptr ) : showDetails(); }


    protected slots:

	/**
	 * Notification that the categories have changed in some way
	 * and we may need to update the panel.  It is currently only
	 * relevant for a regular file in the file details page.
	 **/
	void categoriesChanged();


    protected:

	/**
	 * Getter for a raw pointer to the UI object.
	 **/
	const Ui::FileDetailsView * ui() const { return _ui.get(); }

	/**
	 * Show details about a single item: either a package,
	 * a directory, a pseudo-directory, an actual file, or
	 * a summary for the top level of the package view.
	 **/
	void showDetails( FileInfo * fileInfo );

	/**
	 * Activate a page of this widget stack. This is similar to
	 * setCurrentWidget(), but it also hides all the other pages to
	 * minimize the screen space requirements: no extra space is
	 * reserved for any of the other pages which might be larger
	 * than this one.
	 **/
	void setCurrentPage( QWidget * page );

	/**
	 * Detect theme changes.  Font and palette changes result in the
	 * panel being repopulated.  Only a minority of pages are
	 * affected (only one by palette changes) but these events will
	 * be rare and the effort of trying to match particular change
	 * types to what is being displayed is likely to cause more harm
	 * than just always doing a complete refresh.
	 *
	 * Reimplemented from QWidget.
	 **/
	void changeEvent( QEvent * event ) override;

	/**
	 * Re-calculate the last (right-hand) pixel of the contents area
	 * of the panel if '_lastPixel' >= 0.  Otherwise, leave it as -1
	 * indicating that path labels should not be elided.  Then
	 * re-display the panel with the current selection.
	 *
	 * Reimplemented from QFrame/QWidget.
	 **/
	void resizeEvent( QResizeEvent * ) override;


    private:

	std::unique_ptr<Ui::FileDetailsView> _ui;

	AdaptiveTimer * _pkgUpdateTimer;
	int             _lastPixel{ 0 };

    };	// class FileDetailsView

}	// namespace QDirStat

#endif	// FileDetailsView_h
