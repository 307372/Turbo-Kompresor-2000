#ifndef CUSTOM_TREE_WIDGET_ITEMS_H
#define CUSTOM_TREE_WIDGET_ITEMS_H

#include <QTreeWidgetItem>

#include "archive.h"
#include "archive_structures.h"


class TreeWidgetFolder : public QTreeWidgetItem {
// type = 1001
public:
    Folder* folder_ptr;
    Archive* archive_ptr;

    TreeWidgetFolder(QTreeWidgetItem *parent, Folder* ptr_to_folder, Archive* ptr_to_archive, bool filesize_scaled );
    TreeWidgetFolder(TreeWidgetFolder *parent, Folder* ptr_to_folder, Archive* ptr_to_archive, bool filesize_scaled );

    void unpack( std::string path_for_extraction, bool& aborting_var );
    void set_disabled(bool disabled);
    bool operator<(const QTreeWidgetItem &other)const;
};


class TreeWidgetFile : public QTreeWidgetItem {
// type = 1002
public:
    File* file_ptr;
    Archive* archive_ptr;

    TreeWidgetFile(TreeWidgetFolder *parent, File* ptr_to_file, Archive* ptr_to_archive, bool filesize_scaled );

    void unpack( std::string path_for_extraction, bool& aborting_var );
    void set_disabled(bool disabled);
    bool operator<(const QTreeWidgetItem &other)const;
};

#endif
