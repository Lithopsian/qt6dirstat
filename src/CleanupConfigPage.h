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

#include "ListEditor.h"
#include "ui_cleanup-config-page.h"


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
	void enableWidgets();


    protected:

	/**
	 * Fill the cleanup list widget from the cleanup collection.
	 *
	 * Implemented from ListEditor.
	 **/
	void fillListWidget() override;

	/**
	 * Save the contents of the widgets to the specified value.
	 *
	 * Implemented from ListEditor.
	 **/
	void save( void * value ) override;

	/**
	 * Load the content of the widgets from the specified value.
	 *
	 * Implemented from ListEditor.
	 **/
	void load( void * value ) override;

	/**
	 * Create a new value with default values.
	 * This is called when the 'Add' button is clicked.
	 *
	 * Implemented from ListEditor.
	 **/
	void * createValue() override;

	/**
	 * Remove a value from the internal list and delete it.
	 *
	 * This is called when the 'Remove' button is clicked.
	 *
	 * Implemented from ListEditor.
	 **/
	void removeValue( void * ) override;

	/**
	 * Return a text for the list item of 'value'.
	 *
	 * Implemented from ListEditor.
	 **/

	QString valueText( void * value ) override;

	/**
	 * Enable or disable all the edit widgets on the right side
	 * of the splitter.
	 **/
	void enableEditWidgets( bool enable );


    private:

	//
	// Data members
	//

	std::unique_ptr<Ui::CleanupConfigPage> _ui;

	int _outputWindowDefaultTimeout;

    };	// class CleanupConfigPage

}	// namespace QDirStat

#endif	// CleanupConfigPage_h
