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

#include <QDialog>

#include "ui_find-files-dialog.h"


namespace QDirStat
{
    class DirInfo;
    class FileSearchFilter;

    /**
     * Dialog for searching files in the scanned directory tree.
     **/
    class FindFilesDialog: public QDialog
    {
	Q_OBJECT

    protected:
	/**
	 * Internal constructor.
	 *
	 * Use the static method askFindFiles for access.
	 **/
	FindFilesDialog( QWidget * parent = nullptr );

	/**
	 * Destructor.
	 **/
	~FindFilesDialog() override;

    public:
	/**
	 * Open an "open package" dialog and wait for the user to enter
	 * values.
         *
         * 'cancelled_ret' is a return parameter that (if non-null) is set to
	 * 'true' if the user cancelled the dialog.
	 **/
	static FileSearchFilter askFindFiles( bool    * cancelled_ret,
                                              QWidget * parent = nullptr   );

    protected slots:

        /**
         * Save the values of the widgets to the settings file or
         * to internal static variables.
         **/
        void saveValues();


    protected:

        /**
         * Load values for all widgets from the settings / the config file or
         * from internal static variables
         **/
        void loadValues();

	/**
	 * The package filter the user entered.
	 **/
	FileSearchFilter fileSearchFilter();

	/**
	 * Read settings from the config file
	 **/
	void readSettings();

	/**
	 * Write settings to the config file
	 **/
	void writeSettings();

        /**
         * Return the currently selected subtree if a directory is selected
         * or 0 if not.
         **/
        DirInfo * currentSubtree();

        /**
         * Resize event, reimplemented from QWidget.
	 *
	 * Elide the label to fit inside the current dialog width, so that
	 * they fill the available width but very long subtree paths don't
	 * stretch the dialog.  A little extra room is left for the user to
	 * shrink the dialog, which would then force the label to be elided
	 * further.
         **/
        void resizeEvent( QResizeEvent * event ) override;


    private:

        //
        // Data members
        //

	Ui::FindFilesDialog * _ui;

	static QString _lastPattern;
	static QString _lastPath;

    };	// class FindFilesDialog

}	// namespace QDirStat

#endif	// FindFilesDialog_h
