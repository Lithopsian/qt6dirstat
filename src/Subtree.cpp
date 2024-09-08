/*
 *   File name: Subtree.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "Subtree.h"
#include "DirInfo.h"
#include "DirTree.h"


using namespace QDirStat;


FileInfo * Subtree::subtree() const
{
    if ( !_tree )
	return nullptr;

    FileInfo * item = _url.isEmpty() ? nullptr : _tree->locate( _url );

    if ( !item && _useParentFallback && !_parentUrl.isEmpty() )
	item = _tree->locate( _parentUrl );

    if ( !item && _useRootFallback )
	item = _tree->firstToplevel();

    return item;
}


const DirInfo * Subtree::dir() const
{
    FileInfo * item = subtree();
    if ( !item )
        return nullptr;

    const DirInfo * dir = item->toDirInfo();

    if ( !dir && item->parent() )
        dir = item->parent();

    if ( dir && _tree && dir == _tree->root() )
        dir = nullptr;

    return dir;
}


const QString & Subtree::url() const
{
    if ( _tree && _url == _tree->rootDebugUrl() )
	return _tree->url();

    return _url;
}


void Subtree::set( FileInfo * fileInfo )
{
    _parentUrl.clear();

    if ( fileInfo )
    {
	_tree = fileInfo->tree();
	_url  = fileInfo->debugUrl();

	if ( fileInfo->parent() )
            _parentUrl = fileInfo->parent()->debugUrl();
    }
    else
    {
	_url.clear();
    }
}
