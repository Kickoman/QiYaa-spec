#include "vectors.h"

#include <fstream>
#include <iostream>
#include <utility>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: spotify-vectors <directory to write the spotify/*.json files into>\n";
        return 2;
    }
    const std::string directory = argv[1];
    const std::pair<const char*, Vectors::Json (*)()> files[] = {
        {"shannon.json", Vectors::ShannonVectors},
        {"crypto.json", Vectors::CryptoVectors},
        {"audio.json", Vectors::AudioVectors},
        {"handshake.json", Vectors::HandshakeVectors},
        {"protobuf.json", Vectors::ProtobufVectors},
        {"ids.json", Vectors::IdVectors},
        {"messages.json", Vectors::MessageVectors},
    };
    for (const auto& [name, make] : files) {
        const std::string text = make().dump();
        const std::string path = directory + "/" + name;
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << text;
        out.close();
        Vectors::Check(static_cast<bool>(out), "could not write " + path);
        std::cout << path << ": " << text.size() << " bytes\n";
    }
    return 0;
}
