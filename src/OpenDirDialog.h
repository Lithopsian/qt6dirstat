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
#include <QFileSystemModel>
#include <QHelpEvent>
#include <QStyledItemDelegate>

#include "ui_open-dir-dialog.h"


namespace QDirStat
{
    class ExistingDirValidator;

    /**
     * Dialog to let the user select a directory to read.
     **/
    class OpenDirDialog final : public QDialog
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
	 * 'crossFilesystems' returns whether the "cross filesystems" checkbox is
	 * checked.
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
	 * The path of the directory the user selected.
	 **/
	QString selectedPath() const { return _ui->pathComboBox->currentText(); }

	/**
	 * The "cross filesystems" flag of this dialog (overriding the global
	 * "cross filesystems" setting" from the config dialog).
	 **/
	bool crossFilesystems() const { return _ui->crossFilesystemsCheckBox->isChecked(); }

	/**
	 * Set a path in the dirTree.
	 **/
	void setPath( const QString & path );

	/**
	 * Connect to signals.
	 **/
	void initConnections();


    private:

	std::unique_ptr<Ui::OpenDirDialog> _ui;

	QFileSystemModel     * _filesystemModel;
	ExistingDirValidator * _validator;
	QString                _lastPath;

    };	// class OpenDirDialog



    /**
     * This delegate exists solely to provide a tooltip for
     * elided items in the open directory tree.
     **/
    class OpenDirDelegate final : public QStyledItemDelegate
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	OpenDirDelegate( QObject * parent ):
	    QStyledItemDelegate{ parent }
	{}

	/**
	 * Tooltip event handler.
	 *
	 * The help event handler is called with event type Tooltip
	 * when a tooltip is requested.  If the tree item contents
	 * are wider than the (only) tree section, then a tooltip is
	 * shown with the full item text.
	 *
	 * Reimplemented from QStyledItemDelegate/QAbstractItemDelegate.
	 **/
	bool helpEvent( QHelpEvent                 * event,
	                QAbstractItemView          * view,
	                const QStyleOptionViewItem & option,
	                const QModelIndex          & index ) override;

    };	// class OpenDirDelegate

}	// namespace QDirStat

#endif	// OpenDirDialog_h
