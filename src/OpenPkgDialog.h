/*
 *   File name: OpenPkgDialog.h
 *   Summary:   QDirStat "open installed packages" dialog
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef OpenPkgDialog_h
#define OpenPkgDialog_h

#include <memory>

#include <QDialog>

#include "ui_open-pkg-dialog.h"
#include "PkgFilter.h"


namespace QDirStat
{
    /**
     * Dialog to let the user select installed packages to open, very much like
     * a "get existing directory" dialog, but returning a PkgFilter instead.
     **/
    class OpenPkgDialog: public QDialog
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 *
	 * Consider using the static methods instead.
	 **/
	OpenPkgDialog( QWidget * parent = nullptr );

	/**
	 * Destructor.
	 **/
	~OpenPkgDialog() override;

	/**
	 * Open an "open package" dialog and wait for the user to enter
	 * values.
         *
         * 'cancelled_ret' is a return parameter that (if non-null) is set to
	 * 'true' if the user cancelled the dialog.
	 **/
	static PkgFilter askPkgFilter( bool    * cancelled_ret,
	                               QWidget * parent = nullptr   );

	/**
	 * The package filter the user entered.
	 **/
	PkgFilter pkgFilter();


    protected slots:

	/**
	 * To select the filter radio button if the user enters a pattern.
	 **/
	void textEdited();


    private:

	std::unique_ptr<Ui::OpenPkgDialog> _ui;

    };	// class OpenPkgDialog

}	// namespace QDirStat

#endif	// OpenPkgDialog_h
