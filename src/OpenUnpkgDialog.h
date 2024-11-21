/*
 *   File name: OpenUnpkgDialog.h
 *   Summary:   QDirStat "show unpackaged files" dialog
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef OpenUnpkgDialog_h
#define OpenUnpkgDialog_h

#include <memory>

#include <QContextMenuEvent>
#include <QDialog>

#include "ui_open-unpkg-dialog.h"
#include "UnpkgSettings.h"


namespace QDirStat
{
    /**
     * Dialog to let the user select parameters for showing unpackaged
     * files. This is very much like a "get existing directory" dialog with
     * some more widgets.  There is a dedicated UnpkgSettings class for
     * getting and saving the dialog values.  The starting directory,
     * exclude directory patterns, ignore file patterns, and read cross-
     * filesystems flag are saved to the config file and reloaded next
     * time the dialog is opened.
     *
     * Usage:
     *
     *	   OpenUnpkgDialog dialog( this );
     *
     *	   if ( dialog.exec() == QDialog::Accepted )
     *	   {
     *	       UnpkgSettings unpkgSettings = dialog.values();
     *
     *	       readUnpkgFiles( unpkgSettings );
     *	   }
     **/
    class OpenUnpkgDialog final : public QDialog
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	OpenUnpkgDialog( QWidget * parent = nullptr );

	/**
	 * Destructor.
	 **/
	~OpenUnpkgDialog() override;

	/**
	 * Get all values from the widgets at once.
	 **/
	UnpkgSettings values() const;


    protected slots:

	/**
	 * Write settings to the config file
	 **/
	void writeSettings();

	/**
	 * Reset the values in the dialog to the default values.
	 **/
	void restoreDefaults();


    protected:

	/**
	 * Read settings from the config file
	 **/
	void readSettings();

	/**
	 * Set all values at once.
	 **/
	void setValues( const UnpkgSettings & settings );


    private:

	std::unique_ptr<Ui::OpenUnpkgDialog> _ui;

    };	// class OpenUnpkgDialog

}	// namespace QDirStat

#endif	// OpenUnpkgDialog_h
