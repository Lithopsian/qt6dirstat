/*
 *   File name: ExcludeRulesConfigPage.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef ExcludeRulesConfigPage_h
#define ExcludeRulesConfigPage_h

#include <memory>

#include "ListEditor.h"
#include "ui_exclude-rules-config-page.h"


class QListWidget;


namespace QDirStat
{
    class ConfigDialog;

    /**
     * Configuration page (tab) for exclude rules:
     * Edit, add, delete, and reorder exclude rules.
     **/
    class ExcludeRulesConfigPage: public ListEditor
    {
	Q_OBJECT

    public:

	ExcludeRulesConfigPage( ConfigDialog * parent );
	~ExcludeRulesConfigPage() override;


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
	 * Notification that the user changed the pattern field of the
	 * current exclude rule.
	 **/
	void patternChanged( const QString & newPattern );


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
	 * Enable or disable the widgets to edit an exclude rule.
	 **/
	void enableEditWidgets( bool enable )
	    { _ui->rightColumnWidget->setEnabled( enable ); }

	/**
	 * Fill the exclude rule list widget from the ExcludeRules.
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
	 * This is called when the 'Remove' button is clicked and the user
	 * confirms the confirmation pop-up.
	 *
	 * Reimplemented from ListEditor.
	 **/
	void deleteValue( void * value );

	/**
	 * Return a text for the list item of 'value'.
	 *
	 * Reimplemented from ListEditor.
	 **/

	QString valueText( void * value ) override;


    private:

	//
	// Data members
	//

	std::unique_ptr<Ui::ExcludeRulesConfigPage> _ui;

    };	// class ExcludeRulesConfigPage

}	// namespace QDirStat

#endif	// ExcludeRulesConfigPage_h
