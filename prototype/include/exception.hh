#ifndef EXCEPTION_HH
#define EXCEPTION_HH

#define EXCEPTION(name)                                                                  \
    class name : public std::exception {                                                 \
    public:                                                                              \
        constexpr name(std::string &&message) : message(std::move(message)) {}           \
        constexpr const char *what() const noexcept override { return message.c_str(); } \
                                                                                         \
    private:                                                                             \
        std::string message;                                                             \
    }

#endif
