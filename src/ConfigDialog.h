/*
 *   File name: ConfigDialog.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef ConfigDialog_h
#define ConfigDialog_h

#include <QDialog>
#include "ui_config-dialog.h"


namespace QDirStat
{
    /**
     * Configuration dialog for QDirStat.
     *
     * This class is only the wrapper for the individual config pages; it
     * maintains the tab widget to switch between the pages (the tabs) and the
     * dialog buttons ("OK", "Apply", "Cancel").
     *
     * Each page is pretty much self-sufficient.
     **/
    class ConfigDialog: public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor. Create the dialog and all pages.
	 **/
	ConfigDialog( QWidget * parent );

	/**
	 * Destructor.
	 **/
	~ConfigDialog() override;

	/**
	 * Static method for using one shared instance of this class between
	 * multiple parts of the application. This will create a new instance
	 * if there is none yet (or any more).
	 **/
	static ConfigDialog * sharedInstance( QWidget * parent );


    public:

	/**
	 * Convenience function for creating, if necessary, and showing the
	 * settings dialog window.
	 **/
	static void showSharedInstance( QWidget * parent );


    protected slots:

	/**
	 * Accept the dialog contents, i.e. the user clicked the "OK" button.
	 *
	 * Reimplemented from QDialog.
	 **/
	void accept() override;

	/**
	 * Reject the dialog contents, i.e. the user clicked the "Cancel"
	 * button.
	 *
	 * Reimplemented from QDialog.
	 **/
	void reject() override;


    signals:

	/**
	 * Emitted when the "OK" or the "Apply" button was clicked:
	 *
	 * This is the signal to apply all changes to the settings and/or the
	 * widgets.
	 **/
	void applyChanges();

	/**
	 * Emitted when the "Cancel" button was clicked:
	 *
	 * This is the signal to discard all changes.  Currently, none of the
	 * config pages require this signal.
	 **/
//	void discardChanges();


    private:

	//
	// Data members
	//

	Ui::ConfigDialog       * _ui;

    };	// class ConfigDialog

}	// namespace QDirStat

#endif	// ConfigDialog_h
