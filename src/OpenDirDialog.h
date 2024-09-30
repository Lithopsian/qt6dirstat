/*
 *   File name: OpenDirDialog.h
 *   Summary:   QDirStat "open directory" dialog
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef OpenDirDialog_h
#define OpenDirDialog_h

#include <memory>

#include <QDialog>

#include "ui_open-dir-dialog.h"


class QFileSystemModel;


namespace QDirStat
{
    class ExistingDirValidator;

    /**
     * Dialog to let the user select installed packages to open, very much like
     * a "get existing directory" dialog, but returning a PkgFilter instead.
     **/
    class OpenDirDialog: public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor.
	 *
	 * Private, use the static askOpenDir() instead.
	 **/
	OpenDirDialog( QWidget * parent, bool crossFilesystems );

	/**
	 * Destructor.
	 **/
	~OpenDirDialog() override;


    public:

	/**
	 * Open an "open directory" dialog, wait for the user to select one and
	 * return that path. If the user cancelled the dialog, this returns an
	 * empty string.
	 *
	 * 'crossFilesystems' (if non-null) returns the "cross filesystems"
	 * flag of the dialog.
	 **/
	static QString askOpenDir( QWidget * parent, bool & crossFilesystems );


    protected slots:

	/**
	 * Set a path in the dirTree and expand (open) that branch.
	 **/
	void pathSelected( const QString & path );

	/**
	 * Set a path in the dirTree and accept the dialog.
	 **/
	void pathDoubleClicked( const QString & path );

	/**
	 * Go up one directory level.
	 **/
	void goUp();

	/**
	 * Notification that the user selected a directory in the tree
	 **/
	void treeSelection( const QModelIndex & newCurrentItem );

	/**
	 * Notification that the user edited a path in the combo box.
	 * 'ok' is the result of the validator's check.
	 **/
	void pathEdited( bool ok );


    protected:

	/**
	 * Write settings to the config file
	 **/
	void writeSettings();

	/**
	 * The path of the directory the user selected.
	 **/
	QString selectedPath() const { return _ui->pathComboBox->currentText(); }

	/**
	 * The "cross filesystems" flag of this dialog (overriding the global
	 * "cross filesystems" setting" from the config dialog).
	 **/
	bool crossFilesystems() const { return _ui->crossFilesystemsCheckBox->isChecked(); }

	/**
	 * Read settings from the config file
	 **/
	void readSettings();

	/**
	 * Set a path in the dirTree.
	 **/
	void setPath( const QString & path );

	/**
	 * Create and apply an ExistingDirValidator, enable the clear
	 * button, and set the current combo-box text in the line edit.
	 **/
	void initPathComboBox();

	/**
	 * Initialise the QFileSystemModel and DirTreeView.
	 **/
	void initDirTree();

	/**
	 * Connect to signals.
	 **/
	void initConnections();

	/**
	 * Populate the path combo-box with a new path.  If the path
	 * is already in the list, then set it as the current item,
	 * otherwise build a new list with the path and all its
	 * ancestors.
	 **/
	void populatePathComboBox( const QModelIndex & currentIndex );


    private:

	std::unique_ptr<Ui::OpenDirDialog> _ui;

	QFileSystemModel     * _filesystemModel;
	ExistingDirValidator * _validator;
	QString                _lastPath;

    };	// class OpenDirDialog

}	// namespace QDirStat

#endif	// OpenDirDialog_h
