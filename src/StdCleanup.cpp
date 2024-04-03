/*
 *   File name: StdCleanup.cpp
 *   Summary:	QDirStat classes to reclaim disk space
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "CleanupCollection.h"
#include "StdCleanup.h"
#include "Cleanup.h"
#include "Exception.h"
#include "Logger.h"

#define USE_DEBUG_ACTIONS 0


using namespace QDirStat;


static Cleanup * openFileManagerHere( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "Open File Mana&ger Here" ),
				    "%filemanager",
				    false,
				    false,
				    Cleanup::NoRefresh,
				    true,
				    true,
				    true );
    CHECK_NEW( cleanup );
    cleanup->setIcon( ":/icons/file-manager.png" );
    cleanup->setShortcut( Qt::CTRL | Qt::Key_G );

    return cleanup;
}


static Cleanup * openTerminalHere( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "Open &Terminal Here" ),
				    "%terminal",
				    false,
				    false,
				    Cleanup::NoRefresh,
				    true,
				    true,
				    true );
    CHECK_NEW( cleanup );
    cleanup->setIcon( ":/icons/terminal.png" );
    cleanup->setShortcut( Qt::CTRL | Qt::Key_T );

    return cleanup;
}


static Cleanup * checkFileType( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "Check File T&ype" ),
				    "file %n | sed -e 's/[:,] /\\n  /g'",
				    false,
				    false,
				    Cleanup::NoRefresh,
				    false,
				    true,
				    false,
				    Cleanup::ShowAlways );
    CHECK_NEW( cleanup );
    cleanup->setShortcut( Qt::CTRL | Qt::Key_Y );

    return cleanup;
}


static Cleanup * compressSubtree( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "&Compress" ),
				    "cd ..; tar cjvf %n.tar.bz2 %n && rm -rf %n",
				    false,
				    false,
				    Cleanup::RefreshParent,
				    true,
				    false,
				    false );
    CHECK_NEW( cleanup );

    return cleanup;
}


static Cleanup * makeClean( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "&make clean" ),
				    "make clean",
				    false,
				    false,
				    Cleanup::RefreshThis,
				    true,
				    false,
				    true );
    CHECK_NEW( cleanup );

    return cleanup;
}


static Cleanup * gitClean( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "&git clean" ),
				    "git clean -dfx",
				    false,
				    true,
				    Cleanup::RefreshThis,
				    true,
				    false,
				    true,
				    Cleanup::ShowAlways );
    CHECK_NEW( cleanup );

    return cleanup;
}


static Cleanup * deleteJunk( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "Delete &Junk Files" ),
				    "rm -f *~ *.bak *.auto core",
				    true,
				    false,
				    Cleanup::RefreshThis,
				    true,
				    false,
				    true );
    CHECK_NEW( cleanup );
    cleanup->setShell( "/bin/bash" );

    return cleanup;
}


static Cleanup * hardDelete( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "&Delete (no way to undelete!)" ),
				    "rm -rf %p",
				    false,
				    true,
				    Cleanup::RefreshParent,
				    true,
				    true,
				    false );
    CHECK_NEW( cleanup );
    cleanup->setIcon( ":/icons/delete.png" );
    cleanup->setShortcut( Qt::CTRL | Qt::Key_Delete );

    return cleanup;
}


static Cleanup * clearDirContents( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "Clear Directory C&ontents" ),
				    "rm -rf %d/*",
				    false,
				    true,
				    Cleanup::RefreshThis,
				    true,
				    false,
				    false );
    CHECK_NEW( cleanup );

    return cleanup;
}


#if USE_DEBUG_ACTIONS

static Cleanup * echoargs( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "echoargs" ),
				    "echoargs %p",
				    false,
				    false,
				    Cleanup::NoRefresh,
				    true,
				    true,
				    true );
    CHECK_NEW( cleanup );

    return cleanup;
}


static Cleanup * echoargsMixed( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "Output on stdout and stderr" ),
				    "echoargs_mixed %n one two three four",
				    false,
				    true,
				    Cleanup::NoRefresh,
				    true,
				    true,
				    true );
    CHECK_NEW( cleanup );

    return cleanup;
}


static Cleanup * segfaulter( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "Segfaulter" ),
				    "segfaulter",
				    false,
				    false,
				    Cleanup::NoRefresh,
				    true,
				    true,
				    true );
    CHECK_NEW( cleanup );

    return cleanup;
}


static Cleanup * commandNotFound( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "Nonexistent command" ),
				    "wrglbrmpf",
				    false,
				    false,
				    Cleanup::NoRefresh,
				    true,
				    true,
				    true );
    CHECK_NEW( cleanup );

    return cleanup;
}


static Cleanup * sleepy( QObject * parent )
{
    Cleanup *cleanup = new Cleanup( parent,
				    true,
				    QObject::tr( "Sleepy echoargs" ),
				    "sleep 1; echoargs %p",
				    false,
				    false,
				    Cleanup::NoRefresh,
				    true,
				    true,
				    true );
    CHECK_NEW( cleanup );

    return cleanup;
}

#endif



CleanupList StdCleanup::stdCleanups( QObject * parent )
{
    return { openFileManagerHere( parent ),
	     openTerminalHere   ( parent ),
	     checkFileType      ( parent ),
	     compressSubtree	( parent ),
	     makeClean		( parent ),
	     gitClean		( parent ),
	     deleteJunk		( parent ),
	     hardDelete		( parent ),
	     clearDirContents	( parent ),
#if USE_DEBUG_ACTIONS
	     echoargs		( parent ),
	     echoargsMixed	( parent ),
	     segfaulter		( parent ),
	     commandNotFound	( parent ),
	     sleepy		( parent ),
#endif
	   };
}
