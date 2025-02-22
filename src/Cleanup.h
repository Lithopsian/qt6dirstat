/*
 *   File name: Cleanup.h
 *   Summary:   QDirStat classes to reclaim disk space
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef Cleanup_h
#define Cleanup_h

#include <QAction>
#include <QTextStream>


namespace QDirStat
{
    class FileInfo;
    class FileInfoSet;
    class OutputWindow;

    /**
     * Cleanup action to be performed for DirTree items.
     **/
    class Cleanup final : public QAction
    {
	Q_OBJECT

    public:

	enum RefreshPolicy
	{
	    NoRefresh,
	    RefreshThis,
	    RefreshParent,
	    AssumeDeleted
	};

	enum OutputWindowPolicy
	{
	    ShowAlways,
	    ShowIfErrorOutput,
	    ShowAfterTimeout,  // this includes ShowIfErrorOutput
	    ShowNever
	};


	/**
	 * Constructor with nearly all fields (the icon and hotkey
	 * must be set with separate calls).
	 *
	 * 'title' is the human readable menu title.
	 * 'command' is the shell command to execute.
	 **/
	Cleanup( QObject            * parent,
	         bool                 active,
	         const QString      & title,
	         const QString      & command,
	         bool                 recurse,
	         bool                 askForConfirmation,
	         RefreshPolicy        refreshPolicy,
	         bool                 worksForDir,
	         bool                 worksForFile,
	         bool                 worksForDotEntry,
	         OutputWindowPolicy   outputWindowPolicy = ShowAfterTimeout,
	         int                  outputWindowTimeout = 500,
	         bool                 outputWindowAutoClose = false,
	         QString              shell = QString{} ):
	    QAction{ title, parent },
	    _active{ active },
	    _title{ title },
	    _command{ command },
	    _recurse{ recurse },
	    _askForConfirmation{ askForConfirmation },
	    _refreshPolicy{ refreshPolicy },
	    _worksForDir{ worksForDir },
	    _worksForFile{ worksForFile },
	    _worksForDotEntry{ worksForDotEntry },
	    _outputWindowPolicy{ outputWindowPolicy },
	    _outputWindowTimeout{ outputWindowTimeout },
	    _outputWindowAutoClose{ outputWindowAutoClose },
	    _shell{ shell }
	{}

	/**
	 * Default constructor.  Used by the config dialog to create an empty cleanup
	 * with the default settings and no parent.
	 **/
	Cleanup():
	    Cleanup{ nullptr, true, QString{}, QString{}, false, false, RefreshThis, true, true, false }
	{}

	/**
	 * The copy constructor is deleted by Qt, but make it possible to
	 * provide a copy without being a real action so that the config
	 * dialog can play about with it.
	 **/
	Cleanup( const Cleanup * other ):
	    Cleanup{ nullptr, other->_active, other->_title, other->_command,
	             other->_recurse, other->_askForConfirmation, other->_refreshPolicy,
	             other->_worksForDir, other->_worksForFile, other->_worksForDotEntry,
	             other->_outputWindowPolicy, other->_outputWindowTimeout, other->_outputWindowAutoClose,
	             other->_shell }
	{
	    setIcon( other->iconName() );     // carried on the Cleanup as QString and QAction as QIcon
	    setShortcut( other->shortcut() ); // only carried on the underlying QAction
	}

	/**
	 * Return the command line that will be executed upon calling
	 * Cleanup::execute(). This command line may contain %p for the
	 * complete path of the directory or file concerned or %n for the pure
	 * file or directory name without path.
	 **/
	const QString & command() const { return _command; }

	/**
	 * Return the user title of this command as displayed in menus.
	 * This may include '&' characters for keyboard shortcuts.
	 * See also cleanTitle() .
	 **/
	const QString & title() const { return _title; }

	/**
	 * Return the cleanup action's title without '&' keyboard shortcuts.
	 * Uses the ID as fallback if the name is empty.
	 **/
	QString cleanTitle() const
	    { return _title.isEmpty() ? _command : QString{ _title }.remove( u'&' ); }

	/**
	 * Return the icon for this cleanup action.
	 **/
	const QIcon icon() const { return QAction::icon(); }

	/**
	 * Return the icon name of this cleanup action.
	 **/
	const QString & iconName() const { return _iconName; }

	/**
	 * Return whether or not this cleanup action is generally active.
	 **/
	bool isActive() const { return _active; }

	/**
	 * Return whether or not this cleanup action works for this particular
	 * FileInfo. Checks all the other conditions (active(),
	 * worksForDir(), worksForFile(), ...) accordingly.
	 **/
	bool worksFor( FileInfo * item ) const;

	/**
	 * Return whether or not this cleanup action works for directories,
	 * i.e. whether or not Cleanup::execute() will be successful if the
	 * object passed is a directory.
	 **/
	bool worksForDir() const { return _worksForDir; }

	/**
	 * Return whether or not this cleanup action works for plain files.
	 **/
	bool worksForFile() const { return _worksForFile; }

	/**
	 * Return whether or not this cleanup action works for QDirStat's
	 * special '<Files>' items, i.e. the pseudo nodes created in most
	 * directories that hold the plain files.
	 **/
	bool worksForDotEntry() const { return _worksForDotEntry; }

	/**
	 * Return whether or not the cleanup action should be performed
	 * recursively in subdirectories of the initial FileInfo.
	 **/
	bool recurse() const { return _recurse; }

	/**
	 * Return whether or not this cleanup should ask the user for
	 * confirmation when it is executed.
	 *
	 * The default is 'false'. Use with caution - not only can this become
	 * very annoying, people also tend to automatically click on 'OK' when
	 * too many confirmation dialogs pop up!
	 **/
	bool askForConfirmation() const { return _askForConfirmation; }

	/**
	 * Return the shell to use to invoke the command of this cleanup.
	 * If this is is empty, use defaultShells().first().
	 *
	 * Regardless of which shell is used, the command is always started
	 * with the shell and the "-c" option.
	 **/
	const QString & shell() const { return _shell; }

	/**
	 * Return the refresh policy of this cleanup action - i.e. the action
	 * to perform after each call to Cleanup::execute(). This is supposed
	 * to bring the corresponding DirTree back into sync after the cleanup
	 * action - the underlying file tree might have changed due to that
	 * cleanup action.
	 *
	 * NoRefresh: Don't refresh anything. Assume nothing has changed.
	 * This is the default.
	 *
	 * RefreshThis: Refresh the DirTree from the item on that was passed
	 * to Cleanup::execute().
	 *
	 * RefreshParent: Refresh the DirTree from the parent of the item on
	 * that was passed to Cleanup::execute(). If there is no such parent,
	 * refresh the entire tree.
	 *
	 * AssumeDeleted: Do not actually refresh the DirTree.	Instead,
	 * blindly assume the cleanup action has deleted the item that was
	 * passed to Cleanup::execute() and delete the corresponding subtree
	 * in the DirTree accordingly. This will work well for most deleting
	 * actions as long as they can be performed without problems. If there
	 * are any problems, however, the DirTree might easily run out of sync
	 * with the directory tree: The DirTree will show the subtree as
	 * deleted (i.e. it will not show it any more), but it still exists on
	 * disk. This is the tradeoff to a very quick response. On the other
	 * hand, the user can easily at any time hit one of the explicit
	 * refresh buttons and everything will be back into sync again.
	 **/
	enum RefreshPolicy refreshPolicy() const { return _refreshPolicy; }

	/**
	 * Return whether the refresh policy requires a refresh, either of
	 * the item itself or its parent,
	 **/
	bool requiresRefresh() const
	    { return _refreshPolicy == RefreshThis || _refreshPolicy == RefreshParent; }

	/**
	 * Return the policy when an output window (see also OutputWindow) for
	 * this clean action is shown. Since cleanup actions start shell
	 * commands, the output of those shell commands might be important,
	 * especially if they report an error. In addition to that, if a
	 * cleanup action takes a while, it might be a good idea to show the
	 * user what is going on. Note that there will always be only one
	 * output window for all cleanup tasks that are to be started in one
	 * user action; if multiple items are selected, the corresponding
	 * command will be started for each of the selected items individually
	 * one after another, but the output window will remain open and
	 * collect the output of each one. Likewise, if a command is recursive,
	 * it is started for each directory level, and the output is also
	 * collected in the same output window.
	 *
	 * Possible values:
	 *
	 * ShowAlways: Always open an output window. This makes sense for
	 * cleanup actions that take a while, like compressing files, recoding
	 * videos, recompressing JPG images.
	 *
	 * ShowIfErrorOutput: Leave the output window hidden, but open it if
	 * there is any error output (i.e. output on its stderr channel). This
	 * is useful for most cleanup tasks that are typically quick, but that
	 * might also go wrong - for example, due to insufficient permissions
	 * in certain directories.
	 *
	 * ShowAfterTimeout: (This includes ShowIfErrorOutput) Leave the output
	 * window hidden for a certain timeout (3 seconds by default), but open
	 * it if it takes any longer than that. If there is error output, it is
	 * opened immediately. This is the default and recommended setting. The
	 * output can be configured with setOutputWindowTimeout().
	 *
	 * ShowNever: Never show the output window, no matter how long the
	 * cleanup task takes or if there is any amount of error output, or
	 * even if the cleanup process crashes or could not be started.
	 **/
	enum OutputWindowPolicy outputWindowPolicy() const { return _outputWindowPolicy; }

	/**
	 * Return the timeout (in milliseconds) for the ShowAfterTimeout output
	 * window policy. The default is 0 which means to use the
	 * OutputWindow dialog class default.
	 **/
	int outputWindowTimeout() const { return _outputWindowTimeout; }

	/**
	 * Return 'true' if the output window is closed automatically when the
	 * cleanup task is done and there was no error.
	 **/
	bool outputWindowAutoClose() const { return _outputWindowAutoClose; }

	/**
	 * The heart of the matter: perform the cleanup with the FileInfo
	 * specified.
	 *
	 * 'outputWindow' is the optional dialog to watch the commands and
	 * their stdout and stderr output as they are executed.
	 **/
	void execute( FileInfo * item, OutputWindow * outputWindow );

	/**
	 * From a FileInfoSet, create a list of unique items based
	 * on the variables for this cleanup.  For %d (directories),
	 * this will be a set of any directoiry items and the parents
	 * of any file items.  This avoids opening multiple Cleanups
	 * to act on the same parent, which may already be gone.
	 **/
	FileInfoSet deDuplicateParents( const FileInfoSet & sel );

	/**
	 * Comparison operator for two Cleanups for the purposes of
	 * the config dialog.
	 **/
	bool operator!=( const Cleanup & other ) const;

	/**
	 * Setters (see the corresponding getter for documentation), mainly
	 * for the config page.
	 **/
	void setTitle                ( const QString    & title     ) { QAction::setText( _title = title ); }
	void setActive               ( bool               active    ) { _active                = active;    }
	void setCommand              ( const QString    & command   ) { _command               = command;   }
	void setRecurse              ( bool               recurse   ) { _recurse               = recurse;   }
	void setAskForConfirmation   ( bool               ask       ) { _askForConfirmation    = ask;       }
	void setRefreshPolicy        ( RefreshPolicy      policy    ) { _refreshPolicy         = policy;    }
	void setWorksForDir          ( bool               canDo     ) { _worksForDir           = canDo;     }
	void setWorksForFile         ( bool               canDo     ) { _worksForFile          = canDo;     }
	void setWorksForDotEntry     ( bool               canDo     ) { _worksForDotEntry      = canDo;     }
	void setOutputWindowPolicy   ( OutputWindowPolicy policy    ) { _outputWindowPolicy    = policy;    }
	void setOutputWindowTimeout  ( int                timeout   ) { _outputWindowTimeout   = timeout;   }
	void setOutputWindowAutoClose( bool               autoClose ) { _outputWindowAutoClose = autoClose; }
	void setShell                ( const QString    & sh        ) { _shell                 = sh;        }
	void setIcon                 ( const QString    & iconName  )
	    { QAction::setIcon( QIcon::fromTheme( _iconName = iconName ) ); }


    protected:

	/**
	 * Return the full paths to the available (and executable) shells:
	 *     loginShell()	($SHELL)
	 *     /bin/bash
	 *     /bin/sh
	 **/
	static const QStringList & defaultShells();

	/**
	 * Return the first default shell or an empty string if there is no
	 * usable shell at all.
	 **/
	static QString defaultShell()
	    { return defaultShells().isEmpty() ? QString{} : defaultShells().first(); }

	/**
	 * Choose a suitable shell. Try this->shell() and fall back to
	 * defaultShell(). Return an empty string if no usable shell is found.
	 **/
	QString chooseShell( OutputWindow * outputWindow ) const;


    private:

	bool               _active;
	QString            _title;
	QString            _command;
	QString            _iconName;
	bool               _recurse;
	bool               _askForConfirmation;
	RefreshPolicy      _refreshPolicy;
	bool               _worksForDir;
	bool               _worksForFile;
	bool               _worksForDotEntry;
	OutputWindowPolicy _outputWindowPolicy;
	int                _outputWindowTimeout;
	bool               _outputWindowAutoClose;
	QString            _shell;

    };	// Cleanup



    inline QTextStream & operator<<( QTextStream & stream, const Cleanup * cleanup )
    {
	if ( cleanup )
	    stream << cleanup->cleanTitle();
	else
	    stream << "<NULL Cleanup *>";

	return stream;
    }

}	// namespace QDirStat

#endif	// ifndef Cleanup_h

