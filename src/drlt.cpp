#include "drlt.h"
#include <iostream>
#include <boost/log/trivial.hpp>
#include <string>
#include <unordered_map>
#include <deque>

#include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>

//#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <netinet/in.h>

#include <sstream>
#include <filesystem>

// #ifdef __linux__ 
// #include <fcntl.h> //for fallocate
// #elif _WIN32
// // windows code goes here
// #else

// #endif

#define BOOST_LOG_DYN_LINK 1

using boost::filesystem::directory_entry;
using std::cout;
using std::endl;
using std::pair;
using std::deque;
using std::string;
using std::unordered_map;
using namespace boost::filesystem;

namespace def
{
    const string cmd_scan = string("-g");
    const string cmd_scan_long = string("--get");

    const string cmd_restore = string("-r");
    const string cmd_restore_long = string("--restore");

    const string cmd_compress = string("-c");
    const string cmd_compress_long = string("--compress");

    const string cmd_progress = string("-p");
    const string cmd_progress_long = string("--progress");


    const string cmd_verbose = string("-v");
    const string cmd_verbose_long = string("--verbose");


    const string cmd_help = string("-h");
    const string cmd_help_long = string("-h");

    const string cmd_output_file = string("-o");
    const string cmd_output_file_long = string("--output");

    const string cmd_restore_dir = string("-d");
    const string cmd_restore_dir_long = string("--restore_dir");
    const string default_restore_dir = string("./");


    const string default_output_fname = string("dir_contents.txt");

    const char FD = '\t';
}

int dir_layout_copier_c::usage()
{

    cout << endl;
    cout << "Usage: dir_layout [-h|--help | [ [-g <path>] | -r <path> [-d <dir>]] [-c] [-o <dir_contents_file>]]\n"
         << endl;
    cout << "\t -h, --help - print help" << endl;
    cout << "\t -g, --get -  scans directory and creates dir contents file " << endl;
    cout << "\t -c, --compress - compress dir contents file" << endl;
    cout << "\t -o, --output - dir contents output file name. Default: " << def::default_output_fname << endl;

    cout << "\t -r, --restore - restores dir structure and contents based on the input file" << endl;
    cout << "\t -d, --restore_dir - specifies where layout will be restored. Can be used only with -r flag" << endl;

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

        if (!strncmp(argv[i], def::cmd_restore_dir.c_str(), def::cmd_restore_dir.size()) || !strncmp(argv[i], def::cmd_restore_dir_long.c_str(), def::cmd_restore_dir_long.size()))
        {
            if (i + 1 >= argc)
            {
                BOOST_LOG_TRIVIAL(error) << " - Specify dir where layout will be restored  ...";
                return false;
            }

            _args->insert(pair(def::cmd_restore_dir_long, argv[i + 1]));
        }



        if (!strncmp(argv[i], def::cmd_restore.c_str(), def::cmd_restore.size()) || !strncmp(argv[i], def::cmd_restore_long.c_str(), def::cmd_restore_long.size()))
        {
            if (i + 1 >= argc)
            {
                BOOST_LOG_TRIVIAL(error) << " - import file not specified " << endl;
                return false;
            }

            _args->insert(pair(def::cmd_restore_long, argv[i + 1]));
            _is_restore_mode = true;
        }

        if (!strncmp(argv[i], def::cmd_help.c_str(), def::cmd_help.size()) || !strncmp(argv[i], def::cmd_help_long.c_str(), def::cmd_help_long.size()))
            return false;

        if (!strncmp(argv[i], def::cmd_compress.c_str(), def::cmd_compress.size()) || !strncmp(argv[i], def::cmd_compress_long.c_str(), def::cmd_compress_long.size())){
            _args->insert(pair(def::cmd_compress_long, "true"));
            _is_compress  = true;
        }

        if (!strncmp(argv[i], def::cmd_progress.c_str(), def::cmd_progress.size()) || !strncmp(argv[i], def::cmd_progress_long.c_str(), def::cmd_progress_long.size()))
        {
            _args->insert(pair(def::cmd_progress_long, "true"));
            _is_progress = true;
        }

        if (!strncmp(argv[i], def::cmd_verbose.c_str(), def::cmd_verbose.size()) || !strncmp(argv[i], def::cmd_verbose_long.c_str(), def::cmd_verbose_long.size()))
        {
            _args->insert(pair(def::cmd_verbose_long, "true"));
            _is_verbose = true;
        }
    }

    if ((!_is_scan_mode && !_is_restore_mode) || (_is_scan_mode && _is_restore_mode) )
    {
        BOOST_LOG_TRIVIAL(error) << " - Specify either " << def::cmd_restore_long << " or " << def::cmd_scan_long << " commands but not both ..";
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

        BOOST_LOG_TRIVIAL(info) << "output file:" << out_file;

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

    try
    {


        out_str += dentry.path().c_str();
        out_str += def::FD;

        if (is_symlink(dentry))
            out_str += std::to_string(boost::filesystem::file_type::symlink_file);
        else
            out_str += std::to_string(dentry.status().type());
        
        out_str += def::FD;

        out_str += std::to_string(dentry.status().permissions());
        out_str += def::FD;

        if (is_regular_file(dentry))
            out_str += std::to_string(file_size(dentry.path()));
        else
            out_str += std::to_string(-1);

        out_str += def::FD;

        if (is_symlink(dentry))
        {
            path target = read_symlink(dentry.path());
            out_str += target.c_str();
            out_str += def::FD;;
        }

        if (is_directory(dentry))
            _dnum++;
        else
            _fnum++;
    }

    catch (const filesystem_error &ex)
    {
        BOOST_LOG_TRIVIAL(trace) <<"\r"<< ex.what();
        return false;
    }

    if (_is_verbose)
        cout << out_str << endl;

    _out << out_str << endl;
    _out.flush();

    return true;
}

bool dir_layout_copier_c::_restore_entry(string line, std::vector<string> &retry_list){

    auto iss = std::istringstream{line};
    std::string tmp;

    auto path = std::string{};
    std::getline(iss, tmp, def::FD);
    std::istringstream(tmp) >> path;

    int type{0};
    std::getline(iss, tmp, def::FD);
    std::istringstream(tmp) >> type;

    int permissions{0};
    std::getline(iss, tmp, def::FD);
    std::istringstream(tmp) >> permissions;

    size_t size{0};
    std::getline(iss, tmp, def::FD);
    std::istringstream(tmp) >> size;


    boost::filesystem::file_status st((boost::filesystem::file_type)type,(boost::filesystem::perms)permissions);
    directory_entry d(path,st,st);


    try
    {
        if (is_regular_file(d) && !is_symlink(d)){
            std::ofstream ofs(d.path());
            ofs.seekp(size);
            ofs.close();            
            resize_file(d.path(),size);                        
        }
        else if (is_directory(d)){
            create_directories(d.path());
        }
        else if (is_symlink(d)){
            auto target_path = std::string{};
            std::getline(iss, tmp, def::FD);
            std::istringstream(tmp) >> target_path;

            boost::system::error_code ec;
            create_symlink(target_path, path,ec);

            //TBD
        }
        else{
            switch ((boost::filesystem::file_type)type)
            {
            case boost::filesystem::file_type::block_file :
                BOOST_LOG_TRIVIAL(warning) << path << " is a block_file, skipping";
                break;
            
            case boost::filesystem::file_type::character_file :
                BOOST_LOG_TRIVIAL(warning) << path << " is a character_file , skipping";
                break;

            case boost::filesystem::file_type::fifo_file :
                BOOST_LOG_TRIVIAL(warning) << path << " is a fifo_file , skipping";
                break;

            case boost::filesystem::file_type::socket_file :
                BOOST_LOG_TRIVIAL(warning) << path << " is a socket_file , skipping";
                break;

            case boost::filesystem::file_type::file_not_found :
                BOOST_LOG_TRIVIAL(error) << path << " is a file_not_found !??, skipping";
                break;
            case boost::filesystem::file_type::status_error :
                BOOST_LOG_TRIVIAL(error) << path << " is a status_error !??, skipping";
                break;
            case boost::filesystem::file_type::type_unknown :
                BOOST_LOG_TRIVIAL(error) << path << " is a type_unknown !??, skipping";
                break;


            default:
                //TBD put ASSERT here  
                BOOST_LOG_TRIVIAL(error) << path << " of an unknown type:" << type << " DEBUG ME !!!, skipping";
                break;
            }

        }
            
        boost::filesystem::permissions(path, d.status().permissions());

        if (_is_verbose)
            cout << path << " type:"<<type<< " perm:"<<permissions  <<endl;
    }
    catch (const filesystem_error &ex)
    {
        BOOST_LOG_TRIVIAL(trace) <<"\r"<< ex.what();
        return false;
    }

    return true;
}

bool dir_layout_copier_c::_restore_dir_layout()
{

    string input_file = _args->at(def::cmd_restore_long);

    BOOST_LOG_TRIVIAL(info) << "Trying to restore layout from file " << input_file;

    path p(input_file);
    if (!exists(p)){
        BOOST_LOG_TRIVIAL(error) << "Input file does not exist! ";
        return false;
    }

    string restore_dir = def::default_restore_dir;
    if (_args->find(def::cmd_restore_dir_long) != _args->end())
        restore_dir = _args->at(def::cmd_restore_dir_long);

    BOOST_LOG_TRIVIAL(info) << "Restore directory path: " << restore_dir;
    path prd(restore_dir);
    if (!exists(prd)){
        BOOST_LOG_TRIVIAL(error) << "Specified restore directory does not exist! ";
        return false;
    }
    

    std::ifstream ifs (input_file, std::ios_base::in | std::ios_base::binary);
    short header{0}; // = ifs.peek();
    ifs.read((char*)&header,2);

    if (0x1f8b == htons(header)){
        _is_compress = true;
        BOOST_LOG_TRIVIAL(info) << "Detected compressed (gzip) input file ";
    }

    ifs.seekg(std::ios_base::beg);

    boost::iostreams::filtering_istream in;
    if (_is_compress)
        in.push(boost::iostreams::gzip_decompressor());
    
    in.push(ifs);

    std::string line;
    size_t bytes_read = 0;

    std::vector<string> retry_list;
    while (in)
    {
        std::getline(in, line);
        bytes_read += line.size();
        // cout << line << endl;
        _restore_entry(restore_dir + string("/") + line,retry_list); //prefix path iwith restore dir path

        //TBD : show restore progress % 
    }

    ifs.close();

    return 0;
}

using namespace std;



void show_tick(size_t &tick_num){
    string t{"-\\|+/-\\|/"};
    cout << t[tick_num%8] << "\r";
    tick_num++;
}

int dir_layout_copier_c::_capture_dir_layout()
{
    string root_dir = _args->at(def::cmd_scan_long);

    BOOST_LOG_TRIVIAL(info) << "Capturing dir layout for " << root_dir;


    path p(root_dir);
    deque<path> dq;

    dq.push_front(p);

    while (dq.size())
    {

        try
        {

            path p = dq.front();
            dq.pop_front();

            if (exists(p))
            {
                auto dentry = directory_entry(p);
                _save_file_info(dentry);

                if (is_directory(p))
                {

                    for (directory_entry &x : directory_iterator(p))
                    {
                        try
                        {
                            if (is_directory(x))
                                dq.push_front(x);
                            else{

                                //add link target into dir layout first 
                                //so that during restore link could be created to restored target 
                                if (is_symlink(x.symlink_status())){
                                    directory_entry link_target = directory_entry(read_symlink(x.path()));
                                    _save_file_info(link_target);
                                }

                                _save_file_info(x);
                                
                            }
                                
                        }
                        catch (const filesystem_error &ex) {BOOST_LOG_TRIVIAL(trace) << ex.what();}
                    }
                }

            }
            else
                BOOST_LOG_TRIVIAL(warning) << p << "does not exist\n";

        } // try

        catch (const filesystem_error &ex) { BOOST_LOG_TRIVIAL(trace) << ex.what(); }            
        

    } // while

    if (_is_progress)
    {
        cout << "\r"
             << "Found dirs:" << _dnum << " files:" << _fnum <<  "  Total:"<<_dnum + _fnum <<endl;;
    }

    return 0;
}


dir_layout_copier_c::~dir_layout_copier_c(){
    
    if (_is_compress && _is_scan_mode)
        _out.pop();

    if (_args)
        delete(_args);
}


int dir_layout_copier_c::run(){

    if (_args->find(def::cmd_scan_long) != _args->end())
        return _capture_dir_layout();

    if (_args->find(def::cmd_restore_long) != _args->end())
        return _restore_dir_layout();

    return 0;
}

int main(int argc, char **argv)
{

    dir_layout_copier_c dlc;

    if (!dlc.init(argc, argv))
        return dlc.usage();

    return dlc.run();
}