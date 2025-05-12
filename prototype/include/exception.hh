#ifndef EXCEPTION_HH
#define EXCEPTION_HH

#include <format>
#include <system_error>
#include <utility>

#define EXCEPTION(name)                                                                  \
    class name : public std::exception {                                                 \
    public:                                                                              \
        constexpr name(std::string&& message) : message(std::move(message)) {}           \
        constexpr const char* what() const noexcept override { return message.c_str(); } \
                                                                                         \
    private:                                                                             \
        std::string message;                                                             \
    }

namespace error {

template <typename E>
concept Error = requires(E e) { std::formatter<E>{}; };

struct SimTraceError {
    const std::errc internal;
};

struct SystemError {
    const std::errc internal;

    constexpr auto isSocketError() const {
        switch (internal) {
            case std::errc::address_family_not_supported:
                return true;
            case std::errc::address_in_use:
                return true;
            case std::errc::address_not_available:
                return true;
            case std::errc::already_connected:
                return true;
            case std::errc::bad_address:
                return true;
            case std::errc::bad_file_descriptor:
                return true;
            case std::errc::bad_message:
                return true;
            case std::errc::broken_pipe:
                return true;
            case std::errc::connection_aborted:
                return true;
            case std::errc::connection_already_in_progress:
                return true;
            case std::errc::connection_refused:
                return true;
            case std::errc::connection_reset:
                return true;
            case std::errc::host_unreachable:
                return true;
            case std::errc::interrupted:
                return true;
            case std::errc::network_down:
                return true;
            case std::errc::network_reset:
                return true;
            case std::errc::network_unreachable:
                return true;
            case std::errc::not_a_socket:
                return true;
            case std::errc::operation_would_block:
                return true;
            default:
                return false;
        }

        std::unreachable();
    }
};
}

template <>
struct std::formatter<error::SystemError> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }

    auto format(const error::SystemError& error, std::format_context& context) const {
        auto codeToString = [&error = error.internal] {
            switch (error) {
                case std::errc::address_family_not_supported:
                    return "address family not supported"sv;
                case std::errc::address_in_use:
                    return "address in use"sv;
                case std::errc::address_not_available:
                    return "address not available"sv;
                case std::errc::already_connected:
                    return "already connected"sv;
                case std::errc::argument_list_too_long:
                    return "argument list too long"sv;
                case std::errc::argument_out_of_domain:
                    return "argument out of domain"sv;
                case std::errc::bad_address:
                    return "bad address"sv;
                case std::errc::bad_file_descriptor:
                    return "bad file descriptor"sv;
                case std::errc::bad_message:
                    return "bad message"sv;
                case std::errc::broken_pipe:
                    return "broken pipe"sv;
                case std::errc::connection_aborted:
                    return "connection aborted"sv;
                case std::errc::connection_already_in_progress:
                    return "connection already in progress"sv;
                case std::errc::connection_refused:
                    return "connection refused"sv;
                case std::errc::connection_reset:
                    return "connection reset"sv;
                case std::errc::cross_device_link:
                    return "cross device link"sv;
                case std::errc::destination_address_required:
                    return "destination address required"sv;
                case std::errc::device_or_resource_busy:
                    return "device or resource busy"sv;
                case std::errc::directory_not_empty:
                    return "directory not empty"sv;
                case std::errc::executable_format_error:
                    return "executable format error"sv;
                case std::errc::file_exists:
                    return "file exists"sv;
                case std::errc::file_too_large:
                    return "file too large"sv;
                case std::errc::filename_too_long:
                    return "filename too long"sv;
                case std::errc::function_not_supported:
                    return "function not supported"sv;
                case std::errc::host_unreachable:
                    return "host unreachable"sv;
                case std::errc::identifier_removed:
                    return "identifier removed"sv;
                case std::errc::illegal_byte_sequence:
                    return "illegal byte sequence"sv;
                case std::errc::inappropriate_io_control_operation:
                    return "inappropriate io control operation"sv;
                case std::errc::interrupted:
                    return "interrupted"sv;
                case std::errc::invalid_argument:
                    return "invalid argument"sv;
                case std::errc::invalid_seek:
                    return "invalid seek"sv;
                case std::errc::io_error:
                    return "io error"sv;
                case std::errc::is_a_directory:
                    return "is a directory"sv;
                case std::errc::message_size:
                    return "message size"sv;
                case std::errc::network_down:
                    return "network down"sv;
                case std::errc::network_reset:
                    return "network reset"sv;
                case std::errc::network_unreachable:
                    return "network unreachable"sv;
                case std::errc::no_buffer_space:
                    return "no buffer space"sv;
                case std::errc::no_child_process:
                    return "no child process"sv;
                case std::errc::no_link:
                    return "no link"sv;
                case std::errc::no_lock_available:
                    return "no lock available"sv;
                case std::errc::no_message_available:
                    return "no message available"sv;
                case std::errc::no_message:
                    return "no message"sv;
                case std::errc::no_protocol_option:
                    return "no protocol option"sv;
                case std::errc::no_space_on_device:
                    return "no space on device"sv;
                case std::errc::no_stream_resources:
                    return "no stream resources"sv;
                case std::errc::no_such_device_or_address:
                    return "no such device or address"sv;
                case std::errc::no_such_device:
                    return "no such device"sv;
                case std::errc::no_such_file_or_directory:
                    return "no such file or directory"sv;
                case std::errc::no_such_process:
                    return "no such process"sv;
                case std::errc::not_a_directory:
                    return "not a directory"sv;
                case std::errc::not_a_socket:
                    return "not a socket"sv;
                case std::errc::not_a_stream:
                    return "not a stream"sv;
                case std::errc::not_connected:
                    return "not connected"sv;
                case std::errc::not_enough_memory:
                    return "not enough memory"sv;
                case std::errc::not_supported:
                    return "not supported"sv;
                case std::errc::operation_canceled:
                    return "operation canceled"sv;
                case std::errc::operation_in_progress:
                    return "operation in progress"sv;
                case std::errc::operation_not_permitted:
                    return "operation not permitted"sv;
                case std::errc::operation_would_block:
                    return "operation would block"sv;
                case std::errc::owner_dead:
                    return "owner dead"sv;
                case std::errc::permission_denied:
                    return "permission denied"sv;
                case std::errc::protocol_error:
                    return "protocol error"sv;
                case std::errc::protocol_not_supported:
                    return "protocol not supported"sv;
                case std::errc::read_only_file_system:
                    return "read only file system"sv;
                case std::errc::resource_deadlock_would_occur:
                    return "resource deadlock would occur"sv;
                case std::errc::result_out_of_range:
                    return "result out of range"sv;
                case std::errc::state_not_recoverable:
                    return "state not recoverable"sv;
                case std::errc::stream_timeout:
                    return "stream timeout"sv;
                case std::errc::text_file_busy:
                    return "text file busy"sv;
                case std::errc::timed_out:
                    return "timed out"sv;
                case std::errc::too_many_files_open_in_system:
                    return "too many files open in system"sv;
                case std::errc::too_many_files_open:
                    return "too many files open"sv;
                case std::errc::too_many_links:
                    return "too many links"sv;
                case std::errc::too_many_symbolic_link_levels:
                    return "too many symbolic link levels"sv;
                case std::errc::value_too_large:
                    return "value too large"sv;
                case std::errc::wrong_protocol_type:
                    return "wrong protocol type"sv;
            }

            std::unreachable();
        };

        return std::format_to(context.out(), "{}", codeToString());
    }
};

#endif
