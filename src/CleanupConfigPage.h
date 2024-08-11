/*
 *   File name: CleanupConfigPage.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef CleanupConfigPage_h
#define CleanupConfigPage_h

#include <memory>

#include "ListEditor.h"
#include "ui_cleanup-config-page.h"


class QListWidget;


namespace QDirStat
{
    class Cleanup;
    class ConfigDialog;

    /**
     * Configuration page (tab) for cleanups:
     * Edit, add, delete, reorder cleanups in the cleanup collection.
     **/
    class CleanupConfigPage: public ListEditor
    {
	Q_OBJECT

    public:

	CleanupConfigPage( ConfigDialog * parent );
	~CleanupConfigPage() override;


    protected slots:

	/**
	 * Create a new list item.  Overload of ListEditor::add() to allow
	 * detection of new insertions so that focus can be put in the only
	 * sensible place.
	 **/
	void add() override;

	/**
	 * Write changes back to the settings.
	 **/
	void applyChanges();

	/**
	 * Notification that the user changed the "Title" field of the
	 * current cleanup.
	 **/
	void titleChanged( const QString & newTitle );

	/**
	 * Enable or disable the outputwindow widgets based on the settings
	 * of the refresh policy combo and default timeout checkbox.
	 **/
	void enableWindowPolicyWidgets();


    protected:

	/**
	 * Return the list widget for this class (ie.notebook page).
	 *
	 * Reimplemented from ListEditor.
	 **/
	QListWidget * listWidget() const override { return _ui->listWidget; };

	/**
	 * Returns the corresponding tool button.
	 *
	 * Reimplemented from ListEditor.  The default implementations
	 * return 0.
	 **/
	QToolButton * toTopButton()    const override { return _ui->toTopButton;    };
	QToolButton * moveUpButton()   const override { return _ui->moveUpButton;   };
	QToolButton * addButton()      const override { return _ui->addButton;      };
	QToolButton * removeButton()   const override { return _ui->removeButton;   };
	QToolButton * moveDownButton() const override { return _ui->moveDownButton; };
	QToolButton * toBottomButton() const override { return _ui->toBottomButton; };

	/**
	 * Fill the cleanup list widget from the cleanup collection.
	 *
	 * Reimplemented from ListEditor.
	 **/
	void fillListWidget() override;

	/**
	 * Save the contents of the widgets to the specified value.
	 *
	 * Reimplemented from ListEditor.
	 **/
	void save( void * value ) override;

	/**
	 * Load the content of the widgets from the specified value.
	 *
	 * Reimplemented from ListEditor.
	 **/
	void load( void * value ) override;

	/**
	 * Create a new value with default values.
	 * This is called when the 'Add' button is clicked.
	 *
	 * Reimplemented from ListEditor.
	 **/
	void * newValue() override;

	/**
	 * Delete a value from the internal list.
	 *
	 * This is called when the 'Remove' button is clicked.
	 *
	 * Reimplemented from ListEditor.
	 **/
	void deleteValue( void * ) override;

	/**
	 * Return a text for the list item of 'value'.
	 *
	 * Reimplemented from ListEditor.
	 **/

	QString valueText( void * value ) override;

	/**
	 * Enable or disable all the edit widgets on the right side
	 * of the splitter.
	 **/
	void enableEditWidgets( bool enable )
	    { _ui->rightColumnWidget->setEnabled( enable ); }


    private:

	//
	// Data members
	//

	std::unique_ptr<Ui::CleanupConfigPage> _ui;

	int _outputWindowDefaultTimeout;

    };	// class CleanupConfigPage

}	// namespace QDirStat

#endif	// CleanupConfigPage_h
