#include "custom_tree_widget_items.h"

#include <cassert>

#include <QHeaderView>


TreeWidgetFolder::TreeWidgetFolder(QTreeWidgetItem *parent, Folder* ptr_to_folder, Archive* ptr_to_archive, bool filesize_scaled  )
: QTreeWidgetItem(parent, QStringList() << ptr_to_folder->name.c_str(), QTreeWidgetItem::UserType+1)
{
    this->folder_ptr = ptr_to_folder;
    this->archive_ptr = ptr_to_archive;
    this->setIcon(0, *(new QIcon(":/img/folder.png")));

    if (ptr_to_folder->sibling_ptr) parent->addChild( new TreeWidgetFolder( parent, ptr_to_folder->sibling_ptr.get(), ptr_to_archive, filesize_scaled ) );
    if (ptr_to_folder->child_dir_ptr) this->addChild( new TreeWidgetFolder( this, ptr_to_folder->child_dir_ptr.get(), ptr_to_archive, filesize_scaled ) );
    if (ptr_to_folder->child_file_ptr) this->addChild( new TreeWidgetFile( this, ptr_to_folder->child_file_ptr.get(), ptr_to_archive, filesize_scaled ) );

}


TreeWidgetFolder::TreeWidgetFolder(TreeWidgetFolder *parent, Folder* ptr_to_folder, Archive* ptr_to_archive, bool filesize_scaled  )
: QTreeWidgetItem(parent, QStringList() << ptr_to_folder->name.c_str(), QTreeWidgetItem::UserType+1)
{
    this->folder_ptr = ptr_to_folder;
    this->archive_ptr = ptr_to_archive;
    this->setIcon(0, *(new QIcon(":/img/folder.png")));


    if (ptr_to_folder->sibling_ptr) parent->addChild( new TreeWidgetFolder( parent, ptr_to_folder->sibling_ptr.get(), ptr_to_archive, filesize_scaled ) );

    if (ptr_to_folder->child_dir_ptr) this->addChild( new TreeWidgetFolder( this, ptr_to_folder->child_dir_ptr.get(), ptr_to_archive, filesize_scaled ) );
    if (ptr_to_folder->child_file_ptr) this->addChild( new TreeWidgetFile( this, ptr_to_folder->child_file_ptr.get(), ptr_to_archive, filesize_scaled ) );
}


void TreeWidgetFolder::unpack( std::string path_for_extraction, bool& aborting_var ) {
    std::filesystem::path extraction_path( path_for_extraction );
    if ( !extraction_path.empty() ) {   // check if something is at least written
        std::fstream source( archive_ptr->load_path, std::ios::binary | std::ios::in);
        assert( source.is_open() );

        folder_ptr->unpack( path_for_extraction, source, aborting_var, true );
        source.close();
    }
}


void TreeWidgetFile::unpack( std::string path_for_extraction, bool& aborting_var ) {
    std::filesystem::path extraction_path( path_for_extraction );
    if ( !extraction_path.empty() ) {   // check if something is at least written
        std::fstream source( archive_ptr->load_path, std::ios::binary | std::ios::in);
        assert( source.is_open() );

        file_ptr->unpack( path_for_extraction, source, aborting_var, false );
        source.close();
    }
}


TreeWidgetFile::TreeWidgetFile(TreeWidgetFolder *parent, File* ptr_to_file, Archive* ptr_to_archive, bool filesize_scaled)
: QTreeWidgetItem(parent, QStringList() << ptr_to_file->name.c_str() << QString::fromStdString( ptr_to_file->get_uncompressed_filesize_str(filesize_scaled)) << QString::fromStdString(ptr_to_file->get_compressed_filesize_str(filesize_scaled)) << QString::number((float)ptr_to_file->original_size/(float)(ptr_to_file->compressed_size)), QTreeWidgetItem::UserType+2 )
{
    this->setTextAlignment(1, Qt::AlignRight);
    this->setTextAlignment(2, Qt::AlignRight);
    this->setTextAlignment(3, Qt::AlignRight);
    this->file_ptr = ptr_to_file;
    this->archive_ptr = ptr_to_archive;
    this->setIcon(0, *(new QIcon(":/img/file.png")));
    if (ptr_to_file->sibling_ptr) parent->addChild( new TreeWidgetFile( parent, ptr_to_file->sibling_ptr.get(), ptr_to_archive, filesize_scaled ) );
}




bool TreeWidgetFile::operator<(const QTreeWidgetItem &other)const {
    //1001 - TreeWidgetFolder
    //1002 - TreeWidgetFile

    Qt::SortOrder order = this->treeWidget()->header()->sortIndicatorOrder();


    if (order == Qt::AscendingOrder and other.type() == 1001) return false; // folders always go before files
    if (order == Qt::DescendingOrder and other.type() == 1001) return true; // folders always go before files


    int column = treeWidget()->sortColumn();
    switch( column )
    {
    case 0: // sort by name
            if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
            else return false;
            break;
    case 1: // sort by size
        if (this->type() == other.type() ) {
            if ( this->type() == 1001 ) { // both this and other are TreeWidgetFolders
                if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
                else return false;
            }
            else {  // both this and other are TreeWidgetFiles
                TreeWidgetFile* twfile = (TreeWidgetFile*)(&other);
                if ( this->file_ptr->original_size < twfile->file_ptr->original_size ) return true;
                else return false;
            }
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    case 2: // sort by compressed size
        if (this->type() == other.type() ) {
           // both this and other are TreeWidgetFiles
                TreeWidgetFile* twfile = (TreeWidgetFile*)(&other);
                if ( this->file_ptr->compressed_size < twfile->file_ptr->compressed_size ) return true;
                else return false;
        }
        else if (other.type() == 1001) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    case 3: // sort by compression ratio
        // Testing
        if (this->type() == other.type() ) {
            // both this and other are TreeWidgetFiles
            TreeWidgetFile* twfile = (TreeWidgetFile*)(&other);
            if ( (double)this->file_ptr->original_size/(double)this->file_ptr->compressed_size < (double)twfile->file_ptr->original_size / (double)twfile->file_ptr->compressed_size ) return true;
            else return false;
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    default:
        throw NotImplementedException("Something went terribly wrong while sorting.");
    }
}


bool TreeWidgetFolder::operator<(const QTreeWidgetItem &other)const {
    //1001 - TreeWidgetFolder
    //1002 - TreeWidgetFile

    Qt::SortOrder order = this->treeWidget()->header()->sortIndicatorOrder();

    if (order == Qt::AscendingOrder and other.type() == 1002) return true; // folders always go before files
    if (order == Qt::DescendingOrder and other.type() == 1002) return false; // folders always go before files


    int column = treeWidget()->sortColumn();
    switch( column )
    {
    case 0: // sort by name
            if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
            else return false;
            break;
    case 1: // sort by size
        if (this->type() == other.type() ) {// both this and other are TreeWidgetFolders

            if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
            else return false;
            break;
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    case 2: // sort by compressed size
        if (this->type() == other.type() ) {// both this and other are TreeWidgetFolders

            if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
            else return false;
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    case 3: // sort by compression ratio

        if (this->type() == other.type() ) {// both this and other are TreeWidgetFolders

            if ( this->text(0) < other.text(0) ) return true;    // just sort them alphabetically by name
            else return false;
            break;
        }
        else if (this->type() < other.type()) return true;  // TreeWidgetFolders go before TreeWidgetFiles
        else return false;
        break;

    default:
        throw NotImplementedException("Something went terribly wrong while sorting.");
    }
}




