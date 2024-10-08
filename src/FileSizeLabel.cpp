/*
 *   File name: FileSizeLabel.cpp
 *   Summary:   Specialized QLabel for a file size for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "FileSizeLabel.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "Logger.h"


using namespace QDirStat;


void FileSizeLabel::setSize( const FileInfo * file )
{
    setValueWithLinks( file->rawByteSize(), file->links() );
}


void FileSizeLabel::setAllocated( const FileInfo * file )
{
    const auto size = file->rawAllocatedSize();
    setValueWithLinks( size, file->links() );

    if ( file->isSparseFile() || ( size > 4096 && file->usedPercent() < ALLOCATED_FAT_PERCENT ) )
        setBold();
}


void FileSizeLabel::setValue( FileSize value, QLatin1String prefix )
{
    QLabel::setText( prefix % formatSize( value ) );
    setToolTip( value, prefix, 0 );
}


void FileSizeLabel::setValueWithLinks( FileSize size, nlink_t numLinks )
{
    QLabel::setText( formatSize( size ) % formatLinksInline( numLinks ) );
    setToolTip( size, QLatin1String{}, numLinks );
}


void FileSizeLabel::setToolTip( FileSize size, QLatin1String prefix, nlink_t numLinks )
{
    if ( size < 1000 ) // not useful below (rounded) 1 kB
	QLabel::setToolTip( QString{} );

    QLabel::setToolTip( whitespacePre( prefix % formatByteSize( size ) % formatLinksRichText( numLinks ) ) );
}


void FileSizeLabel::setBold()
{
    QFont textFont = font();
    textFont.setBold( true );
    setFont( textFont );
}

/*
void FileSizeLabel::suppressIfSameContent( FileSizeLabel * cloneLabel, QLabel * caption ) const
{
    if ( text() == cloneLabel->text() )
    {
	cloneLabel->clear();
	caption->setEnabled( false );
    }
    else
    {
	caption->setEnabled( true );
    }
}
*/
