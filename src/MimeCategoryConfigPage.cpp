/*
 *   File name: MimeCategoryConfigPage.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QColorDialog>
#include <QRandomGenerator>

#include "MimeCategoryConfigPage.h"
#include "ConfigDialog.h"
#include "DirTree.h"
#include "DirInfo.h"
#include "MainWindow.h"
#include "MimeCategorizer.h"
#include "MimeCategory.h"
#include "QDirStatApp.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


// This is a mess that became necessary because Qt's moc cannot handle template
// classes. Yes, this is ugly.
#define CATEGORY_CAST(VOID_PTR) (static_cast<MimeCategory *>(VOID_PTR))



MimeCategoryConfigPage::MimeCategoryConfigPage( ConfigDialog * parent ):
    ListEditor ( parent ),
    _ui { new Ui::MimeCategoryConfigPage }
{
    //logDebug() << "MimeCategoryConfigPage constructor" << Qt::endl;

    CHECK_NEW( _ui );
    _ui->setupUi( this );

    setListWidget  ( _ui->listWidget   );
    setAddButton   ( _ui->addButton    );
    setRemoveButton( _ui->removeButton );

    setup();

    connect( _ui->nameLineEdit,             &QLineEdit::textChanged,
	     this,                          &MimeCategoryConfigPage::nameChanged );

    connect( _ui->categoryColorEdit,        &QLineEdit::textChanged,
	     this,                          &MimeCategoryConfigPage::categoryColorChanged );

    connect( _ui->categoryColorButton,      &QPushButton::clicked,
	     this,                          &MimeCategoryConfigPage::pickCategoryColor );

    connect( _ui->tileColorEdit,            &QLineEdit::textChanged,
	     this,                          &MimeCategoryConfigPage::tileColorChanged );

    connect( _ui->tileColorButton,          &QPushButton::clicked,
	     this,                          &MimeCategoryConfigPage::pickTileColor );

    connect( _ui->squarifiedCheckBox,       &QCheckBox::stateChanged,
	     this,                          &MimeCategoryConfigPage::configChanged );

    connect( _ui->cushionShadingCheckBox,   &QCheckBox::stateChanged,
	     this,                          &MimeCategoryConfigPage::cushionShadingChanged );

    connect( _ui->cushionHeightSpinBox,     qOverload<double>( &QDoubleSpinBox::valueChanged ),
	     this,                          &MimeCategoryConfigPage::configChanged );

    connect( _ui->heightScaleFactorSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ),
	     this,                          &MimeCategoryConfigPage::configChanged );

    connect( _ui->minTileSizeSpinBox,       qOverload<int>( &QSpinBox::valueChanged ),
	     this,                          &MimeCategoryConfigPage::configChanged );

    connect( _ui->horizontalSplitter,       &QSplitter::splitterMoved,
	     this,                          &MimeCategoryConfigPage::splitterMoved );

    connect( _ui->actionColour_previews,    &QAction::triggered,
	     this,                          &MimeCategoryConfigPage::colourPreviewsTriggered );

    connect( _ui->actionAdd_new_category,   &QAction::triggered,
	     this,                          &MimeCategoryConfigPage::addTriggered );

    connect( _ui->actionRemove_category,    &QAction::triggered,
	     this,                          &MimeCategoryConfigPage::removeTriggered );

    connect( parent,                        &ConfigDialog::applyChanges,
	     this,                          &MimeCategoryConfigPage::applyChanges );
}


MimeCategoryConfigPage::~MimeCategoryConfigPage()
{
    //logDebug() << "MimeCategoryConfigPage destructor" << Qt::endl;

    // Delete the working categories
    for ( int i = 0; i < listWidget()->count(); ++i )
	delete CATEGORY_CAST( value( listWidget()->item( i ) ) );

    _ui->treemapView->setDirTree( nullptr );
    delete _dirTree;

    delete _ui;
}


void MimeCategoryConfigPage::setup()
{
    //logDebug() << Qt::endl;

    populateTreemapView();
    _ui->treemapView->setFixedColor( Qt::white );

    // Populate the category list
    fillListWidget();
    updateActions();

    // Get the treemap configuration settings from the main treemapView
    const MainWindow * mainWindow = app()->findMainWindow();
    if ( !mainWindow )
	return;

    _ui->squarifiedCheckBox->setChecked    ( mainWindow->treemapView()->squarify() );
    _ui->cushionShadingCheckBox->setChecked( mainWindow->treemapView()->doCushionShading() );
    _ui->cushionHeightSpinBox->setValue    ( mainWindow->treemapView()->cushionHeight() );
    _ui->heightScaleFactorSpinBox->setValue( mainWindow->treemapView()->heightScaleFactor() );
    _ui->minTileSizeSpinBox->setValue      ( mainWindow->treemapView()->minTileSize() );

    if ( mainWindow->treemapView()->fixedColor().isValid() )
	_ui->tileColorEdit->setText( mainWindow->treemapView()->fixedColor().name() );

    _ui->cushionHeightLabel->setEnabled      ( _ui->cushionShadingCheckBox->isChecked() );
    _ui->cushionHeightSpinBox->setEnabled    ( _ui->cushionShadingCheckBox->isChecked() );
    _ui->heightScaleFactorLabel->setEnabled  ( _ui->cushionShadingCheckBox->isChecked() );
    _ui->heightScaleFactorSpinBox->setEnabled( _ui->cushionShadingCheckBox->isChecked() );
}


void MimeCategoryConfigPage::applyChanges()
{
    //logDebug() << Qt::endl;

    // Save the treemap settings first, there might not be anything else to do
    const MainWindow * mainWindow = app()->findMainWindow();
    if ( mainWindow )
    {
//	treemapView->setFixedColor( QColor( _ui->tileColorEdit->text() ) );
	mainWindow->treemapView()->configChanged( QColor( _ui->tileColorEdit->text() ),
						 _ui->squarifiedCheckBox->isChecked(),
						 _ui->cushionShadingCheckBox->isChecked(),
						 _ui->cushionHeightSpinBox->value(),
						 _ui->heightScaleFactorSpinBox->value(),
						 _ui->minTileSizeSpinBox->value() );
    }

    // The patterns for the current category might have been modified and not yet saved to the category
    save( value( listWidget()->currentItem() ) );

    // If nothing has changed ...
    if ( !_dirty )
	return;

    // Build a list of the working categories to write out to the settings file
    MimeCategoryList categories;
    for ( int i = 0; i < listWidget()->count(); ++i )
	categories << CATEGORY_CAST( value( listWidget()->item( i ) ) );

    // Pass the working category list to the categorizer to save
    MimeCategorizer::instance()->replaceCategories( categories );
}


void MimeCategoryConfigPage::fillListWidget()
{
//    listWidget()->clear(); // should always be empty, but future-proof it
    //logDebug() << listWidget()->count() << ", " << currentRow << Qt::endl;


    const MimeCategoryList * categories = &(MimeCategorizer::instance()->categories());
    for ( int i = 0; i < categories->size(); ++i )
    {
	// Make a deep copy so the config dialog can work without disturbing the real categories
	MimeCategory * category = new MimeCategory( *categories->at( i ) );

	QListWidgetItem * item = new ListEditorItem( category->name(), category );
	CHECK_NEW( item );
	listWidget()->addItem( item );
    }

    listWidget()->sortItems();
    listWidget()->setCurrentRow( 0 );
}


void MimeCategoryConfigPage::updateActions()
{
//    ListEditor::updateActions();

    setActions( listWidget()->currentItem() );
}


void MimeCategoryConfigPage::currentItemChanged( QListWidgetItem * current,
                                                 QListWidgetItem * previous)
{
    //logDebug() << current << ", " << previous << Qt::endl;

    ListEditor::currentItemChanged( current, previous );

    setActions( current );

    setBackground( current );
    setBackground( previous );
}


void MimeCategoryConfigPage::setBackground( QListWidgetItem * item )
{
    if ( !item )
	return;

    const MainWindow * mainWindow = app()->findMainWindow();
    if ( !mainWindow )
	return;

    const MimeCategory * category = CATEGORY_CAST( value( item ) );
    if ( !category )
	return;

    const bool previews = mainWindow->treemapView()->colourPreviews();

    const qreal width         = listWidget()->width();
    const qreal backgroundEnd = previews ? ( width - 21 ) / width : width;
    const qreal shadingStart  = ( width - 20 ) / width;
    const qreal shadingMiddle = ( width - 10 ) / width;

    const QPalette palette       = listWidget()->palette();
    const bool current           = item == listWidget()->currentItem();
    const QColor backgroundColor = current ? Qt::lightGray : palette.color( QPalette::Active, QPalette::Base );

    QLinearGradient gradient( 0, 0, 1, 0 );
    gradient.setCoordinateMode( QGradient::ObjectMode );
    gradient.setColorAt( 0, backgroundColor );
    gradient.setColorAt( backgroundEnd, backgroundColor );
    if ( previews )
    {
	if ( _ui->cushionShadingCheckBox->isChecked() )
	{
	    const QColor shadingColor = category->color().darker( 300 );
	    gradient.setColorAt( shadingStart,  shadingColor );
	    gradient.setColorAt( shadingMiddle, category->color() );
	    gradient.setColorAt( 1, shadingColor );
	}
	else
	    gradient.setColorAt( shadingStart, category->color() );
    }

    const QBrush brush( gradient );

    item->setBackground( brush );
}


void MimeCategoryConfigPage::setActions( const QListWidgetItem * currentItem )
{
    const bool isSymlink =
	currentItem && currentItem->text() == MimeCategorizer::instance()->symlinkCategoryName();
    const bool isExecutable =
	currentItem && currentItem->text() == MimeCategorizer::instance()->executableCategoryName();

    // Name can't be changed for symlinks and executables
    _ui->nameLineEdit->setEnabled( currentItem && !isSymlink && !isExecutable );
    _ui->categoryColorEdit->setEnabled( currentItem );

    // Patterns can't be changed for symlinks
    _ui->patternsTopWidget->setEnabled( currentItem && !isSymlink );
    _ui->patternsBottomWidget->setEnabled( currentItem && !isSymlink );

    // Symlinks and executables can't be removed
    _ui->actionRemove_category->setEnabled( currentItem && !isSymlink && !isExecutable );
    enableButton( _ui->removeButton, currentItem && !isSymlink && !isExecutable );
    enableButton( _ui->categoryColorButton, currentItem );
}


void MimeCategoryConfigPage::nameChanged( const QString & newName )
{
    QListWidgetItem * currentItem = listWidget()->currentItem();
    if ( !currentItem )
	return;

    MimeCategory * category = CATEGORY_CAST( value( currentItem ) );
    if ( newName == category->name() )
	return;

    category->setName( newName );
    currentItem->setText( newName );
    listWidget()->sortItems();

    _dirty = true;
}


void MimeCategoryConfigPage::categoryColorChanged( const QString & newColor )
{
    QListWidgetItem * currentItem = listWidget()->currentItem();

    // Always set the new colour, even if empty or invalid, for the mini-treemap to rebuild
    const QColor color( newColor );
    _ui->treemapView->setFixedColor( color );

    if ( !currentItem )
	return;

    MimeCategory * category = CATEGORY_CAST( value( currentItem ) );
    if ( color == category->color() )
	return;

    category->setColor( color );
    setBackground( currentItem );

    _dirty = true;
}


void MimeCategoryConfigPage::pickCategoryColor()
{
    QListWidgetItem * currentItem = listWidget()->currentItem();

    if ( currentItem )
    {
	const MimeCategory * category = CATEGORY_CAST( value( currentItem ) );
	QColor color = category->color();
	color = QColorDialog::getColor( color,
					window(), // parent
					tr( "Pick a category color" ) );

	if ( color.isValid() )
	    _ui->categoryColorEdit->setText( color.name() );
    }
}


void MimeCategoryConfigPage::tileColorChanged( const QString & newColor )
{
    const QColor color( newColor );
    _ui->treemapView->setFixedColor( color.isValid() ? color : QColor( _ui->categoryColorEdit->text() ) );
}


void MimeCategoryConfigPage::pickTileColor()
{
    const QColor color = QColorDialog::getColor( color,
						 window(), // parent
						 tr( "Pick a fixed tile color" ) );

    if ( !color.isValid() )
	return;

    _ui->tileColorEdit->setText( color.name() );
}


void MimeCategoryConfigPage::cushionShadingChanged( int state )
{
    //logDebug() << state << Qt::endl;

    _ui->cushionHeightSpinBox->setEnabled( state == Qt::Checked );
    _ui->heightScaleFactorSpinBox->setEnabled( state == Qt::Checked );
    adjustShadingWidth();
    configChanged();
}


void MimeCategoryConfigPage::configChanged()
{
    // Rebuild the mini-treemap with the latest settings
    const QColor color( _ui->tileColorEdit->text() );
    _ui->treemapView->configChanged( color.isValid() ? color : QColor( _ui->categoryColorEdit->text() ),
                                     _ui->squarifiedCheckBox->isChecked(),
				     _ui->cushionShadingCheckBox->isChecked(),
				     _ui->cushionHeightSpinBox->value(),
				     _ui->heightScaleFactorSpinBox->value(),
				     _ui->minTileSizeSpinBox->value() );
}


void MimeCategoryConfigPage::save( void * value )
{
    MimeCategory * category = CATEGORY_CAST( value );
    // logDebug() << category << Qt::endl;

    if ( !category || updatesLocked() )
	return;

    // Make a list of the patterns, one per line, and remove the empty entry caused by the trailing newline
    QStringList caseSensitivePatterns = _ui->caseSensitivePatternsTextEdit->toPlainText().split( '\n' );
    if ( !caseSensitivePatterns.isEmpty() && caseSensitivePatterns.last().isEmpty() )
	caseSensitivePatterns.removeLast();
    QStringList caseInsensitivePatterns = _ui->caseInsensitivePatternsTextEdit->toPlainText().split( '\n' );
    if ( !caseInsensitivePatterns.isEmpty() && caseInsensitivePatterns.last().isEmpty() )
	caseInsensitivePatterns.removeLast();

    // If they're different to the current patterns on the category, update the category
    if ( caseSensitivePatterns != category->humanReadablePatternList( Qt::CaseSensitive ) ||
	 caseInsensitivePatterns != category->humanReadablePatternList( Qt::CaseInsensitive ) )
    {
	category->clear();
	category->addPatterns( caseSensitivePatterns, Qt::CaseSensitive );
	category->addPatterns( caseInsensitivePatterns, Qt::CaseInsensitive );
	_dirty = true;
    }
}


void MimeCategoryConfigPage::load( void * value )
{
     if ( updatesLocked() )
	return;

    const MimeCategory * category = CATEGORY_CAST( value );
    //logDebug() << category << " (" << value << ")" << Qt::endl;

    // Populate the name and pattern fields from this category
    _ui->nameLineEdit->setText( category ? category->name() : QString() );

    QStringList patternList = category ? category->humanReadablePatternList( Qt::CaseSensitive ) : QStringList();
    setPatternList( _ui->caseSensitivePatternsTextEdit, patternList );

    patternList = category ? category->humanReadablePatternList( Qt::CaseInsensitive ) : QStringList();
    setPatternList( _ui->caseInsensitivePatternsTextEdit, patternList );

    // Set this category colour in the form and mini-treemap
    const QColor color = category ? category->color() : QColor();
    _ui->categoryColorEdit->setText( color.isValid() ? category->color().name() : QString() );
    _ui->treemapView->setFixedColor( color );
}


void MimeCategoryConfigPage::setPatternList( QPlainTextEdit    * textEdit,
					     const QStringList & patternList )
{
    QString text = patternList.join( '\n' );

    if ( !text.isEmpty() )
	text += '\n';	   // Let the user begin writing on a new line

    textEdit->setPlainText( text );
}


void * MimeCategoryConfigPage::createValue()
{
    // ListEditor is making a new row in the category list
    MimeCategory * category = new MimeCategory( "", Qt::white );
    CHECK_NEW( category );

    _dirty = true;

    return category;
}


void MimeCategoryConfigPage::removeValue( void * value )
{
    // LitEditor is removing a row in the category list
    MimeCategory * category = CATEGORY_CAST( value );
    CHECK_PTR( category );

    delete category;

    _dirty = true;
}


QString MimeCategoryConfigPage::valueText( void * value )
{
    const MimeCategory * category = CATEGORY_CAST( value );
    CHECK_PTR( category );

    return category->name();
}


void MimeCategoryConfigPage::populateTreemapView()
{
    _dirTree = new DirTree();
    CHECK_NEW( _dirTree );

    DirInfo       * root    = _dirTree->root();
    const mode_t    mode    = 0755;
    const FileSize  dirSize = 4096;

    // Create a very basic directory structure:
    //
    //	dir1
    //	  dir11
    //	  dir12
    //	dir2
    //	  dir21
    //	    dir211
    //	    dir212

    DirInfo * dir1 = new DirInfo( root, _dirTree, "dir1", mode, dirSize );
    CHECK_NEW( dir1 );
    root->insertChild( dir1 );

    DirInfo * dir11 = new DirInfo( dir1, _dirTree, "dir11", mode, dirSize );
    CHECK_NEW( dir11 );
    dir1->insertChild( dir11 );

    DirInfo * dir12 = new DirInfo( dir1, _dirTree, "dir12", mode, dirSize );
    CHECK_NEW( dir12 );
    dir1->insertChild( dir12 );

    DirInfo * dir2 = new DirInfo( root, _dirTree, "dir2", mode, dirSize );
    CHECK_NEW( dir2 );
    root->insertChild( dir2 );

    DirInfo * dir21 = new DirInfo( dir2, _dirTree, "dir21", mode, dirSize );
    CHECK_NEW( dir21 );
    dir2->insertChild( dir21 );

    DirInfo * dir211 = new DirInfo( dir21, _dirTree, "dir211", mode, dirSize );
    CHECK_NEW( dir211 );
    dir21->insertChild( dir211 );

    DirInfo * dir212 = new DirInfo( dir21, _dirTree, "dir212", mode, dirSize );
    CHECK_NEW( dir212 );
    dir21->insertChild( dir212 );

    const FileSize maxSize   = 100*1024*1024;	// 100 MB

    // Generate a random number of files with random sizes
    QRandomGenerator * random = QRandomGenerator::global();
    for ( DirInfo * parent : { dir1, dir11, dir11, dir11, dir12, dir2, dir21, dir211, dir211, dir212 } )
    {
	const int fileCount = random->bounded( 1, 100 );
	for ( int i=0; i < fileCount; i++ )
	{
	    // Select a random file size
	    const FileSize fileSize = random->bounded( 1, maxSize );

	    // Create a FileInfo item and add it to the parent
	    FileInfo * file = new FileInfo( parent, _dirTree, QString( "File_%1" ).arg( i ), mode, fileSize );
	    CHECK_NEW( file );
	    parent->insertChild( file );
	}

	parent->finalizeLocal(); // moves files out of DotEntries when there are no sub-directories
	//logDebug() << parent->name() << " " << (long)parent->totalAllocatedSize() << Qt::endl;
    }

    _ui->treemapView->setDirTree( _dirTree );
}


void MimeCategoryConfigPage::adjustShadingWidth()
{
    // Keep the colour preview the same width always
    for ( int x = 0; x < listWidget()->count(); ++x )
    {
	QListWidgetItem * item = listWidget()->item( x );
	setBackground( item );
    }
}


void MimeCategoryConfigPage::contextMenuEvent( QContextMenuEvent * event )
{
    //logDebug() << Qt::endl;

    const MainWindow * mainWindow = app()->findMainWindow();
    if ( !mainWindow )
	return;

    _ui->actionColour_previews->setChecked( mainWindow->treemapView()->colourPreviews() );

    QMenu menu;
    menu.addAction( _ui->actionAdd_new_category );
    menu.addAction( _ui->actionRemove_category );
    menu.addSeparator();
    menu.addAction( _ui->actionColour_previews );

    menu.exec( event->globalPos() );
}


void MimeCategoryConfigPage::colourPreviewsTriggered( bool checked )
{
    // Context menu colour previews toggle action
    const MainWindow * mainWindow = app()->findMainWindow();
    if ( !mainWindow )
	return;

    mainWindow->treemapView()->setColourPreviews( checked );
    adjustShadingWidth();
}


void MimeCategoryConfigPage::addTriggered( bool )
{
    // Context menu add action
    ListEditor::add();
}


void MimeCategoryConfigPage::removeTriggered( bool )
{
    // Context menu remove action
    ListEditor::remove();
}


void MimeCategoryConfigPage::add()
{
    // Overload LitEditor to allow sorting after an insert
    // (and put the focus in the name field, which almost always needs filling in)
    ListEditor::add();

    listWidget()->sortItems();
    _ui->nameLineEdit->setFocus();
}
