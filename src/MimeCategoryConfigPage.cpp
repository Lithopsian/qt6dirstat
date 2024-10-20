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
#include "FormatUtil.h"
#include "Logger.h"
#include "MainWindow.h"
#include "MimeCategorizer.h"
#include "QDirStatApp.h"


#define CATEGORY_CAST(VOID_PTR) (static_cast<MimeCategory *>(VOID_PTR))


using namespace QDirStat;


namespace
{
    /**
     * Add demo content to the tremap view.
     **/
    void populateTreemapView( TreemapView * treemapView )
    {
	DirTree * dirTree = new DirTree{ treemapView };

	DirInfo        * root    = dirTree->root();
	const mode_t     mode    = 0755;
	const FileSize   dirSize = 4096LL;

	// Create a very basic directory structure:
	//
	//	dir1
	//	  dir11
	//	  dir12
	//	dir2
	//	  dir21
	//	    dir211
	//	    dir212

	DirInfo * dir1 = new DirInfo{ root, dirTree, "dir1", mode, dirSize };
	root->insertChild( dir1 );

	DirInfo * dir11 = new DirInfo{ dir1, dirTree, "dir11", mode, dirSize };
	dir1->insertChild( dir11 );

	DirInfo * dir12 = new DirInfo{ dir1, dirTree, "dir12", mode, dirSize };
	dir1->insertChild( dir12 );

	DirInfo * dir2 = new DirInfo{ root, dirTree, "dir2", mode, dirSize };
	root->insertChild( dir2 );

	DirInfo * dir21 = new DirInfo{ dir2, dirTree, "dir21", mode, dirSize };
	dir2->insertChild( dir21 );

	DirInfo * dir211 = new DirInfo{ dir21, dirTree, "dir211", mode, dirSize };
	dir21->insertChild( dir211 );

	DirInfo * dir212 = new DirInfo{ dir21, dirTree, "dir212", mode, dirSize };
	dir21->insertChild( dir212 );

	// Generate a random number of files with random sizes
	QRandomGenerator * random = QRandomGenerator::global();
	const FileSize maxSize = 100LL*1024*1024; // 100 MB
	for ( DirInfo * parent : { dir1, dir11, dir11, dir11, dir12, dir2, dir21, dir211, dir211, dir212 } )
	{
	    const int fileCount = random->bounded( 1, 200 );
	    for ( int i=0; i < fileCount; i++ )
	    {
		// Select a random file size
		const FileSize fileSize = random->bounded( 1, maxSize );

		// Create a FileInfo item and add it to the parent
		parent->insertChild( new FileInfo{ parent, dirTree, QString{}, mode, fileSize } );
	    }

	    parent->finalizeLocal(); // moves files out of DotEntries when there are no sub-directories
	    //logDebug() << parent->name() << " " << parent->totalAllocatedSize() << Qt::endl;
	}

	treemapView->setDirTree( dirTree );
    }


    /**
     * Convert 'patternList' into a newline-separated string and set it as
     * text of 'textEdit'.
     **/
    void setPatternList( QPlainTextEdit * textEdit, const QStringList & patternList )
    {
	QString text = patternList.join( u'\n' );

	if ( !text.isEmpty() )
	    text += u'\n'; // Let the user begin writing on a new line

	textEdit->setPlainText( text );
    }

} // namespace


MimeCategoryConfigPage::MimeCategoryConfigPage( ConfigDialog * parent ):
    ListEditor{ parent },
    _ui{ new Ui::MimeCategoryConfigPage }
{
    //logDebug() << "MimeCategoryConfigPage constructor" << Qt::endl;

    _ui->setupUi( this );
    _ui->nameLineEdit->setValidator( new QRegularExpressionValidator{ excludeControlCharacters(), this } );

    initListWidget();

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

    connect( _ui->squarifiedCheckBox,       &QCheckBox::toggled,
             this,                          &MimeCategoryConfigPage::configChanged );

    connect( _ui->cushionShadingCheckBox,   &QCheckBox::toggled,
             this,                          &MimeCategoryConfigPage::cushionShadingChanged );

    connect( _ui->cushionHeightSpinBox,     QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
             this,                          &MimeCategoryConfigPage::configChanged );

    connect( _ui->heightScaleFactorSpinBox, QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
             this,                          &MimeCategoryConfigPage::configChanged );

    connect( _ui->minTileSizeSpinBox,       QOverload<int>::of( &QSpinBox::valueChanged ),
             this,                          &MimeCategoryConfigPage::configChanged );

    connect( _ui->listWidget,               &QListWidget::itemSelectionChanged,
             this,                          &MimeCategoryConfigPage::setShading );

    connect( _ui->horizontalSplitter,       &QSplitter::splitterMoved,
             this,                          &MimeCategoryConfigPage::setShading );

    connect( _ui->actionColourPreviews,     &QAction::triggered,
             this,                          &MimeCategoryConfigPage::colourPreviewsTriggered );

    connect( parent,                        &ConfigDialog::applyChanges,
             this,                          &MimeCategoryConfigPage::applyChanges );

    // Do this now so the correct settings will be sent to the mini-treemap
    populateTreemapTab();
}


MimeCategoryConfigPage::~MimeCategoryConfigPage()
{
    //logDebug() << "MimeCategoryConfigPage destructor" << Qt::endl;

    // Delete the working categories
    for ( int i = 0; i < _ui->listWidget->count(); ++i )
	delete CATEGORY_CAST( value( _ui->listWidget->item( i ) ) );
}


void MimeCategoryConfigPage::populateTreemapTab()
{
    populateTreemapView( _ui->treemapView );

    // Get the treemap configuration settings from the main treemapView
    // The settings in _ui->treemapView will be from diak and possibly out of date
    const MainWindow * mainWindow = app()->mainWindow();
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
    app()->mainWindow()->treemapView()->configChanged( QColor{ _ui->tileColorEdit->text() },
                                                       _ui->squarifiedCheckBox->isChecked(),
                                                       _ui->cushionShadingCheckBox->isChecked(),
                                                       _ui->cushionHeightSpinBox->value(),
                                                       _ui->heightScaleFactorSpinBox->value(),
                                                       _ui->minTileSizeSpinBox->value() );

    // The patterns for the current category might have been modified and not yet saved to the category
    save( value( _ui->listWidget->currentItem() ) );

    // If nothing has changed, don't write the category settings file
    if ( !_dirty )
	return;

    // Build a list of the working categories to write out to the settings file
    MimeCategoryList categories;
    for ( int i = 0; i < _ui->listWidget->count(); ++i )
	categories << CATEGORY_CAST( value( _ui->listWidget->item( i ) ) );

    // Pass the working category list to the categorizer to save
    MimeCategorizer::instance()->replaceCategories( categories );
}


void MimeCategoryConfigPage::fillListWidget()
{
    //logDebug() << _ui->listWidget->count() << ", " << currentRow << Qt::endl;

    for ( const MimeCategory * mimeCategory : *( MimeCategorizer::instance() ) )
    {
	// Make a deep copy so the config dialog can work without disturbing the real categories
	MimeCategory * category = new MimeCategory{ *mimeCategory };
	createItem( category->name(), category );
    }

    _ui->listWidget->sortItems();
}


void MimeCategoryConfigPage::currentItemChanged( QListWidgetItem * current,
                                                 QListWidgetItem * previous)
{
    //logDebug() << current << ", " << previous << Qt::endl;

    ListEditor::currentItemChanged( current, previous );

    setBackground( current );
    setBackground( previous );
}


void MimeCategoryConfigPage::setBackground( QListWidgetItem * item )
{
    if ( !item )
	return;

    const MimeCategory * category = CATEGORY_CAST( value( item ) );
    if ( !category )
	return;

    const bool previews = app()->mainWindow()->treemapView()->colourPreviews();

    const qreal width         = _ui->listWidget->width();
    const qreal backgroundEnd = previews ? ( width - 21 ) / width : width;
    const qreal shadingStart  = ( width - 20 ) / width;
    const qreal shadingMiddle = ( width - 10 ) / width;

    const bool     current         = item == _ui->listWidget->currentItem();
    const QColor & backgroundColor = current ? palette().highlight().color() : palette().base().color();

    QLinearGradient gradient{ 0, 0, 1, 0 };
    gradient.setCoordinateMode( QGradient::ObjectMode );
    gradient.setColorAt( 0, backgroundColor );
    gradient.setColorAt( backgroundEnd, backgroundColor );
    if ( previews )
    {
	if ( _ui->cushionShadingCheckBox->isChecked() )
	{
	    const QColor shadeColor = category->color().darker( 300 );
	    gradient.setColorAt( shadingStart,  shadeColor );
	    gradient.setColorAt( shadingMiddle, category->color() );
	    gradient.setColorAt( 1, shadeColor );
	}
	else
	    gradient.setColorAt( shadingStart, category->color() );
    }

    item->setBackground( gradient );
    item->setForeground( current ? palette().highlightedText() : palette().text() );
}


void MimeCategoryConfigPage::updateActions()
{
    const QListWidgetItem * currentItem = _ui->listWidget->currentItem();

    const bool isSymLink =
	currentItem && currentItem->text() == MimeCategorizer::symlinkCategoryName();
    const bool isExecutable =
	currentItem && currentItem->text() == MimeCategorizer::executableCategoryName();

    // Name can't be changed for symlinks and executables
    _ui->nameLineEdit->setEnabled( currentItem && !isSymLink && !isExecutable );
    _ui->categoryColorEdit->setEnabled( currentItem );

    // Patterns can't be changed for symlinks
    _ui->patternsTopWidget->setEnabled( currentItem && !isSymLink );
    _ui->patternsBottomWidget->setEnabled( currentItem && !isSymLink );

    // Symlinks and executables can't be removed
    actionRemove()->setEnabled( currentItem && !isSymLink && !isExecutable );

    // Any category can have a colour
    _ui->categoryColorButton->setEnabled( currentItem );
}


void MimeCategoryConfigPage::nameChanged( const QString & newName )
{
    QListWidgetItem * currentItem = _ui->listWidget->currentItem();
    if ( !currentItem )
	return;

    MimeCategory * category = CATEGORY_CAST( value( currentItem ) );
    if ( newName == category->name() )
	return;

    category->setName( newName );
    currentItem->setText( newName );
    _ui->listWidget->sortItems();

    _dirty = true;
}


void MimeCategoryConfigPage::categoryColorChanged( const QString & newColor )
{
    // Always set the new colour, even if empty or invalid, for the mini-treemap to rebuild
    const QColor color{ newColor };
    _ui->treemapView->setFixedColor( color );

    QListWidgetItem * currentItem = _ui->listWidget->currentItem();
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
    QListWidgetItem * currentItem = _ui->listWidget->currentItem();
    if ( currentItem )
    {
	const MimeCategory * category = CATEGORY_CAST( value( currentItem ) );
	const QColor color =
	    QColorDialog::getColor( category->color(), window(), tr( "Pick a category color" ) );
	if ( color.isValid() )
	    _ui->categoryColorEdit->setText( color.name() );
    }
}


void MimeCategoryConfigPage::tileColorChanged( const QString & newColor )
{
    const QColor color{ newColor };
    _ui->treemapView->setFixedColor( color.isValid() ? color : QColor{ _ui->categoryColorEdit->text() } );
}


void MimeCategoryConfigPage::pickTileColor()
{
    const QColor color = QColorDialog::getColor( color, window(), tr( "Pick a fixed tile color" ) );
    if ( color.isValid() )
	_ui->tileColorEdit->setText( color.name() );
}


void MimeCategoryConfigPage::cushionShadingChanged( bool state )
{
    //logDebug() << state << Qt::endl;

    _ui->cushionHeightSpinBox->setEnabled( state );
    _ui->heightScaleFactorSpinBox->setEnabled( state );
    setShading();
    configChanged();
}


void MimeCategoryConfigPage::configChanged()
{
    // Rebuild the mini-treemap with the latest settings
    const QColor color{ _ui->tileColorEdit->text() };
    _ui->treemapView->configChanged( color.isValid() ? color : QColor{ _ui->categoryColorEdit->text() },
                                     _ui->squarifiedCheckBox->isChecked(),
                                     _ui->cushionShadingCheckBox->isChecked(),
                                     _ui->cushionHeightSpinBox->value(),
                                     _ui->heightScaleFactorSpinBox->value(),
                                     _ui->minTileSizeSpinBox->value() );
}


void MimeCategoryConfigPage::save( void * value )
{
    MimeCategory * category = CATEGORY_CAST( value );
    if ( !category )
	return;

    // Make a list of the patterns, one per line, and remove the empty entry caused by the trailing newline
    QStringList caseSensitivePatterns = _ui->caseSensitivePatternsTextEdit->toPlainText().split( u'\n' );
    if ( !caseSensitivePatterns.isEmpty() && caseSensitivePatterns.last().isEmpty() )
	caseSensitivePatterns.removeLast();
    QStringList caseInsensitivePatterns = _ui->caseInsensitivePatternsTextEdit->toPlainText().split( u'\n' );
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
    const MimeCategory * category = CATEGORY_CAST( value );
    //logDebug() << category << " (" << value << ")" << Qt::endl;

    // Populate the name and pattern fields from this category
    _ui->nameLineEdit->setText( category ? category->name() : QString{} );

    QStringList patternList = category ? category->humanReadablePatternList( Qt::CaseSensitive ) : QStringList{};
    setPatternList( _ui->caseSensitivePatternsTextEdit, patternList );

    patternList = category ? category->humanReadablePatternList( Qt::CaseInsensitive ) : QStringList{};
    setPatternList( _ui->caseInsensitivePatternsTextEdit, patternList );

    // Set this category colour in the form and mini-treemap
    const QColor color = category ? category->color() : QColor{};
    _ui->categoryColorEdit->setText( color.isValid() ? category->color().name() : QString{} );
    _ui->treemapView->setFixedColor( color );
}


void * MimeCategoryConfigPage::newValue()
{
    // ListEditor is making a new row in the category list
    _dirty = true;

    return new MimeCategory{};
}


void MimeCategoryConfigPage::deleteValue( void * value )
{
    // LitEditor is removing a row in the category list
    delete CATEGORY_CAST( value );
    _dirty = true;
}


QString MimeCategoryConfigPage::valueText( void * value )
{
    const MimeCategory * category = CATEGORY_CAST( value );

    return category->name();
}


void MimeCategoryConfigPage::setShading()
{
    // Keep the colour preview the same width always
    for ( int x = 0; x < _ui->listWidget->count(); ++x )
    {
	QListWidgetItem * item = _ui->listWidget->item( x );
	setBackground( item );
    }
}


void MimeCategoryConfigPage::contextMenuEvent( QContextMenuEvent * event )
{
    //logDebug() << Qt::endl;

    if ( _ui->listWidget->underMouse() )
    {
        _ui->actionColourPreviews->setChecked( app()->mainWindow()->treemapView()->colourPreviews() );

        QMenu menu;
        menu.addAction( actionAdd() );
        menu.addAction( actionRemove() );
        menu.addSeparator();
        menu.addAction( _ui->actionColourPreviews );

        menu.exec( event->globalPos() );
    }
}


void MimeCategoryConfigPage::colourPreviewsTriggered( bool checked )
{
    // Context menu colour previews toggle action
    app()->mainWindow()->treemapView()->setColourPreviews( checked );
    setShading();
}


void MimeCategoryConfigPage::add()
{
    // Overload LitEditor to allow sorting after an insert
    // (and put the focus in the name field, which almost always needs filling in)
    ListEditor::add();

    _ui->listWidget->sortItems();
    _ui->nameLineEdit->setFocus();
}


bool MimeCategoryConfigPage::event( QEvent * event )
{
    const auto type = event->type();
    if ( type == QEvent::PaletteChange || type == QEvent::Show || type == QEvent::Resize )
	setShading();

    return ListEditor::event( event );
}
