#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << argv[1] << "\n";
        return 1;
    }

    int total_keys = 0;
    int freq_one_count = 0;
    std::string line;

    while (std::getline(file, line))
    {
        std::size_t pos = line.find_last_of(',');
        if (pos != std::string::npos)
        {
            int freq = std::stoi(line.substr(pos + 1));
            total_keys++;
            if (freq == 1)
            {
                freq_one_count++;
            }
        }
    }

    file.close();

    std::cout << "Total number of keys: " << total_keys << "\n";
    if (total_keys > 0)
    {
        double percentage = (freq_one_count * 100.0) / total_keys;
        std::cout << "Percentage of keys with freq=1: " << percentage << "%\n";
    }
    else
    {
        std::cout << "No valid entries found in the file.\n";
    }

    return 0;
}
