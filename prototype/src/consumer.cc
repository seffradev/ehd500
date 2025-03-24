#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <print>
#include <thread>

#include "atp.hh"

extern "C" {
// libusb.h uses the keyword `class`. This is an escape hatch to forgo this
#define class _class
#include <osmocom/sim/class_tables.h>
#include <osmocom/sim/sim.h>
#include <osmocom/simtrace2/apdu_dispatch.h>
#include <osmocom/simtrace2/gsmtap.h>
#include <osmocom/simtrace2/simtrace2_api.h>
#include <osmocom/simtrace2/simtrace_prot.h>
#include <osmocom/simtrace2/simtrace_usb.h>
#include <osmocom/usb/libusb.h>
#undef class
};

#include "exception.hh"
#include "tcp.hh"

using namespace std::literals;

EXCEPTION(UsbException);
EXCEPTION(GsmTapException);

class SimTrace2 {
public:
    constexpr SimTrace2() {
        transport = std::make_unique<osmo_st2_transport>();
        slot      = std::make_unique<osmo_st2_slot>(transport.get(), 0);
        instance  = std::make_unique<osmo_st2_cardem_inst>(slot.get(), &osim_uicc_sim_cic_profile);

        // auto cardem_config =
        //     cardemu_usb_msg_config{.features = CEMU_FEAT_F_STATUS_IRQ};

        auto result_code = osmo_libusb_init(nullptr);
        if (result_code < 0) {
            throw UsbException{"libusb initialization failed"};
        }

        result_code = osmo_st2_gsmtap_init(gsmtapHost);
        if (result_code < 0) {
            throw GsmTapException("unable to open GSMTAP");
        }

        auto interface_match = std::make_unique<usb_interface_match>();
        memset(interface_match.get(), 0, sizeof(usb_interface_match));
        interface_match->vendor        = vendorId;
        interface_match->product       = productId;
        interface_match->configuration = configuration.value_or(-1);
        interface_match->interface     = interface;
        interface_match->altsetting    = altsetting;

        transport->udp_fd    = udpFd.value_or(-1);
        transport->usb_async = usbAsync;
        transport->usb_devh  = osmo_libusb_open_claim_interface(nullptr, nullptr, interface_match.get());
        if (!transport->usb_devh) {
            throw UsbException{"can't open USB device"};
        }

        result_code = libusb_claim_interface(transport->usb_devh, interface);
        if (result_code < 0) {
            throw UsbException{"can't claim interface"};
        }

        result_code = osmo_libusb_get_ep_addrs(transport->usb_devh, interface, &transport->usb_ep.out,
                                               &transport->usb_ep.in, &transport->usb_ep.irq_in);
        if (result_code < 0) {
            throw UsbException{"can't obtain EP address interface"};
        }
    }

    constexpr ~SimTrace2() {
        if (transport->usb_devh) {
            libusb_release_interface(transport->usb_devh, 0);
            libusb_close(transport->usb_devh);
            transport->usb_devh = nullptr;
        }

        osmo_libusb_exit(nullptr);
    }

private:
    static constexpr uint16_t                             vendorId      = USB_VENDOR_OPENMOKO;
    static constexpr uint16_t                             productId     = USB_PRODUCT_SIMTRACE2_DFU;
    static constexpr auto                                 gsmtapHost    = "127.0.0.1";
    static constexpr std::optional<uint8_t>               configuration = 1;
    static constexpr uint8_t                              interface     = 0;
    static constexpr uint8_t                              altsetting    = 0;
    static constexpr std::optional<uint8_t>               address       = std::nullopt;
    static constexpr std::optional<std::filesystem::path> path          = std::nullopt;
    static constexpr std::optional<uint16_t>              udpFd         = std::nullopt;
    static constexpr bool                                 usbAsync      = true;

    std::unique_ptr<osmo_st2_cardem_inst> instance;
    std::unique_ptr<osmo_st2_slot>        slot;
    std::unique_ptr<osmo_st2_transport>   transport;
};

int main(int, char *[]) {
    try {
        auto trace = SimTrace2{};
    } catch (std::exception &e) {
        std::println(stderr, "Failed to construct SIMtrace instance: {}", e.what());
    }

    auto client = TcpClient{"127.0.0.1", 9000};

    auto read = std::jthread{[&client] {
        auto message = static_cast<Atp>(client.read());
        std::println("{}", message);
    }};

    client.write("hello"s);

    return 0;
}
