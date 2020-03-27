#include <iostream>
#include <stdexcept>
#include <utility>

extern "C" {
#include "../kernel/phonedir.h"
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
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

class Deferred {
private:
    std::function<void(void)> func;

public:
    template <class T>
    Deferred(const T& mfunc) : func(mfunc) {}

    ~Deferred() {
        func();
    }
};

#define defer(a) const Deferred a =

void write_all(int fd, const char* buf, size_t len) {
    ssize_t done;
    while (len > 0 && (done = write(fd, buf, len)) >= 0) {
        len -= done;
        buf += done;
    }
    if (done < 0) {
        throw CError("Write failed");
    }
}

bool read_quant(int fd, char* buf, size_t len) {
    ssize_t done;
    bool iterated = false;
    while (len > 0 && (done = read(fd, buf, len)) > 0) {
        len -= done;
        buf += done;
        iterated = true;
    }
    if (done < 0) {
        throw CError("Read failed");
    }
    if (iterated && len) {
        throw std::runtime_error("Cannot read whole quant");
    }
    return len == 0;
}

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

void preform_add(int fd, const std::string& surname, const std::string& name,
                 const std::string& phone, const std::string& email, unsigned int age) {
    phonedir_record record{};
    copy_normalized(surname, record.surname, PHONEDIR_NAME_LEN);
    copy_normalized(name, record.name, PHONEDIR_NAME_LEN);
    copy_normalized(email, record.email, PHONEDIR_EMAIL_LEN);
    copy_normalized(phone, record.phone, PHONEDIR_PHONE_LEN);
    record.age = age;

    auto payload = static_cast<char>(PHONEDIR_ADD) + std::string(reinterpret_cast<char*>(&record), sizeof(record));
    write_all(fd, payload.data(), payload.size());
}

void preform_search(int fd, const std::string& surname) {
    auto data = surname_to_op_string(surname);
    data[0] = PHONEDIR_FIND;
    write_all(fd, data.data(), data.size());

    phonedir_record record{};
    char* record_ptr = reinterpret_cast<char*>(&record);
    while (read_quant(fd, record_ptr, sizeof(phonedir_record))) {
        std::cout << record << std::endl;
    }
}

void perform_delete(int fd, std::string surname) {
    auto data = surname_to_op_string(surname);
    data[0] = PHONEDIR_DELETE;
    write_all(fd, data.data(), data.size());
}

int main(int argc, const char* argv[]) {
    int fd = -1;
    CLI::App app{"Phonedir client"};

    app.add_option_function<std::string>("-d", [&fd](const std::string& dname) mutable {
        fd = open(dname.c_str(), O_RDWR);
        if (fd == -1) {
            throw CError("Cannot open device");
        }
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

    add_sbc->callback([&fd, &surname, &name, &phone, &email, &age] {
        preform_add(fd, surname, name, phone, email, age);
    });
    find_sbc->callback([&fd, &surname] {
        preform_search(fd, surname);
    });
    del_sbc->callback([&fd, &surname] {
        perform_delete(fd, surname);
    });

    app.require_subcommand();

    CLI11_PARSE(app, argc, argv)
}
