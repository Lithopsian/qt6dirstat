# Qt6DirStat Screenshots

Qt-based directory statistics.

(c) 2015-2021 Stefan Hundhammer Stefan.Hundhammer@gmx.de
and 2023-2024 Ian Nartowicz

Target Platforms: Linux, BSD, Unix-like systems

License: GPL V2


## QDirStat Main Window

![Main Window Screenshot](QDirStat-main-win.png)


### Different Main Window Layouts - all columns are configurable as well as the directory breadcrumbs
and file details panel.  Choose your own settings for each one.

![Layout 1 (short)](QDirStat-details-file-L1.png)
Layout 1 (short): typically a bare minimum set of columns plus the file details panel.

![Layout 2 (classic)](QDirStat-details-file-L2.png)
Layout 2 (classic): the classic set QDirStat tree columns plus the details panel for the selected item.

![Layout 3 (full)](QDirStat-details-file-L3.png)
Layout 3 (full): All or most tree columns including file owner, group and permissions in both "rwxrwxrwx"
and octal formats, but no details panel to make room for all the columns.


### Details Panel

![Details Panel for a File](QDirStat-details-file-L2.png)

File selected. Note the "Package" field: for "system" files, Qt6DirStat queries
the package manager (dpkg, rpm, or pacman) to find which package the file belongs
to.

![Details Panel for a Directory](QDirStat-details-dir.png)

Directory selected. The size of the directory itself is shown as well as the
total size of its contents.

![Details Panel for a "Files" Pseudo Directory](QDirStat-details-dot-entry.png)

"Files" pseudo directory selected.  This is a virtual container for all the files
in a directory so they are shown separately from any child directories.

![Details Panel for Multiple Selected Items](QDirStat-details-multi-sel.png)

Multiple items selected.  This may be a combination of directories and files,
with the total referring to the common selected ancestor, so that selecting a
directory and all of its contents will show a selected count of 1.


## Open Directory Dialog

![Custom Open Directory Dialog Screenshot](QDirStat-open-dir-dialog.png)

Custom "Open Directory" dialog with quick access to the home directory and
regular mounted filesystems. The "Cross Filesystems" check box here is a temporary
override (just for this program run) of the global configuration setting.


## Output During Cleanup Actions

![Cleanup Action Output Screenshot](QDirStat-cleanup-output.png)

Screenshot of output during cleanup actions. This window is optional, and can
be configured separately for each Cleanup action.  It is generally shown when
an error occurs, but can be completely disabled for Cleanups that produce
unhelpful output.


## Error Reporting

![Reporting Directory Read Errors Screenshot](QDirStat-err-dirs-light.png)

Reporting errors during directory reading. Typically this is because of
missing permissions.

![Details about Directory Read Errors](QDirStat-unreadable-dirs-window.png)

Clicking on the "Details..." link opens a separate window that lists all the
directories that could not be read. This window is non-modal, i.e. the user can
interact with the main window while it is open. A click on any directory in
that list locates that directory in the main window, i.e. opens all parent
branches and scrolls to ensure thae selected directory is visible.


## Discovering Files

![Discovering Files](QDirStat-discover.png)

Discovering files of certain categories.  Clicking a file in the list selects
it in the tree view in the main window.


## Packages View

![Packages View Screenshot](QDirStat-pkg-details.png)

Packages view: shows all installed packages, with the files grouped by package.


![Packages Summary Screenshot](QDirStat-pkg-summary.png)

Packages Summary shows the total number of packages read.


![Packages View Screenshot](QDirStat-pkg-emacs.png)

Packages view limited to package names starting with "emacs".


!["Open Packages" Dialog Screenshot](QDirStat-open-pkg-dialog.png)

Dialog for selecting which packages to display.

To avoid the initial "Choose Directory" dialog, start QDirStat with the "-d"
or "--dont-ask" command line parameter (or simply click "Cancel" in the "Choose
Directory" dialog).  Packages can also be read directly from the command line
by specifying "QDirStat Pkg:/", optionally with a package wildcard such as "Pkg:/emacs".


## Unpackaged Files View

![Unpackaged Files Screenshot](QDirStat-unpkg-usr-share-qt5.png)

Unpackaged files view: files in selected directories that are not part of
any package.  Directories that are not expected to contain packaged files
can be excluded from the tree, as well as specific types of files that are
commonly not in a package but are generated when a package is installed
(eg. \*.pyc Python files).

The greyed-out ignored directories contain the files that are known to be
in an installed package.


![Unpackaged Files Screenshot](QDirStat-unpkg-boot-grub2.png)

Unpackaged files view: here there is a Grub font that is not from a package.


!["Show Unpackaged Files" Dialog Screenshot](QDirStat-show-unpkg-dialog.png)

Dialog to start the "unpackaged files" view (Menu "File" -> "Show Unpackaged
Files"). Some directories are excluded by default because they are expected to
contain a lot of unpackaged files. You can add more directories or remove
predefined ones.

Since Python tends to leave behind so many unpackaged files, all `*.pyc` files
are ignored by default. You can add more wildcard patterns to ignore or remove
the predefined one.  The Unpackaged view can also be shown directly from the
command line by "QDirStat Unpkg:/".


## File Size Statistics


![File Size Histogram Screenshot](QDirStat-histogram.png)

File size histogram for a directory

![Histogram with only JPGs](QDirStat-histogram-jpg-work.png)

File size histogram for all files of type .jpg (start from the File Type Statistics window)

![Histogram Options](QDirStat-histogram-options.png)

Histogram settings can be tweaked

![Logarithmic Scale](QDirStat-histogram-log-scale.png)

Logarithmic scale for the vertical axis if needed

![Histogram with P100](QDirStat-histogram-P100.png)

Degenerated histogram if the last percentiles are too far spread out

![Histogram with P99](QDirStat-histogram-P99.png)

Just one percentile less makes all the difference: Scaled down from P100 to P99



![Percentiles Table](QDirStat-percentiles-table.png)

Numeric percentiles table

![Full Percentiles Table](QDirStat-percentiles-table-full.png)

Full percentiles table

![Extreme Percentiles](QDirStat-percentiles-extreme.png)

Notice the leap from P99 to P100


![Buckets Table](QDirStat-buckets-table.png)

Buckets table (information also available by tooltips in the histogram)


![File Size Help](QDirStat-stats-help.png)

Dedicated help start page for file size statistics


## File Age Statistics

![File Age Statistics: Years](QDirStat-file-age-years.png)

This window (opened fom the menu with View -> Show File Age Statistics) shows
the modification time (mtime) of files (not directories) in the selected
subtree broken down by years; i.e., it shows in what year the files were last
modified.

For each year, it shows the number of files that were last modified in that
year, the percent of that number relative to all files in that subtree both as
a percentage bar and as a number, the total size of those files and the percent
of that total size relative to the total size of that subtree, again as a
percentage bar and as a number.


![File Age Statistics: Years and Months](QDirStat-file-age-months.png)

File age statistics with the months of the current year expanded. It goes into
finer detail for the months of the current year and the year before, i.e. the
most recent 13-24 months. The percentages in the months are relative to that
year, not to the complete subtree.

Notice that the months breakdown is always only displayed for the current year
and the year before, no matter in what year any activity in that subtree
starts.


![File Age Statistics: Some Years Ago](QDirStat-file-age-long-ago.png)

File age statistics for a subtree where there was no activity at all for the
last couple of years. Notice how the years up to that first active year are
also displayed, but greyed out: You can instantly see that the last activity
was some years ago, even without looking at the year numbers.



## File Type Statistics

![File Type Statistics Window Screenshot](QDirStat-file-type-stats.png)

Menu View -> File Type Statistics...


## Locating Files by Type

![Locate Files Window](QDirStat-locate-file-types-window.png)

Results after clicking on the "Locate" button in the "File Type Statistics" window.

![Locating Files](QDirStat-locating-file-types.png)

Locating files with a specific extension. That branch is automatically opened
in the tree view in the main window, and all matching files in that directory are
selected.


## Mounted Filesystems Window

![Custom Open Directory Dialog Screenshot](QDirStat-filesystems-window.png)

This window is opened from the menu with View -> Show Mounted Filesystems) and
shows the currently mounted filesystems. System mounts like /dev, /proc, /sys,
and also bind mounts and multiple mounts of the same filesystem are only shown
if the checkbox is unticked.

The "Reserved" column shows the amount of disk space that is reserved for the
root user; "Free" is what is available for non-privileged users.  Information reported
for BTRFS filesystems may not be accurate.




## Configuration


![General Options Configuration Screenshot](QDirStat-config-general.png)

Screenshot of the general (miscellaneous) configuration options.

![MIME Categories Configuration Screenshot](QDirStat-config-mime.png)

Screenshot of MIME category configuration where you can set the treemap colors
for different file types (MIME types), based on wildcard pattern matching.
Both case-sensitive and case-insensitive matches are supported.  The configured
colours are previewed for each category, or can be hidden.

![MIME Categories Configuration Screenshot](QDirStat-config-mime-2.png)

Screenshot of MIME category configuration where you can set the treemap colors
for different file types (MIME types).  The treemap configuration can be
changed and a real treemap previews the result.

![Cleanup Action Configuration Screenshot](QDirStat-config-cleanups.png)

Screenshot of cleanup actions configuration.

![Exclude Rules Configuration Screenshot](QDirStat-config-exclude.png)

Screenshot of the exclude rules configuration where you can define rules for
which directories to exclude when reading directory trees.

------------------

![Tree Column Configuration Screenshot](QDirStat-column-config.png)

Context menu of the tree header where you can configure the columns.


-------------------

## Navigation

[Top: Qt6DirStat home page](https://github.com/Lithopsian/qt6dirstat)

[Statistics help page](https://github.com/Lithopsian/qdirstat/tree/Qt6threaded/doc/stats/Statistics.md)
