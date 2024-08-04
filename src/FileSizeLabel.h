/*
 *   File name: FileSizeLabel.h
 *   Summary:   Specialized QLabel for a file size for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileSizeLabel_h
#define FileSizeLabel_h

#include <QLabel>

#include "Typedefs.h" // FileSize


#define ALLOCATED_FAT_PERCENT	33


namespace QDirStat
{
    class FileInfo;

    /**
     * Widget to display a file size in human readable form (i.e. "123.4 MB")
     * and with a context menu that displays the exact byte size.
     *
     * This is just a thin wrapper around PopupLabel / QLabel.
     **/
    class FileSizeLabel: public QLabel
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	FileSizeLabel( QWidget * parent = nullptr ):
	    QLabel { parent }
	{}

	/**
	 * Set the text of a file size label including special handling for
	 * sparse files and files with multiple hard links.
	 **/
	void setSize( const FileInfo * file );

	/**
	 * Set the text of an allocated size label including special handling
	 * for sparse files and files with multiple hard links.
	 *
	 * Note that this is only useful for plain files, not for directories,
	 * packages or multiple selected files.
	 **/
	void setAllocated( const FileInfo * file );

	/**
	 * Set the label text and tooltip. This will format the value and display
	 * it in human-readable format, i.e. something like "123.4 MB".  Values of
	 * zero or -1 will be formatted as an empty string.
	 *
	 * 'prefix' is an optional text prefix like "> " to indicate that the
	 * exact value is unknown (e.g. because of insuficcient permissions in
	 * a directory tree).
	 *
	 * If the value is more than 1024, the label is given a tooltip containing
	 * the exact value in bytes.
	 **/
	void setValue( FileSize value, QLatin1String prefix );

	/**
	 * Set the label text and tooltip.  The label string is formatted in a
	 * human-readable format, including the number of hard links (only when there
	 * is more than one hard link).
	 **/
	void setValueWithLinks( FileSize size, nlink_t numLinks );

	/**
	 * Set the tooltip for a value.  The value will be formatted as the exact
	 * number of bytes with the unit "bytes".  For values below 1000 bytes (will
	 * appear as 1.0kB), no tooltip will be shown since the exact number of bytes
	 * are already visible.  The tooltip may have a prefix (eg. ">") or it may
	 * have jard links, but it should never have both.
	 **/
	void setToolTip( FileSize size, QLatin1String prefix, nlink_t numLinks );

	/**
	 * Set a custom text. This text may or may not contain the value.  The
	 * tooltip is disabled.
	 **/
	void setText( const QString & text );

	/**
	 * Set the label font to bold.
	 **/
	void setBold( bool bold );

        /**
         * Suppress the content of FileSizeLabel 'cloneLabel' if it has the
         * same content as this label: clear its text and disable its caption
         * 'caption'.
         **/
//        void suppressIfSameContent( FileSizeLabel * cloneLabel, QLabel * caption ) const;

	/**
	 * Clear everything, including the visible text, the numeric value,
	 * the context menu text and the bold font.
	 **/
	void clear();

    }; // class FileSizeLabel

} // namespace QDirStat

#endif // FileSizeLabel_h
