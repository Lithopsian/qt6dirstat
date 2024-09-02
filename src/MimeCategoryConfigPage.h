/*
 *   File name: MimeCategoryConfigPage.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef MimeCategoryConfigPage_h
#define MimeCategoryConfigPage_h

#include <memory>

#include "ListEditor.h"
#include "ui_mime-category-config-page.h"


class QListWidget;


namespace QDirStat
{
    class ConfigDialog;

    /**
     * Configuration page (tab) for MimeCategories:
     * Edit, add, delete categories in the MimeCategorizer.
     * A working set of new MimeCategory objects is copied from the
     * live categories and used to populate the list widget.
     **/
    class MimeCategoryConfigPage: public ListEditor
    {
	Q_OBJECT

    public:

	MimeCategoryConfigPage( ConfigDialog * parent );
	~MimeCategoryConfigPage() override;


    protected slots:

	/**
	 * Write changes back to the settings.
	 **/
	void applyChanges();

	/**
	 * Create a new list item.  Overload of ListEditor::add() to allow
	 * detection of new insertions for sorting and setting focus when
	 * new items are added.
	 *
	 * A sorted QListWidget does not behave well with items that have the
	 * same sort key, in this case an empty category name.  So the category
	 * list is configured to be unsorted and is then sorted explicitly
	 * whenever a sort key changes, including inserting a new category.
	 **/
	void add() override;

	/**
	 * Notification that the user changed the "Name" field of the
	 * current category.
	 **/
	void nameChanged( const QString & newName );

	/**
	 * Notification that the user changed the "Color" field of the
	 * current category.
	 **/
	void categoryColorChanged( const QString & newColor );

	/**
	 * Open a color dialog and let the user pick a color for the current
	 * category.
	 **/
	void pickCategoryColor();

	/**
	 * Notification that the user changed the fixed tile color.
	 **/
	void tileColorChanged( const QString & newColor );

	/**
	 * Open a color dialog and let the user pick a fixed color for
	 * the tiles.
	 **/
	void pickTileColor();

	/**
	 * Set the other widgets when the cushion shading checkbox is changed.
	 **/
	void cushionShadingChanged( bool state );

	/**
	 * Updates the treemapView when something changes in the configuration.
	 **/
	void configChanged();

	/**
	 * Process the action to toggle the colour previews.
	 **/
	void colourPreviewsTriggered( bool );


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
	QToolButton * addButton()    const override { return _ui->addButton;      };
	QToolButton * removeButton() const override { return _ui->removeButton;   };

	/**
	 * Populate the widgets.
	 **/
	void populateTreemapTab();

	/**
	 * Fill the category list widget from the category collection.
	 *
	 * Reimplemented from ListEditor.
	 **/
	void fillListWidget() override;

	/**
	 * The category list has been shown.  Adjust the colour shading.
	 **/
	void setShading();

	/**
	 * Set the background shading of a list item.
	 **/
	void setBackground( QListWidgetItem * item );

	/**
	 * Save the patterns from the dialog to the specified category.
	 * The name and colour are handled in real-time as they are edited.
	 **/
	void save( void * value ) override;

	/**
	 * Load the fields from the specified category into the dialog.
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
	void deleteValue( void * value ) override;

	/**
	 * Return a text for the list item 'value'.
	 *
	 * Reimplemented from ListEditor.
	 **/

	QString valueText( void * value ) override;

	/**
	 * Signal handler for a change in the list widget current item.
	 *
	 * Reimplemented from ListEditor.
	 **/
	void currentItemChanged( QListWidgetItem * current, QListWidgetItem * previous) override;

	/**
	 * Update actions to match the current item properties.
	 *
	 * Reimplemented from ListEditor.
	 **/
	void updateActions() override;

	/**
	 * Handle a right click.
	 *
	 * Reimplemented from QWidget.
	 **/
	void contextMenuEvent( QContextMenuEvent * event ) override;

	/**
	 * Detect when the category list background needs to be reset
	 * because of palette or size changes.
	 **/
	bool event( QEvent * event ) override;


    private:

	// Data

	std::unique_ptr<Ui::MimeCategoryConfigPage> _ui;

	bool _dirty{ false };

    };	// class MimeCategoryConfigPage

}	// namespace QDirStat

#endif	// MimeCategoryConfigPage_h
