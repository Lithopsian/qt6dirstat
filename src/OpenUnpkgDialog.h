/*
 *   File name: OpenUnpkgDialog.h
 *   Summary:	QDirStat "show unpackaged files" dialog
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#ifndef OpenUnpkgDialog_h
#define OpenUnpkgDialog_h


#include <QDialog>
#include <QString>
#include <QStringList>

#include "ui_open-unpkg-dialog.h"
#include "UnpkgSettings.h"


class QContextMenuEvent;


namespace QDirStat
{
    /**
     * Dialog to let the user select parameters for showing unpackaged
     * files. This is very much like a "get existing directory" dialog with
     * some more widgets.
     *
     * Usage:
     *
     *	   OpenUnpkgDialog dialog( this );
     *
     *	   if ( dialog.exec() == QDialog::Accepted )
     *	   {
     *	       QString dir = dialog.startingDir();
     *	       QStringList excludeDirs = dialog.excludeDirs();
     *
     *	       readUnpkgFiles( dir, excludeDirs );
     *	   }
     **/
    class OpenUnpkgDialog: public QDialog
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
	 * Get the starting directory from the dialog's widgets or an empty
	 * string if the dialog was cancelled.
	 **/
	QString startingDir() const;

	/**
	 * Get the directories to exclude from the dialog's widgets.
	 **/
	QStringList excludeDirs() const { return cleanedLines( _ui->excludeDirsTextEdit ); }

	/**
	 * Get the wildcard patterns of files to ignore from the dialog's
	 * widgets.
	 **/
	QStringList ignorePatterns() const { return cleanedLines( _ui->ignorePatternsTextEdit ); }

	/**
	 * Get whether the cross filesystems checkbox is checked,
	 **/
	bool crossFilesystems() const { return _ui->crossFilesystemsCheckBox->isChecked(); }

	/**
	 * Read settings from the config file
	 **/
	void readSettings();

        /**
         * Set all values at once.
         **/
        void setValues( const UnpkgSettings & settings );

	/**
	 * Get the content of a QPlainTextEdit widget as QStringList with
	 * leading and trailing whitespace removed from each line and without
	 * empty lines.
	 **/
	QStringList cleanedLines( QPlainTextEdit * widget ) const;


    private:

	// Data members

	Ui::OpenUnpkgDialog * _ui;
	QPushButton         * _okButton;

    };	// class OpenUnpkgDialog

}	// namespace QDirStat

#endif	// OpenUnpkgDialog_h
