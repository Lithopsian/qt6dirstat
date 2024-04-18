/*
 *   File name: FileSizeLabel.cpp
 *   Summary:   Specialized QLabel for a file size for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMenu>
#include <QObject>

#include "FileSizeLabel.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "Logger.h"


using namespace QDirStat;


void FileSizeLabel::clear()
{
    QLabel::setToolTip( QString() );
    QLabel::clear();
}


void FileSizeLabel::setSize( const FileInfo * file )
{
    setValueWithLinks( file->rawByteSize(), file->links() );
}


void FileSizeLabel::setAllocated( const FileInfo * file )
{
    setValueWithLinks( file->rawAllocatedSize(), file->links() );

    setBold( file->isSparseFile() ||
             ( file->rawAllocatedSize() > 4096 && file->usedPercent() < ALLOCATED_FAT_PERCENT ) );
}


void FileSizeLabel::setValue( FileSize value, const QString & prefix )
{
    QLabel::setText( prefix + formatSize( value ) );
    setToolTip( value, prefix, 0 );
}


void FileSizeLabel::setValueWithLinks( FileSize size, nlink_t numLinks )
{
    const QString & text = formatSize( size );
    QLabel::setText( text + formatLinksInline( numLinks ) );
    setToolTip( size, "", numLinks );
}


void FileSizeLabel::setToolTip( FileSize size, const QString & prefix, nlink_t numLinks )
{
    if ( size < 1000 ) // not useful below (rounded) 1 kB
	QLabel::setToolTip( QString() );

    QLabel::setToolTip( whitespacePre( prefix + formatByteSize( size ) + formatLinksRichText( numLinks ) ) );
}


void FileSizeLabel::setText( const QString & text )
{
    QLabel::setText( text );
    QLabel::setToolTip( QString() );
}


void FileSizeLabel::setBold( bool bold )
{
    QFont textFont = font();
    textFont.setBold( bold );
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
