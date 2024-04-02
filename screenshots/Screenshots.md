# QDirStat Screenshots

Qt-based directory statistics: KDirStat without any KDE -- from the original KDirStat author.

(c) 2015-2021 Stefan Hundhammer <Stefan.Hundhammer@gmx.de>

Target Platforms: Linux, BSD, Unix-like systems

License: GPL V2


## QDirStat Main Window

![Main Window Screenshot](QDirStat-main-win.png)


### Different Main Window Layouts

![Layout 1 (short)](QDirStat-details-file-L1.png)
Layout 1 (short): Only the bare minimum of tree columns plus the details panel for the selected item.

![Layout 2 (classic)](QDirStat-details-file-L2.png)
Layout 2 (classic): The classic QDirStat tree columns plus the details panel for the selected item.

![Layout 3 (full)](QDirStat-details-file-L3.png)
Layout 3 (full): All tree columns including file owner, group and permissions in both "rwxrwxrwx" and octal,
but no details panel to make room for all the columns.


### Details Panel

![Details Panel for a File](QDirStat-details-file-L2.png)

File selected. Notice the "Package" field: For system files, QDirStat queries
the package manager (dpkg / rpm / pacman) which package the file belongs to.

![Details Panel for a Directory](QDirStat-details-dir.png)

Directory selected

![Details Panel for a "Files" Pseudo Directory](QDirStat-details-dot-entry.png)

"Files" pseudo directory selected

![Details Panel for Multiple Selected Items](QDirStat-details-multi-sel.png)

Multi-selection


## Open Directory Dialog

![Custom Open Directory Dialog Screenshot](QDirStat-open-dir-dialog.png)

Custom "Open Directory" dialog with quick access to the home directory and to
mounted filesystems. The "Cross Filesystems" check box here is a temporary
override (just for this program run) of the configuration setting of the same
name.


## Output During Cleanup Actions

![Cleanup Action Output Screenshot](QDirStat-cleanup-output.png)

Screenshot of output during cleanup actions. Of course this window is purely optional.


## Error Reporting

![Reporting Directory Read Errors Screenshot](QDirStat-err-dirs-light.png)

Reporting errors during directory reading. Typically this is because of
missing permissions, so it may or may not be important to the user. Those
errors are reported in small panels at the bottom of the directory tree
widget. The user can leave them open or close them.

![Details about Directory Read Errors](QDirStat-unreadable-dirs-window.png)

Clicking on the "Details..." link opens a separate window that lists all the
directories that could not be read. This window is non-modal, i.e. the user can
interact with the main window while it is open. A click on any directory in
that list locates that directory in the main window, i.e. opens all parent
branches and scrolls to ensure that directory is visible.


## Mounted Filesystems Window

![Custom Open Directory Dialog Screenshot](QDirStat-filesystems-window.png)

This window (opened from the menu with View -> Show Mounted Filesystems) shows
the currently mounted filesystems. It does not show system mounts like /dev,
/proc, /sys, and also no bind mounts, no Btrfs subvolumes and no multiple
mounts of the same filesystem.

The "Reserved" column shows the amount of disk space that is reserved for the
root user; "Free" is what is available for non-privileged users.


## File Type Statistics

![File Type Statistics Window Screenshot](QDirStat-file-type-stats.png)

Menu View -> File Type Statistics...


## Locating Files by Type

![Locate Files Window](QDirStat-locate-file-types-window.png)

Results after clicking on the "Locate" button in the "File Type Statistics" window.

![Locating Files](QDirStat-locating-file-types.png)

Locating files with a specific extension. That branch is automatically opened
in the tree view in the main window, and all matching files in that directory are selected.


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


## Discovering Files

![Discovering Files](QDirStat-discover.png)

Discovering files of certain categories. Similar to "locating by type" above,
clicking a file in the list selects it in the tree view in the main window.


## Packages View

![Packages View Screenshot](QDirStat-pkg-details.png)

Packages view: All packages of a Xubuntu 18.04 LTS installation. Notice that
each directory contains only the files that belong to that package: /usr/bin
only contains the chromium-browser binary in this view.


![Packages Summary Screenshot](QDirStat-pkg-summary.png)

Packages Summary


![Packages View Screenshot](QDirStat-pkg-emacs.png)

Packages view limited to package names starting with "emacs".


!["Open Packages" Dialog Screenshot](QDirStat-open-pkg-dialog.png)

Dialog for selecting which packages to display. Use menu "File" -> "Show
Installed Packages".

To avoid the initial "Choose Directory" dialog, start QDirStat with the "-d"
or "--dont-ask" command line parameter (or simply click "Cancel" in the "Choose
Directory" dialog).


## Unpackaged Files View

![Unpackaged Files Screenshot](QDirStat-unpkg-usr-share-qt5.png)

Unpackaged files view: files in selected directories that are not part of
any package.  Directories that are not expected to contain packaged files
can be excluded from the tree, as well as specific types of files that are
commonly not in a package but are generated when a package is installed
(eg. \*.pyc Python files).

Notice the greyed-out ignored directories which contain the files that are
properly packaged.


![Unpackaged Files Screenshot](QDirStat-unpkg-boot-grub2.png)

Unpackaged files view: I created my own font for Grub2 which clearly stands out as an unpackaged file.


!["Show Unpackaged Files" Dialog Screenshot](QDirStat-show-unpkg-dialog.png)

Dialog to start the "unpackaged files" view (Menu "File" -> "Show Unpackaged
Files"). Some directories are excluded by default because they are expected to
contain a lot of unpackaged files. You can add more directories or remove
predefined ones.

Since Python tends to leave behind so many unpackaged files, all `*.pyc` files
are ignored by default. You can add more wildcard patterns to ignore or remove
the predefined one.



## Configuration


![Cleanup Action Configuration Screenshot](QDirStat-config-cleanups.png)

Screenshot of cleanup actions configuration.

![MIME Categories Configuration Screenshot](QDirStat-config-mime.png)

Screenshot of MIME category configuration where you can set the treemap colors
for different file types (MIME types), complete with a real treemap widget as preview.

![Exclude Rules Configuration Screenshot](QDirStat-config-exclude.png)

Screenshot of the exclude rules configuration where you can define rules which
directories to exclude when reading directory trees.

![General Options Configuration Screenshot](QDirStat-config-general.png)

Screenshot of the general (miscellaneous) configuration options.

------------------

![Tree Column Configuration Screenshot](QDirStat-column-config.png)

Context menu of the tree header where you can configure the columns.


-----------------

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


--------------------

## Navigation

[Top: QDirStat home page](https://github.com/shundhammer/qdirstat)

[Statistics help page](https://github.com/Lithopsian/qdirstat/tree/Qt6threaded/doc/stats/Statistics.md)
