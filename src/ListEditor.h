/*
 *   File name: ListEditor.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef ListEditor_h
#define ListEditor_h

#include <QAction>
#include <QListWidgetItem>
#include <QToolButton>
#include <QWidget>


namespace QDirStat
{
    /**
     * This is an abstract widget base class for config pages that have a list
     * of items on the left and details for one item (the current item) on the
     * right.
     *
     * This base class manages selecting an item in the list and displaying its
     * contents (load()), saving any user changes (save()), adding and removing
     * list items, and optionally moving the current item up, down, to the
     * top, and to the bottom of the list.
     *
     * This class contains pure virtual methods; derived classes are required
     * to implement them.
     *
     * This class was first designed as a template class, but since even in
     * 2016 Qt's moc cannot handle templates, ugly void * and nightmarish
     * type casts had to be used.
     **/
    class ListEditor: public QWidget
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	ListEditor( QWidget * parent ):
	    QWidget{ parent }
	{}

	/**
	 * Destructor.  Just declared here for clarity.
	 **/
	~ListEditor() override = default;


    protected slots:

	/**
	 * Move the current list item to the top of the list.
	 **/
	void toTop();

	/**
	 * Move the current list item one position up.
	 **/
	void moveUp();

	/**
	 * Create a new list item.
	 **/
	virtual void add();

	/**
	 * Remove the current list item.
	 **/
	void remove();

	/**
	 * Move the current list item one position down.
	 **/
	void moveDown();

	/**
	 * Move the current list item to the bottom of the list.
	 **/
	void toBottom();

	/**
	 * Notification that the current item in the list widget changed.
	 **/
	virtual void currentItemChanged( QListWidgetItem * current,
	                                 QListWidgetItem * previous);


    protected:

	/**
	 * Return the list widget for this page.
	 *
	 * Derived classes are required to implement this.
	 **/
	virtual QListWidget * listWidget() const = 0;

	/**
	 * Return a tool button for the derived class page.
	 *
	 * The default implementations return 0.  Derived classes can
	 * reimplement if they have that button.
	 **/
	virtual QToolButton * toTopButton()    const { return nullptr; };
	virtual QToolButton * moveUpButton()   const { return nullptr; };
	virtual QToolButton * addButton()      const { return nullptr; };
	virtual QToolButton * removeButton()   const { return nullptr; };
	virtual QToolButton * moveDownButton() const { return nullptr; };
	virtual QToolButton * toBottomButton() const { return nullptr; };

	/**
	 * Fill the list widget: create a QListWidgetItem for each value.
	 *
	 * Derived classes are required to implement this.
	 **/
	virtual void fillListWidget() = 0;

	/**
	 * Save the contents of the widgets to the specified value.
	 *
	 * Derived classes are required to implement this.
	 **/
	virtual void save( void * value ) = 0;

	/**
	 * Load the content of the widgets from the specified value.
	 *
	 * Derived classes are required to implement this.
	 **/
	virtual void load( void * value ) = 0;

	/**
	 * Create a new value item with default values and add it to the
	 * internal list.
	 *
	 * This is called when the 'Add' button is clicked.
	 *
	 * Derived classes are required to implement this.
	 **/
	virtual void * newValue() = 0;

	/**
	 * Delete a value from the internal list.
	 *
	 * This is called when the 'Remove' button is clicked.
	 *
	 * Derived classes are required to implement this.
	 **/
	virtual void deleteValue( void * value ) = 0;

	/**
	 * Return the text for the list item of 'value'.
	 *
	 * Derived classes are required to implement this.
	 **/
	virtual QString valueText( void * value ) = 0;

	/**
	 * Connect the list widget and toolbutton actions.  This can't
	 * be done in the ListEditor constructor because the page UI
	 * isn't set up yet.
	 **/
	void connectActions();

	/**
	 * Create an action for a QToolButton.  Actions are only
	 * created for pages that set the corresponding button.
	 *
	 * The new action is connected to the button and slot
	 * function, and a hotkey set based on the settings and
	 * the given default value.
	 **/
	void createAction( const QString      & actionName,
	                   const QString      & icon,
	                   const QString      & text,
	                   const QKeySequence & keySequence,
	                   QToolButton        * button,
	                   void( ListEditor::*actee )( void ) );

	/**
	 * Create a QListWidgetItem and add it to the list widget.
	 **/
	QListWidgetItem * createItem( const QString & valueText, void * value );

	/**
	 * Enable or disable buttons depending on internal status.
	 **/
	virtual void updateActions();

	/**
	 * Return the value (void *) for 'item'.
	 **/
	void * value( QListWidgetItem * item );

	/**
	 * Moves the current item to a new position in the list.
	 **/
	void moveCurrentItem( int newRow );

	/**
	 * Handle a right click.
	 *
	 * The default implementation opens context menu with the six
	 * button actions.  Derived classes can override this.
	 *
	 * Reimplemented from QWidget.
	 **/
	void contextMenuEvent( QContextMenuEvent * event ) override;

	/**
	 * Getters for the actions
	 **/
	QAction * actionToTop()    const { return toTopButton()->defaultAction();    }
	QAction * actionMoveUp()   const { return moveUpButton()->defaultAction();   }
	QAction * actionAdd()      const { return addButton()->defaultAction();      }
	QAction * actionRemove()   const { return removeButton()->defaultAction();   }
	QAction * actionMoveDown() const { return moveDownButton()->defaultAction(); }
	QAction * actionToBottom() const { return toBottomButton()->defaultAction(); }

    };	// class ListEditor

}	// namespace QDirStat

#endif	// ListEditor_h
