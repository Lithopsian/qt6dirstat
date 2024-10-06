/*
 *   File name: FindFilesDialog.h
 *   Summary:   QDirStat "Find Files" dialog
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FindFilesDialog_h
#define FindFilesDialog_h

#include <memory>

#include <QDialog>

#include "ui_find-files-dialog.h"


namespace QDirStat
{
    class FileSearchFilter;

    /**
     * Model dialog for searching files in the scanned directory tree.
     **/
    class FindFilesDialog: public QDialog
    {
	Q_OBJECT

	/**
	 * Private constructor.
	 *
	 * Use the static method askFindFiles for access.
	 **/
	FindFilesDialog( QWidget * parent, const QString & pattern );

	/**
	 * Destructor.  Saves the window geometry.  askFindFiles() creates
	 * the dialog on the stack, so it is always destroyed when it is
	 * closed.
	 **/
	~FindFilesDialog() override;


    public:

	/**
	 * Open a find files dialog and execute a search if the dialog is
	 * accepted.  The dialog is modal and blocking; this function
	 * does not return until the dialog has been closed and any search
	 * has been run.
	 **/
	static void askFindFiles( QWidget * parent = nullptr );


    protected:

	/**
	 * Return a file search filter coresponding to the values entered
	 * in the dialog.
	 **/
	FileSearchFilter fileSearchFilter();

	/**
	 * Read settings from the config file.  All the dialog fields
	 * except for 'pattern' are saved.  The 'pattern' string is
	 * remembered in local static storage only for as long as the
	 * program is open.  The window size and position are also
	 * loaded from Settings.
	 **/
	void readSettings();

	/**
	 * Write settings to the config file.  The dialog fields are
	 * written to Settings only if the dialog is accepted.  The
	 * window geometry is always saved when the dialog is destroyed.
	 **/
	void writeSettings();

	/**
	 * Resize event, reimplemented from QWidget.
	 *
	 * Elide the label to fit inside the current dialog width, so that
	 * it fills the available width but very long subtree paths don't
	 * stretch the dialog.  A little extra room is left for the user to
	 * shrink the dialog, which would then force the label to be elided
	 * further.
	 **/
	void resizeEvent( QResizeEvent * ) override;


    private:

	std::unique_ptr<Ui::FindFilesDialog> _ui;

    };	// class FindFilesDialog

}	// namespace QDirStat

#endif	// FindFilesDialog_h
