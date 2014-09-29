#include "FS.h"
 
#include <cstring>
#include <memory>
#include <fstream

int main(int argc, char** argv) {
    std::string file_path;
    
    std::shared_ptr<std::fstream> ggg(new std::fstream), fff;
    
    if (argc == 2) {
        file_path = argv[1]; 
    } else {
        file_path = "/media/alex/DATA/big_ext";
        //file_path = "/media/alex/DATA/flash_img.img";
        //printf("fail\n");
        //return 0;
    }

    std::ofstream output, meta_output;
    output.open("./out.txt");
    meta_output.open("./meta_out.txt");
    
    if (!output.is_open() || !meta_output.is_open()) {
        throw std::runtime_error("ERROR WITH FILES");
    }
    
    DiskOverRegFile disk(file_path);
    FS file_sys(&disk);

    file_sys.analize(output, meta_output);
    
    output.close();
    meta_output.close();

    return 0;
}
