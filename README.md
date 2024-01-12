### What is it
This is the tool which re-creates directory/file structure for testing 
purposes. It recursevely scans dir/file structure(layout) and saves into 
intermediate file (can be compressed). Then it allows to restore same layout
on another env for testing purposes ( i.e. re-creates same directory 
structure and files. The actual file's data is not re-created - we truncate 
files to originaslsize and set permissions and attributes ) 

```
Usage: dir_layout [--help | [ [--scan <path>] | [--restore <path> ] [--file <path>] [--compress] [--verbose] [--progress]] ]
Allowed options:
  --help                produce help message
  --scan arg            scans directory <arg> for it's layout/contents
  --compress            gzip/compress scan output file
  --verbose             noisy output
  --progress            report some stats
  --file arg            input/output file for dir layout
  --restore arg         restore layout under specified directory

Examples - 
To capture dir layout of /tmp/source:
./dir_layout --scan /tmp/source --file contents.gz --compress --verbose --progress

To recreate captured layout:
./dir_layout --restore /tmp/restore --file contents.gz --verbose 
```

### Building on Unix/Linux  
Install boost libraries "sudo apt install libboost-all-dev"

```
mkdir build && cd build
cmake  ../src/ -DCMAKE_BUILD_TYPE=Debug
make 
```
### Building on Windows 
Donwload boost (https://www.boost.org/) and compile with zlib enabled.
E.g: 
```
.\bootstrap.bat
.\b2 -j15 --toolset=msvc --build-type=complete stage -s \ 
    ZLIB_SOURCE="C:\zlib-1.2.8" -s ZLIB_INCLUDE="C:\zlib-1.2.8" 
```

If needed, adjust path to boost includes and libs in dir_layout VS project 
(created in VS2022)

