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

#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <iterator>
#include <boost/program_options.hpp>
#include <exception>

#include <sstream>
#include <filesystem>

#ifdef __linux__ 
#include <netinet/in.h>
#elif _WIN32
#include <winsock.h>
#else
#endif

#define BOOST_LOG_DYN_LINK 1

using boost::filesystem::directory_entry;
using std::cout;
using std::endl;
using std::pair;
using std::deque;
using std::string;
using std::unordered_map;
using std::cerr;
using std::exception;

namespace po = boost::program_options;
using namespace boost::filesystem;


namespace def
{

    const string cmd_help = string("help");
    const string cmd_scan = string("scan");
    const string cmd_compress = string("compress");
    const string cmd_file = string("file");
    const string cmd_restore = string("restore");
    const string cmd_verbose = string("verbose");
    const string cmd_progress = string("progress");
    const string cmd_anonymize = string("anonymize");

    const char FD = '\t';
}

void dir_layout_copier_c::_usage()
{
    cout << "Usage: dir_layout [--help | [ [--scan <path>] | [--restore <path> ] [--file <path>] [--compress] [--verbose] [--progress] [--anonymize]]" << endl;

    cout << *_pdesc << "\n";
    return;
}

bool dir_layout_copier_c::init(int argc, char** argv)
{
	try {
        _pdesc->add_options()
            ("help", "produce help message")
            ("scan", po::value<string>(), "scans directory <arg> for it's layout/contents")
            ("compress", "gzip/compress scan output file")
            ("verbose", "noisy output")
            ("progress", "report some stats")
            ("anonymize", "anonymize dir/file names")
            ("file", po::value<string>(), "input/output file for dir layout")
            ("restore", po::value<string>(), "restore layout under specified directory");


		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, *_pdesc), vm);
		po::notify(vm);

        if (argc ==1 || vm.count("help")) {
            _usage();
            return false;
        }


        if (vm.count(def::cmd_verbose)) 
            _is_verbose = true;

        if (vm.count(def::cmd_compress))
            _is_compress = true;

        if (vm.count(def::cmd_anonymize))
            _is_anonymize = true;



        if (vm.count(def::cmd_progress))
            _is_progress = true;

        if (_is_verbose)
			BOOST_LOG_TRIVIAL(info) << "Options verbose:" << _is_verbose << " compress:" << _is_compress << " progress:" << _is_progress;

        if (vm.count(def::cmd_scan)) {
            _is_scan_mode = true;
            _target_dir = vm[def::cmd_scan].as<string>();

            if (_is_verbose)
                BOOST_LOG_TRIVIAL(info) << "Scan mode. target_dir: " << _target_dir;
        }

        if (vm.count(def::cmd_restore)) {
            _is_restore_mode = true;
            _target_dir = vm[def::cmd_restore].as<string>();
            if (_is_verbose)
                BOOST_LOG_TRIVIAL(info) << "Restore mode. target_dir: " << _target_dir;

        }

        if (!(_is_restore_mode ^ _is_scan_mode)) {
            BOOST_LOG_TRIVIAL(error) << "Specify at least one of the following commands: " << def::cmd_scan <<","<<def::cmd_restore;
            return false;
        }

        if (vm.count(def::cmd_file)) {
            _file = vm[def::cmd_file].as<string>();
            if (_is_verbose)
                BOOST_LOG_TRIVIAL(info) << "Input/output file:" << _file;
        }
        else {
            BOOST_LOG_TRIVIAL(error) << "Specify input/output file using --" << def::cmd_file << " option";
            return false;
        }


        if (_is_scan_mode)
        {
            if (_is_compress)
                _out.push(boost::iostreams::gzip_compressor());

            _out.push(boost::iostreams::file_sink(_file, std::ofstream::binary));
        }

	}
	catch (exception& e) {
		cerr << "error: " << e.what() << "\n";
		return false;
	}
	catch (...) {
		cerr << "Exception of unknown type!\n";
		return false;
	}
    

    return true;
}



bool dir_layout_copier_c::_save_file_info(directory_entry& dentry, pair<string,string> * anon=nullptr)
{

    auto fstat = dentry.status();

    string out_str{ "" };

    try
    {

        //anon.first - parent dir anonimized name; 
        //return new anonymized file/dir name component in anon.second
        if (_is_anonymize && anon){
            anon->second = anon->first;
            anon->second += dentry.path().separator;
            anon->second += (is_directory(dentry) ? std::to_string(_dnum) : std::to_string(_fnum));
            out_str += anon->second;             
        }
        else
            out_str += dentry.path().string();
        
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
            out_str += target.string();
            out_str += def::FD;;
        }

        if (is_directory(dentry))
            _dnum++;
        else
            _fnum++;
    }

    catch (const filesystem_error& ex)
    {
        BOOST_LOG_TRIVIAL(trace) << ex.what();
        return false;
    }

    if (_is_verbose)
        cout << out_str << endl;

    _out << out_str << endl;
    //_out.flush();

    return true;
}

bool dir_layout_copier_c::_restore_entry(string line, std::vector<string>& retry_list) {

    auto iss = std::istringstream{ line };
    std::string tmp;

    auto path = std::string{};
    std::getline(iss, tmp, def::FD);
    std::istringstream(tmp) >> path;

    int type{ 0 };
    std::getline(iss, tmp, def::FD);
    std::istringstream(tmp) >> type;

    int permissions{ 0 };
    std::getline(iss, tmp, def::FD);
    std::istringstream(tmp) >> permissions;

    size_t size{ 0 };
    std::getline(iss, tmp, def::FD);
    std::istringstream(tmp) >> size;


    boost::filesystem::file_status st((boost::filesystem::file_type)type, (boost::filesystem::perms)permissions);
    directory_entry d(path, st, st);


    try
    {
        if (is_regular_file(d) && !is_symlink(d)) {
            std::ofstream ofs(d.path().string());
            ofs.seekp(size);
            ofs.close();
            resize_file(d.path(), size);
        }
        else if (is_directory(d)) {
            create_directories(d.path());
        }
        else if (is_symlink(d)) {
            auto target_path = std::string{};
            std::getline(iss, tmp, def::FD);
            std::istringstream(tmp) >> target_path;

            boost::system::error_code ec;
            create_symlink(target_path, path, ec);

            //TBD
        }
        else {
            switch ((boost::filesystem::file_type)type)
            {
            case boost::filesystem::file_type::block_file:
                BOOST_LOG_TRIVIAL(warning) << path << " is a block_file, skipping";
                break;

            case boost::filesystem::file_type::character_file:
                BOOST_LOG_TRIVIAL(warning) << path << " is a character_file , skipping";
                break;

            case boost::filesystem::file_type::fifo_file:
                BOOST_LOG_TRIVIAL(warning) << path << " is a fifo_file , skipping";
                break;

            case boost::filesystem::file_type::socket_file:
                BOOST_LOG_TRIVIAL(warning) << path << " is a socket_file , skipping";
                break;

            case boost::filesystem::file_type::file_not_found:
                BOOST_LOG_TRIVIAL(error) << path << " is a file_not_found !??, skipping";
                break;
            case boost::filesystem::file_type::status_error:
                BOOST_LOG_TRIVIAL(error) << path << " is a status_error !??, skipping";
                break;
            case boost::filesystem::file_type::type_unknown:
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
            cout << path << " type:" << type << " perm:" << permissions << endl;
    }
    catch (const filesystem_error& ex)
    {
        BOOST_LOG_TRIVIAL(trace) << "\r" << ex.what();
        return false;
    }

    return true;
}

bool dir_layout_copier_c::_restore_dir_layout()
{

    if (_is_verbose)
    BOOST_LOG_TRIVIAL(info) << "Trying to restore layout from file " << _file;

    path p(_file);
    if (!exists(p)) {
        BOOST_LOG_TRIVIAL(error) << "Input file does not exist! ";
        return false;
    }


    BOOST_LOG_TRIVIAL(info) << "Restore directory path: " << _target_dir;
    path prd(_target_dir);
    if (!exists(prd)) {
        BOOST_LOG_TRIVIAL(error) << "Specified restore directory does not exist! ";
        return false;
    }


    std::ifstream ifs(_file, std::ios_base::in | std::ios_base::binary);
    short header{ 0 }; // = ifs.peek();
    ifs.read((char*)&header, 2);

    if (0x1f8b == htons(header)) {
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
        
        //on windows cut off drive letter & normalize path?
        if (line.size() >= 3 && line[1] == ':' && line[2] == '\\') {
            line[1] = '/';
            line[2] = '/';
        }

#ifdef _WIN32
        string sep{"\\"};
#else
        string sep{ "/" };
#endif
        _restore_entry(_target_dir + sep + line, retry_list); //prefix path iwith restore dir path
        line.clear();

        //TBD : show restore progress % 
    }

    ifs.close();

    return 0;
}

using namespace std;



void show_tick(size_t& tick_num) {
    string t{ "-\\|+/-\\|/" };
    cout << t[tick_num % 8] << "\r";
    tick_num++;
}

int dir_layout_copier_c::_capture_dir_layout()
{
    if (_is_verbose)
    BOOST_LOG_TRIVIAL(info) << "Capturing dir layout for " << _target_dir;


    path p(_target_dir);
    
    deque<pair<path,string>> dq;


    dq.push_front(pair<path,string>(p,"ROOT_DIR"));

    while (dq.size())
    {

        try
        {

            pair<path,string> p = dq.front();
            dq.pop_front();

            if (!exists(p.first)) {
                BOOST_LOG_TRIVIAL(warning) << p.first << "does not exist\n";
                continue;
            }

            auto dentry = directory_entry(p.first);
            pair<string,string> anon(p.second,"");
            _save_file_info(dentry,&anon);

            if (is_directory(p.first))
            {

                for (directory_entry& x : directory_iterator(p.first))
                {
                    try
                    {
                        if (is_directory(x))
                            dq.push_front(pair<path,string>(x,anon.second));
                        else
                        {

                            // add link target into dir layout first
                            // so that during restore link could be created to restored target

                            //TBD - add links support for anonymized mode ...
                            if (is_symlink(x.symlink_status()) && !_is_anonymize)
                            {
                                directory_entry link_target = directory_entry(read_symlink(x.path()));
                                _save_file_info(link_target);
                            }

                            pair<string,string> a(p.second,"");
                            _save_file_info(x,&a);
                        }
                    }
                    catch (const filesystem_error& ex)
                    {
                        BOOST_LOG_TRIVIAL(trace) << ex.what();
                    }
                }
            }

        } // try

        catch (const filesystem_error& ex) { BOOST_LOG_TRIVIAL(trace) << ex.what(); }


    } // while

    if (_is_progress)
    {
        cout << "\r"
            << "Found dirs:" << _dnum << " files:" << _fnum << "  Total:" << _dnum + _fnum << endl;;
    }

    return 0;
}


dir_layout_copier_c::~dir_layout_copier_c() {

    if (_is_compress && _is_scan_mode && _out.size()) {
        _out.flush();
        _out.pop();
        _out.pop();
    }

    if (_pdesc)
        delete(_pdesc);

}


int dir_layout_copier_c::run() {

    if (_is_scan_mode)
        return _capture_dir_layout();

    if (_is_restore_mode)
        return _restore_dir_layout();

    //never reached 
    assert(0);
    return 0;
}

int main(int argc, char** argv)
{

    dir_layout_copier_c dlc;

    if (!dlc.init(argc, argv))
        return -1;

    return dlc.run();
}