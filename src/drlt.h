#pragma once 


#include <iostream>
#include <boost/log/trivial.hpp>
#include <string>
#include <unordered_map>
#include <deque>
#include <vector>

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
#include <boost/program_options.hpp>

using std::string;
using std::pair;
using std::unordered_map;
using namespace boost::filesystem;
using boost::filesystem::directory_entry;
namespace po = boost::program_options;

class dir_layout_copier_c{

    public:
        dir_layout_copier_c(){ _pdesc = new po::options_description("Allowed options"); }

        virtual ~dir_layout_copier_c();

        bool init(int argc, char **argv);

        int run();

    private:
        void _usage();
        bool _restore_dir_layout();
        int _capture_dir_layout();
        bool _save_file_info(directory_entry& dentry, pair<string,string> *anon);
        bool _restore_entry(string line, std::vector<string> &retry_list);

        // https://theboostcpplibraries.com/boost.iostreams-filters
        boost::iostreams::filtering_ostream _out;

        
        bool _is_scan_mode = false;
        bool _is_restore_mode = false;
        bool _is_verbose = false;
        bool _is_compress = false;
        bool _is_progress = false;
        bool _is_anonymize = false;
        size_t _dnum{0};
        size_t _fnum{0};
        string _target_dir;
        string _file;

        po::options_description  *_pdesc;
};