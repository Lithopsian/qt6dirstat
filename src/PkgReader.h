/*
 *   File name: PkgReader.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PkgReader_h
#define PkgReader_h

#include <QProcess>
#include <QSharedPointer>

#include "DirReadJob.h"
#include "PkgInfo.h"


namespace QDirStat
{
    class DirTree;
    class PkgFileListCache;
    class PkgFilter;


    typedef QSharedPointer<PkgFileListCache> PkgFileListCachePtr;


    /**
     * A class for reading information about installed packages.
     *
     * This uses PkgQuery and PkgManager to read first the installed packages
     * and then one by one the file list for each of those packages in a read
     * job very much like DirReadJob.
     **/
    class PkgReader final
    {
    public:

        /**
         * Constructor. Even though this object creates a PkgReadJob for each
         * package it finds, it is not necessary to keep this reader around
         * longer than after read() is finished: once created and queued, the
         * PkgReadJobs are self-sufficient. They don't need this reader.
         **/
        PkgReader( DirTree * tree, const PkgFilter & filter ) { read( tree, filter); }


    protected:

        /**
         * Read installed packages from the system's package manager(s), select
         * those that match the specified filter and create a PkgReadJob for
         * each one to read its file list.
         *
         * Like all read jobs, this is done with a zero duration timer in the
         * Qt event loop, so whenever there is no user or X11 event to process,
         * it will pick one read job and execute it.
         **/
        void read( DirTree * tree, const PkgFilter & filter );

    };  // class PkgReader



    /**
     * Read job class for reading information about a package, based on
     * DirReadJob so that the standard DirTree job queue can be used.
     *
     * This is the base class for the asynchronous process-based
     * AsyncPkgReadJob and the file-cache-based CachePkgReadJob, with a
     * simplistic approach that just starts the external command
     * used for getting the file list when needed and then waits for it to
     * return a result.
     **/
    class PkgReadJob : public DirReadJob
    {
    public:

        /**
         * Constructor: prepare to read the file list of existing PkgInfo node
         * 'pkg' and create a DirInfo or FileInfo node for each item in the
         * file list below 'pkg'.

         * Reading is then started from the outside with startReading().
         **/
        PkgReadJob( DirTree * tree, PkgInfo * pkg ):
            DirReadJob{ tree, pkg },
            _pkg{ pkg }
        {}

        /**
         * Destructor.
         **/
        ~PkgReadJob() override = default;

        /**
         * Start reading the file list of the package.
         *
         * Reimplemented from DirReadJob.
         **/
        void startReading() override;

        /**
         * Return the parent PkgInfo node.
         **/
        PkgInfo * pkg() const { return _pkg; }


    protected:

        /**
         * Get the file list for this package.  This default implementation
         * does a simple PkgQuery::fileList() call.
         *
         * Derived classes might want to do something more sophisticated like
         * using a background process (AsyncPkgReader) or a file list cache
         * (CachePkgReader).
         **/
        virtual QStringList fileList() const = 0;

        /**
         * Obtain information about the file or directory specified in
         * 'path' and 'name' and create a new FileInfo or a DirInfo (whatever
         * is appropriate) from that information. Use FileInfo::isDirInfo()
         * to find out which.
         *
         * If the underlying syscall fails, a DirInfo object is created with
         * the relevant error read state and very limited information.
         **/
        FileInfo * addItem( const QString & path, const QString & name, DirInfo * parent );

        /**
         * Add all files belonging to 'path' to this package.
         * Create all directories as needed.
         **/
        void addFiles( QStringList fileList );


    private:

        PkgInfo * _pkg;

    };  // class PkgReadJob



    /**
     * Read job class for reading information about a package that uses a
     * number of background processes to parallelize all the external commands
     * ("rpm -ql", "dpkg -L", "pacman -Qlp") to speed up getting all the file
     * lists. This is considerably faster than doing that one by one and
     * waiting for the result each time (which is what the more generic
     * PkgReadJob does).
     **/
    class AsyncPkgReadJob final : public QObject, public PkgReadJob
    {
        Q_OBJECT

    public:

        /**
         * Constructor: Prepare to read the file list of existing PkgInfo node
         * 'pkg' and create a DirInfo or FileInfo node for each item in the
         * file list below 'pkg'. Operation starts when the
         * 'readFileListProcess' has data to read, i.e. when it sends
         * the 'readFileListFinished' signal.
         *
         * Create the readFileListProcess, then this read job, add the read job
         * to a DirReadJobQueue as a blocked job and then (!) start the
         * readFileListprocess. The job will unblock itself when it receives
         * file list data from the process so it will be put into the queue of
         * jobs that are ready to run.
         *
         * Reading is then started from the outside with startReading() when
         * the job is scheduled.
         **/
        AsyncPkgReadJob( DirTree   * tree,
                         PkgInfo   * pkg,
                         QProcess  * readFileListProcess );


    protected slots:

        /**
         * Notification that the attached read file list process is finished.
         **/
        void readFileListFinished( int exitCode, QProcess::ExitStatus exitStatus );


    protected:

        /**
         * Get the file list for this package.
         *
         * Reimplemented from PkgReadJob.
         **/
        QStringList fileList() const override { return _fileList; }


    private:

        QStringList   _fileList;

    };  // class AsyncPkgReadJob



    class CachePkgReadJob final : public PkgReadJob
    {
    public:

        /**
         * Constructor: Prepare to read the file list of existing PkgInfo node
         * 'pkg' and create a DirInfo or FileInfo node for each item in the
         * file list below 'pkg'. This uses 'fileListCache' to get the file
         * list.
         *
         * Create this type of job and add it as a normal job (not blocked,
         * unlike AsyncPkgReadJob) to the read queue.
         *
         * Reading is then started from the outside with startReading() when
         * the job queue picks this job.
         **/
        CachePkgReadJob( DirTree           * tree,
                         PkgInfo           * pkg,
                         PkgFileListCachePtr fileListCache ):
            PkgReadJob{ tree, pkg },
            _fileListCache{ fileListCache }
        {}


    protected:

        /**
         * Get the file list for this package.
         *
         * Reimplemented from PkgReadJob.
         **/
        QStringList fileList() const override;


    private:

        PkgFileListCachePtr _fileListCache;

    };  // class CachePkgReadJob

}       // namespace QDirStat

#endif  // ifndef PkgReader_h
