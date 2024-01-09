#pragma once 


#include "drlt.h"
#include <iostream>
#include <boost/log/trivial.hpp>
#include <string>
#include <unordered_map>
#include <queue>

#include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <unordered_map>
#include <string>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>


using std::string;
using std::unordered_map;
using namespace boost::filesystem;
using boost::filesystem::directory_entry;


class dir_layout_copier_c{

    public:
        dir_layout_copier_c():_args(nullptr){}

        virtual ~dir_layout_copier_c();

        bool init(int argc, char **argv);

        static int usage();
        int run();

    private:
        bool _do_compress();
        bool _import_dir_layout();
        int _capture_dir_layout();
        bool _save_file_info(directory_entry &dentry);

        // https://theboostcpplibraries.com/boost.iostreams-filters
        boost::iostreams::filtering_ostream _out;

        
        std::unordered_map<string, string> * _args;

        bool _is_scan_mode = false;
        bool _is_import_mode = false;
        bool _is_verbose = false;
        bool _is_compress = false;
};