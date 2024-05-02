/*
 *   File name: FileInfoSet.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "FileInfoSet.h"
#include "DirTree.h"
#include "DirInfo.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


bool FileInfoSet::containsAncestorOf( FileInfo * item ) const
{
    while ( item )
    {
	item = item->parent();

	if ( contains( item ) )
	    return true;
    }

    return false;
}


FileInfoSet FileInfoSet::normalized() const
{
    FileInfoSet normalized;

    for ( FileInfo * item : *this )
    {
	if ( !containsAncestorOf( item ) )
	    normalized << item;
#if 0
	else
	    logDebug() << "Removing " << item << " with ancestors in the set" << Qt::endl;
#endif
    }

    return normalized;
}


FileInfoSet FileInfoSet::invalidRemoved() const
{
    FileInfoSet result;

    for ( FileInfo * item : *this )
    {
	if ( item && item->checkMagicNumber() )
	{
	    // logDebug() << "Keeping " << item << Qt::endl;
	    result << item;
	}
	else
	{
	    logWarning() << "Removing invalid item" << Qt::endl;
	}
    }

    return result;
}


bool FileInfoSet::containsPseudoDir() const
{
    for ( const FileInfo * item : *this )
    {
	if ( item && item->isPseudoDir() )
	    return true;
    }

    return false;
}


bool FileInfoSet::containsDotEntry() const
{
    for ( const FileInfo * item : *this )
    {
	if ( item && item->isDotEntry() )
	    return true;
    }

    return false;
}


bool FileInfoSet::containsAttic() const
{
    for ( const FileInfo * item : *this )
    {
	if ( item && item->isAttic() )
	    return true;
    }

    return false;
}


bool FileInfoSet::containsDir() const
{
    for ( const FileInfo * item : *this )
    {
	if ( item && item->isDir() )
	    return true;
    }

    return false;
}


bool FileInfoSet::containsFile() const
{
    for ( const FileInfo * item : *this )
    {
	if ( item && item->isFile() )
	    return true;
    }

    return false;
}


bool FileInfoSet::containsSpecial() const
{
    for ( const FileInfo * item : *this )
    {
	if ( item && item->isSpecial() )
	    return true;
    }

    return false;
}


bool FileInfoSet::containsPkg() const
{
    for ( const FileInfo * item : *this )
    {
	if ( item && item->isPkgInfo() )
	    return true;
    }

    return false;
}


bool FileInfoSet::containsBusyItem() const
{
    for ( const FileInfo * item : *this )
    {
	if ( item && item->isBusy() )
	    return true;
    }

    return false;
}


bool FileInfoSet::treeIsBusy() const
{
    if ( isEmpty() )
	return false;

    return first()->tree()->isBusy();
}


FileSize FileInfoSet::totalSize() const
{
    FileSize sum = 0LL;

    for ( FileInfo * item : *this )
    {
	if ( item )
	    sum += item->totalSize();
    }

    return sum;
}


FileSize FileInfoSet::totalAllocatedSize() const
{
    FileSize sum = 0LL;

    for ( FileInfo * item : *this )
    {
	if ( item )
	    sum += item->totalAllocatedSize();
    }

    return sum;
}


FileInfoSet FileInfoSet::parents() const
{
    FileInfoSet parents;

    for ( FileInfo * child : *this )
    {
	if ( child && child->parent() )
        {
            FileInfo * parent = child->parent();

            if ( parent->isPseudoDir() )
                parent = parent->parent();

            if ( parent )
                parents << parent;
        }
    }

    return parents.normalized();
}

