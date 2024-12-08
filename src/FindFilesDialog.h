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
    class FindFilesDialog final : public QDialog
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
	 * Getter for a raw pointer to the UI object.
	 **/
	const Ui::FindFilesDialog * ui() const { return _ui.get(); }

	/**
	 * Event handler, reimplemented from QDialog/QWidget.
	 *
	 * This detects resize events, font change events, and show events.
	 * Resize events are ignored if the old size was not valid as this
	 * means the layouts have not been done yet.  The path label is
	 * elided to fit the dialog width and displayed.
	 **/
	bool event( QEvent * event ) override;


    private:

	std::unique_ptr<Ui::FindFilesDialog> _ui;

    };	// class FindFilesDialog

}	// namespace QDirStat

#endif	// FindFilesDialog_h
