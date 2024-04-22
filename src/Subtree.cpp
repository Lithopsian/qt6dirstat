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
//#include "Logger.h"


using namespace QDirStat;


FileInfo * Subtree::subtree() const
{
    FileInfo * item = locate();

    if ( !item && _useParentFallback && _tree && !_parentUrl.isEmpty() )
	item = _tree->locate( _parentUrl, true ); // findPseudoDirs

    if ( !item && _useRootFallback && _tree )
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
    if ( _url == "<root>" && _tree )
	return _tree->url();
    else
	return _url;
}


void Subtree::set( FileInfo * subtree )
{
    _parentUrl.clear();

    if ( subtree )
    {
	_tree = subtree->tree();
	_url  = subtree->debugUrl();

	if ( subtree->parent() )
            _parentUrl = subtree->parent()->debugUrl();
    }
    else
    {
	_url.clear();
    }
}


FileInfo * Subtree::locate() const
{
    if ( !_tree || _url.isEmpty() )
	return nullptr;

    return _tree->locate( _url,
			  true ); // findPseudoDirs
}

/*
void Subtree::clone( const Subtree & other )
{
    if ( &other == this )
	return;

    _tree = other.tree();
    _url  = other.url();
    _parentUrl  = other.parentUrl();

    _useRootFallback   = other.useRootFallback();
    _useParentFallback = other.useParentFallback();
}
*/
