/*
 *   File name: OpenDirDialog.h
 *   Summary:	QDirStat "open directory" dialog
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef OpenDirDialog_h
#define OpenDirDialog_h

#include <QDialog>
#include <QModelIndex>

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

    public:
	/**
	 * Open an "open directory" dialog, wait for the user to select one and
         * return that path. If the user cancelled the dialog, this returns an
         * empty string.
         *
         * 'crossFilesystems' (if non-null) returns the "cross filesystems"
         * flag of the dialog.
	 **/
	static QString askOpenDir( QWidget * parent, bool * crossFilesystems );


    protected:
	/**
	 * Internal constructor.
	 *
	 * Use the static askOpenDir() instead.
	 **/
	OpenDirDialog( QWidget * parent, bool crossFilesystems );

	/**
	 * Destructor.
	 **/
	~OpenDirDialog() override;

	/**
	 * The path of the directory the user selected.
	 **/
        QString selectedPath() const;

        /**
         * The "cross filesystems" flag of this dialog (overriding the global
         * "cross filesystems" setting" from the config dialog).
         **/
        bool crossFilesystems() const { return _ui->crossFilesystemsCheckBox->isChecked(); }

        /**
         * Return this dialog's path selector so it can be populated.
         **/
        PathSelector * pathSelector() const { return _ui->pathSelector; }

	/**
	 * Read settings from the config file
	 **/
	void readSettings();

        /**
         * Set a path in the dirTree.
         **/
        void setPath( const QString & path );


    protected slots:

        /**
         * Set a path in the dirTree and expand (open) that branch.
         **/
        void setPathAndExpand( const QString & path );

        /**
         * Set a path in the dirTree and accept the dialog.
         **/
        void setPathAndAccept( const QString & path );

        /**
         * Go up one directory level.
         **/
        void goUp();

	/**
	 * Write settings to the config file
	 **/
	void writeSettings();

       /**
         * Notification that the user selected a directory in the tree
         **/
        void treeSelection( const QModelIndex & newCurrentItem,
                            const QModelIndex & oldCurrentItem );

        /**
         * Notification that the user edited a path in the combo box.
         * 'ok' is the result of the validator's check.
         **/
        void pathEdited( bool ok );

        /**
         * Select a directory once everything is initialized and the first
         * signals are processed.
         **/
        void initialSelection();


    protected:

        void initPathComboBox();
        void initDirTree();
        void initConnections();

        void populatePathComboBox( const QString & path );


    private:

	Ui::OpenDirDialog *     _ui;
        QFileSystemModel *      _filesystemModel;
	QPushButton *           _okButton;
        ExistingDirValidator *  _validator;
        bool                    _settingPath { false };
        QString                 _lastPath;

    };	// class OpenDirDialog

}	// namespace QDirStat

#endif	// OpenDirDialog_h
