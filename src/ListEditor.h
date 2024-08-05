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
     * Item class for the QListWidget in a ListEditor. This connects the
     * QListWidgetItem with a void * pointer to the underlying configuration
     * object.
     **/
    class ListEditorItem: public QListWidgetItem
    {
    public:
	/**
	 * Create a new item with the specified text and store the value.
	 **/
	ListEditorItem( QListWidget * listWidget, const QString & text, void * value ):
	    QListWidgetItem { text, listWidget },
	    _value { value }
	{}

	/**
	 * Return the associated value.
	 **/
	void * value() const { return _value; }

    private:
	void * _value;
    };


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
	    QWidget { parent }
	{}

	/**
	 * Destructor.  Just declared here for clarity.
	 **/
	~ListEditor() override = default;


    protected:

	/**
	 * Return the list widget for this page (derived class).
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
	 * Fill the list widget: create a ListEditorItem for each value.
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
	 * Create a ListEditorItem and add it to the list widget.
	 **/
	ListEditorItem * createItem( const QString & valueText, void * value )
	    { return new ListEditorItem { listWidget(), valueText, value }; }

	/**
	 * Enable or disable buttons depending on internal status.
	 **/
	virtual void updateActions();

	/**
	 * Convert 'item' to a ListEditorItem and return its value.
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
	QAction * actionToTop()    { return toTopButton()->defaultAction();    }
	QAction * actionMoveUp()   { return moveUpButton()->defaultAction();   }
	QAction * actionAdd()      { return addButton()->defaultAction();      }
	QAction * actionRemove()   { return removeButton()->defaultAction();   }
	QAction * actionMoveDown() { return moveDownButton()->defaultAction(); }
	QAction * actionToBottom() { return toBottomButton()->defaultAction(); }

    };	// class ListEditor

}	// namespace QDirStat

#endif	// ListEditor_h
