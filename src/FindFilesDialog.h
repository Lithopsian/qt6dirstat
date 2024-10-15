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
	 * Getter for a raw pointer to the UI object.
	 **/
	const Ui::FindFilesDialog * ui() const { return _ui.get(); }

	/**
	 * Resize event, reimplemented from QDialog/QWidget.
	 *
	 * This makes a call to showEvent() which re-displays the path
	 * label, elided to the new dialog width.
	 **/
	void resizeEvent( QResizeEvent * event ) override;

	/**
	 * Resize event, reimplemented from QDialog/QWidget.
	 *
	 * This displays the path label after the layouts have been
	 * completed, so that it is elided correctly.
	 **/
	void showEvent( QShowEvent * ) override;


    private:

	std::unique_ptr<Ui::FindFilesDialog> _ui;

    };	// class FindFilesDialog

}	// namespace QDirStat

#endif	// FindFilesDialog_h
