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
     * This class does not own, create or destroy any widgets; it only uses
     * widgets that have been created elsewhere, typically via a Qt Designer
     * .ui file. The widgets should be set with any of the appropriate setter
     * methods.
     *
     * This class contains pure virtual methods; derived classes are required
     * to implement them.
     *
     * This class was first designed as a template class, but since even in
     * 2016 Qt's moc cannot handle templates, ugly void * and nightmarish
     * type casts had to be used. This is due to deficiencies of the underlying
     * tools, not by design. Yes, this is ugly. But it's the best that can be
     * done with that moc preprocessor that dates back to 1995 or so.
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
	 * Destructor.
	 **/
	~ListEditor() override = default;


    protected:

	//
	// Pure virtual methods that are required to be implemented by a
	// derived class
	//

	/**
	 * Fill the list widget: Create a ListEditorItem for each value.
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
	 * Create a new Value_t item with default values and add it to the
	 * internal list.
	 *
	 * This is called when the 'Add' button is clicked.
	 *
	 * Derived classes are required to implement this.
	 **/
	virtual void * createValue() = 0;

	/**
	 * Remove a value from the internal list and delete it.
	 *
	 * This is called when the 'Remove' button is clicked.
	 *
	 * Derived classes are required to implement this.
	 **/
	virtual void removeValue( void * value ) = 0;

	/**
	 * Return a text for the list item of 'value'.
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
	 * Enable or disable buttons depending on internal status.
	 **/
	virtual void updateActions();

	/**
	 * Notification that the current item in the list widget changed.
	 **/
	virtual void currentItemChanged( QListWidgetItem * current,
					 QListWidgetItem * previous);


    protected:

	/**
	 * Set the QListWidget to work on.
	 **/
	void setListWidget( QListWidget * listWidget );

	/**
	 * Return the QListWidget.
	 **/
	QListWidget * listWidget() const { return _listWidget; }

	/**
	 * Return 'true' if updates are currently locked. save() and load()
	 * should check this and do nothing if updates are locked.
	 **/
	bool updatesLocked() const { return _updatesLocked; }

	/**
	 * Lock or unlock updates.
	 **/
	void setUpdatesLocked( bool locked ) { _updatesLocked = locked; }

	/**
	 * Convert 'item' to a ListEditorItem<void *> and return its value.
	 **/
	void * value( QListWidgetItem * item );

	/**
	 * Handle a right click.
	 *
	 * Reimplemented from QWidget.
	 **/
	void contextMenuEvent( QContextMenuEvent * event ) override;

	//
	// Set the various buttons and connect them to the appropriate action.
	//

	void setToTopButton   ( QToolButton * button );
	void setMoveUpButton  ( QToolButton * button );
	void setAddButton     ( QToolButton * button );
	void setRemoveButton  ( QToolButton * button );
	void setMoveDownButton( QToolButton * button );
	void setToBottomButton( QToolButton * button );

	QAction * toTopAction()    { return &_toTopAction; }
	QAction * moveUpAction()   { return &_moveUpAction; }
	QAction * addAction()      { return &_addAction; }
	QAction * removeAction()   { return &_removeAction; }
	QAction * moveDownAction() { return &_moveDownAction; }
	QAction * toBottomAction() { return &_toBottomAction; }


    private:

	//
	// Data members
	//

	bool _updatesLocked	{ false };

	QListWidget * _listWidget	{ nullptr };
	QToolButton * _toTopButton	{ nullptr };
	QToolButton * _moveUpButton	{ nullptr };
	QToolButton * _addButton	{ nullptr };
	QToolButton * _removeButton	{ nullptr };
	QToolButton * _moveDownButton	{ nullptr };
	QToolButton * _toBottomButton	{ nullptr };

	QAction _toTopAction    { QIcon( ":/icons/move-top.png" ),    tr( "Move to &top" ),           this };
	QAction _moveUpAction   { QIcon( ":/icons/move-up.png" ),     tr( "Move &up" ),               this };
	QAction _addAction      { QIcon( ":/icons/add.png" ),         tr( "&Create a new category" ), this };
	QAction _removeAction   { QIcon( ":/icons/remove.png" ),      tr( "&Remove category" ),       this };
	QAction _moveDownAction { QIcon( ":/icons/move-down.png" ),   tr( "Move &down" ),             this };
	QAction _toBottomAction { QIcon( ":/icons/move-bottom.png" ), tr( "Move to &bottom" ),        this };

    };	// class ListEditor


    /**
     * Item class for the QListWidget in a ListEditor. This connects the
     * QListWidgetItem with the void * pointer.
     **/
    class ListEditorItem: public QListWidgetItem
    {
    public:
	/**
	 * Create a new item with the specified text and store the value.
	 **/
	ListEditorItem( const QString & text, void * value ):
	    QListWidgetItem { text },
	    _value { value }
	{}

	/**
	 * Return the associated value.
	 **/
	void * value() const { return _value; }

    private:
	void * _value;
    };

}	// namespace QDirStat

#endif	// ListEditor_h
