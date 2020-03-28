#include <iostream>
#include <stdexcept>
#include <utility>

extern "C" {
#include "../kernel/phonedir.h"
#include "calls.h"
}

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


#include "CLI11.hpp"

class CError : public std::runtime_error {
public:
    explicit CError(const std::string& prefix)
            : runtime_error(prefix + ": " + strerror(errno)) {
        errno = 0;
    }
};

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

void preform_add(const std::string& surname, const std::string& name,
                 const std::string& phone, const std::string& email, unsigned int age) {
    phonedir_record record{};
    copy_normalized(surname, record.surname, PHONEDIR_NAME_LEN);
    copy_normalized(name, record.name, PHONEDIR_NAME_LEN);
    copy_normalized(email, record.email, PHONEDIR_EMAIL_LEN);
    copy_normalized(phone, record.phone, PHONEDIR_PHONE_LEN);
    record.age = age;

    if (phonedir_add(&record)) {
        throw CError("Add failed");
    }
}

void preform_search(const std::string& surname) {
    phonedir_record record{};
    long res = phonedir_find(surname.c_str(), surname.size(), &record);
    if (res < 0) {
        throw CError("Search failed");
    }
    if (res) {
        std::cout << record << std::endl;
    }
}

void perform_delete(std::string surname) {
    if (phonedir_del(surname.c_str(), surname.size())) {
        throw CError("Delete failed");
    }
}

int main(int argc, const char* argv[]) {
    CLI::App app{"Phonedir client"};

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

    add_sbc->callback([&surname, &name, &phone, &email, &age] {
        preform_add(surname, name, phone, email, age);
    });
    find_sbc->callback([&surname] {
        preform_search(surname);
    });
    del_sbc->callback([&surname] {
        perform_delete(surname);
    });

    app.require_subcommand();

    CLI11_PARSE(app, argc, argv)
}
