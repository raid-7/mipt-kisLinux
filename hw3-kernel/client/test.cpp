#include <iostream>
#include <fstream>
#include <stdexcept>

extern "C" {
#include "../kernel/phonedir.h"
}

#include "CLI11.hpp"

std::ostream& operator<<(std::ostream& stream, const phonedir_record& record) {
    stream << record.surname << ' ' << record.name << ' ' << record.phone << ' ' << record.email << ' ' << record.age;
    return stream;
}

std::string surname_to_op_string(const std::string& surname) {
    if (surname.size() + 1 > PHONEDIR_NAME_LEN) {
        throw std::runtime_error("Surname is too long");
    }
    std::string data(PHONEDIR_NAME_LEN + 1, '\0');
    std::copy(surname.begin(), surname.end(), data.begin() + 1);
    return data;
}

void copy_normalized(const std::string& from, char* to, size_t maxlen) {
    if (from.size() + 1 > maxlen) {
        throw std::runtime_error("Parameter is too long");
    }
    std::copy(from.begin(), from.end(), to);
    std::fill(to + from.size(), to + maxlen, '\0');
}

void preform_add(std::fstream& stream, const std::string& surname, const std::string& name,
                 const std::string& phone, const std::string& email, unsigned int age) {
    phonedir_record record{};
    copy_normalized(surname, record.surname, PHONEDIR_NAME_LEN);
    copy_normalized(name, record.name, PHONEDIR_NAME_LEN);
    copy_normalized(email, record.email, PHONEDIR_EMAIL_LEN);
    copy_normalized(phone, record.phone, PHONEDIR_PHONE_LEN);
    record.age = age;

    std::string payload(reinterpret_cast<char*>(&record), sizeof(record));
    stream << static_cast<char>(PHONEDIR_ADD) << payload;
    stream.flush();
}

void preform_search(std::fstream& stream, const std::string& surname) {
    auto data = surname_to_op_string(surname);
    data[0] = PHONEDIR_FIND;
    stream << data;
    stream.flush();

    phonedir_record record{};
    char* record_ptr = reinterpret_cast<char*>(&record);
    std::streamsize read;
    while ((read = stream.readsome(record_ptr, sizeof(record))) == sizeof(record)) {
        std::cout << record << std::endl;
    }
    if (read) {
        throw std::runtime_error("Strange results...");
    }
}

void perform_delete(std::fstream& stream, std::string surname) {
    auto data = surname_to_op_string(surname);
    data[0] = PHONEDIR_DELETE;
    stream << data;
    stream.flush();
}

int main(int argc, const char* argv[]) {
    std::fstream stream;

    CLI::App app{"Phonedir client"};

    app.add_option_function<std::string>("-d", [&stream](const std::string& dname) mutable {
        stream = std::fstream(dname);
    }, "Device name")->required();

    CLI::App* add_sbc = app.add_subcommand("add");
    CLI::App* del_sbc = app.add_subcommand("del");
    CLI::App* find_sbc = app.add_subcommand("find");

    std::string name, surname, phone, email;
    unsigned int age = 0;

    add_sbc->add_option("surname", surname)->required();
    add_sbc->add_option("name", name)->required();
    add_sbc->add_option("phone", phone)->required();
    add_sbc->add_option("email", email)->required();
    add_sbc->add_option("age", age)->required();

    find_sbc->add_option("surname", surname)->required();
    del_sbc->add_option("surname", surname)->required();

    add_sbc->callback([&] {
        preform_add(stream, name, surname, phone, email, age);
    });
    find_sbc->callback([&] {
        preform_search(stream, surname);
    });
    del_sbc->callback([&] {
        perform_delete(stream, surname);
    });

    app.require_subcommand();

    CLI11_PARSE(app, argc, argv)
}
