#include "drlt.h"
#include <iostream>
#include <boost/log/trivial.hpp>
#include <string>
#include <unordered_map>
#include <queue>

#include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>

//#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>


#define BOOST_LOG_DYN_LINK 1

using boost::filesystem::directory_entry;
using std::cout;
using std::endl;
using std::pair;
using std::queue;
using std::string;
using std::unordered_map;
using namespace boost::filesystem;

namespace def
{
    const string cmd_scan = string("-s");
    const string cmd_scan_long = string("--scan");

    const string cmd_import = string("-i");
    const string cmd_import_long = string("--import");

    const string cmd_compress = string("-c");
    const string cmd_compress_long = string("--compress");

    const string cmd_verbose = string("-v");
    const string cmd_verbose_long = string("--verbose");


    const string cmd_help = string("-h");
    const string cmd_help_long = string("-h");

    const string cmd_output_file = string("-o");
    const string cmd_output_file_long = string("--output");


    const string default_output_fname = string("dir_contents.txt");

}

int dir_layout_copier_c::usage()
{

    cout << endl;
    cout << "Usage: drlt [-h|--help | [ [-s <path>] | -i <path>] [-c] [-o <dir_contents_file>]]\n"
         << endl;
    cout << "\t -h, --help - print help" << endl;
    cout << "\t -s, --scan - scans directory and creates dir contents file " << endl;
    cout << "\t -c, --compress - compress dir contents file" << endl;
    cout << "\t -o, --output - dir contents output file name. Default: " << def::default_output_fname << endl;

    cout << "\t -i, --import - imports dir contents file and recreates dir structure" << endl;

    return 0;
}

bool dir_layout_copier_c::init(int argc, char **argv)
{

    if (_args)
        delete(_args);

    _args = new std::unordered_map<string, string>();


    if (argc == 1)
        return false;

    for (int i = 1; i < argc; i++)
    {
        if (!strncmp(argv[i], def::cmd_scan.c_str(), def::cmd_scan.size()) || !strncmp(argv[i], def::cmd_scan_long.c_str(), def::cmd_scan_long.size()))
        {
            if (i + 1 >= argc)
            {
                BOOST_LOG_TRIVIAL(error) << " - Specify directory to scan ...";
                return false;
            }

            _args->insert(pair(def::cmd_scan_long, argv[i + 1]));
            _is_scan_mode = true;
        }

        if (!strncmp(argv[i], def::cmd_output_file.c_str(), def::cmd_output_file.size()) || !strncmp(argv[i], def::cmd_output_file_long.c_str(), def::cmd_output_file_long.size()))
        {
            if (i + 1 >= argc)
            {
                BOOST_LOG_TRIVIAL(error) << " - Specify output file name  ...";
                return false;
            }

            _args->insert(pair(def::cmd_output_file_long, argv[i + 1]));
        }



        if (!strncmp(argv[i], def::cmd_import.c_str(), def::cmd_import.size()) || !strncmp(argv[i], def::cmd_import_long.c_str(), def::cmd_import_long.size()))
        {
            if (i + 1 >= argc)
            {
                BOOST_LOG_TRIVIAL(error) << " - import file not specified " << endl;
                return false;
            }

            _args->insert(pair(def::cmd_import_long, argv[i + 1]));
            _is_import_mode = true;
        }

        if (!strncmp(argv[i], def::cmd_help.c_str(), def::cmd_help.size()) || !strncmp(argv[i], def::cmd_help_long.c_str(), def::cmd_help_long.size()))
            return false;

        if (!strncmp(argv[i], def::cmd_compress.c_str(), def::cmd_compress.size()) || !strncmp(argv[i], def::cmd_compress_long.c_str(), def::cmd_compress_long.size())){
            _args->insert(pair(def::cmd_compress_long, "true"));
            _is_compress  = true;
        }

        if (!strncmp(argv[i], def::cmd_verbose.c_str(), def::cmd_verbose.size()) || !strncmp(argv[i], def::cmd_verbose_long.c_str(), def::cmd_verbose_long.size()))
        {
            _args->insert(pair(def::cmd_verbose_long, "true"));
            _is_verbose = true;
        }
    }

    if ((!_is_scan_mode && !_is_import_mode) || (_is_scan_mode && _is_import_mode) )
    {
        BOOST_LOG_TRIVIAL(error) << " - Specify either " << def::cmd_import_long << " or " << def::cmd_scan_long << " commands but not both ..";
        return false;
    }

    if (_is_scan_mode)
    {
        if (_is_compress)
        {
            // out.push(boost::iostreams::zlib_compressor());
            _out.push(boost::iostreams::gzip_compressor());
        }
        string out_file = def::default_output_fname;
        if (_args->find(def::cmd_output_file_long) != _args->end())
            out_file = _args->at(def::cmd_output_file_long);
        
        if (_is_compress)
            out_file +=".gz";

        BOOST_LOG_TRIVIAL(info) << "output file:" << out_file << endl;

        _out.push(boost::iostreams::file_sink(out_file));
    }

    return true;
}

bool dir_layout_copier_c::_do_compress()
{
    if (_args->find(def::cmd_compress_long) != _args->end())
        return true;

    return false;
}

bool dir_layout_copier_c::_save_file_info(directory_entry &dentry)
{

    auto fstat = dentry.status();

    string out_str{""};

    out_str += dentry.path().c_str();
    out_str += " ";
    out_str += std::to_string(dentry.status().type());
    out_str += " ";

    out_str += std::to_string(dentry.status().permissions());
    out_str += " ";

    if (is_regular_file(dentry))
        out_str += std::to_string(file_size(dentry.path()));
    else
        out_str += std::to_string(-1);

    out_str += " ";

    if (is_symlink(dentry))
    {
        path target = read_symlink(dentry.path());
        out_str += target.c_str();
        out_str += " ";
    }

    if (_is_verbose)
        cout << out_str << endl;

    _out << out_str << endl;
    _out.flush();

    return true;
}

bool dir_layout_copier_c::_import_dir_layout()
{

    return 0;
}

using namespace std;

int dir_layout_copier_c::_capture_dir_layout()
{



    string root_dir = _args->at(def::cmd_scan_long);

    BOOST_LOG_TRIVIAL(info) << "Capturing dir layout for " << root_dir;

    path p(root_dir);
    queue<path> q;

    q.push(p);

    while (q.size())
    {

        try
        {

            path p = q.front();
            q.pop();

            if (exists(p))
            {
                auto dentry = directory_entry(p);
                _save_file_info(dentry);

                if (is_directory(p))
                {
                    for (directory_entry &x : directory_iterator(p))
                    {
                        if (is_directory(x))
                            q.push(x);
                        else
                            _save_file_info(x);
                    }
                }
            }
            else
                BOOST_LOG_TRIVIAL(warning) << p << " does not exist\n";

        } // try

        catch (const filesystem_error &ex)
        {
            BOOST_LOG_TRIVIAL(warning) << ex.what();
        }

    } // while


    return 0;
}


dir_layout_copier_c::~dir_layout_copier_c(){
    
    if (_do_compress())
        _out.pop();
}


int dir_layout_copier_c::run(){

    if (_args->find(def::cmd_scan_long) != _args->end())
        return _capture_dir_layout();

    if (_args->find(def::cmd_import_long) != _args->end())
        return _import_dir_layout();

    return 0;
}

int main(int argc, char **argv)
{

    dir_layout_copier_c dlc;

    if (!dlc.init(argc, argv))
        return dlc.usage();

    return dlc.run();
}