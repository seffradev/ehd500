#ifndef SIMTRACE_HH
#define SIMTRACE_HH

#include <libusb.h>

#include <cassert>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <source_location>
#include <string_view>
#include <type_traits>

extern "C" {
// libusb.h uses the keyword `class`. This is an escape hatch to forgo this
#define class _class
#include <osmocom/core/select.h>
#include <osmocom/core/utils.h>
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

#include "data.hh"
#include "exception.hh"

EXCEPTION(UsbException);
EXCEPTION(GsmTapException);

class SimTrace2 {
public:
    static constexpr auto create() -> std::expected<SimTrace2, SimTraceError> {
        auto trace = SimTrace2{};

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

        trace.transport->udp_fd    = udpFd.value_or(-1);
        trace.transport->usb_async = usbAsync;
        trace.transport->usb_devh  = osmo_libusb_open_claim_interface(nullptr, nullptr, interface_match.get());
        if (!trace.transport->usb_devh) {
            throw UsbException{"can't open USB device"};
        }

        result_code = libusb_claim_interface(trace.transport->usb_devh, interface);
        if (result_code < 0) {
            throw UsbException{"can't claim interface"};
        }

        result_code = osmo_libusb_get_ep_addrs(trace.transport->usb_devh, interface, &trace.transport->usb_ep.out,
                                               &trace.transport->usb_ep.in, &trace.transport->usb_ep.irq_in);
        if (result_code < 0) {
            throw UsbException{"can't obtain EP address interface"};
        }

        trace.allocateAndSubmit();

        osmo_st2_cardem_request_config(trace.instance.get(), CEMU_FEAT_F_STATUS_IRQ);

        return trace;
    }

    constexpr SimTrace2(SimTrace2 &&trace) {
        std::swap(instance, trace.instance);
        std::swap(slot, trace.slot);
        std::swap(transport, trace.transport);
        std::swap(hasCard, trace.hasCard);
        std::swap(lastStatusFlags, trace.lastStatusFlags);
        trace.moved = true;
        moved       = false;
    }

    constexpr SimTrace2 &operator=(SimTrace2 &&trace) {
        std::swap(instance, trace.instance);
        std::swap(slot, trace.slot);
        std::swap(transport, trace.transport);
        std::swap(hasCard, trace.hasCard);
        std::swap(lastStatusFlags, trace.lastStatusFlags);
        trace.moved = true;
        moved       = false;
        return *this;
    }

    constexpr auto run() {
        if (!hasCard) {
            return;
        }

        osmo_select_main(0);
    }

    constexpr auto setCard(GenericData atr) {
        osmo_st2_cardem_request_card_insert(instance.get(), true);
        osmo_st2_modem_sim_select_remote(instance->slot);
        osmo_st2_cardem_request_set_atr(instance.get(), reinterpret_cast<const uint8_t *>(atr.data()), atr.size());
        osmo_st2_modem_reset_pulse(slot.get(), 300);
        hasCard = true;
    }

    constexpr auto unsetCard() {
        hasCard = false;
        osmo_st2_cardem_request_card_insert(instance.get(), false);
        osmo_st2_modem_sim_select_local(instance->slot);
    }

    constexpr ~SimTrace2() {
        if (moved) {
            return;
        }

        libusb_release_interface(transport->usb_devh, 0);

        if (transport->usb_devh) {
            libusb_close(transport->usb_devh);
            transport->usb_devh = nullptr;
        }

        osmo_libusb_exit(nullptr);
    }

private:
    constexpr SimTrace2()
        : transport(std::make_unique<osmo_st2_transport>()),
          slot(std::make_unique<osmo_st2_slot>(transport.get(), 0)),
          instance(std::make_unique<osmo_st2_cardem_inst>(slot.get(), &osim_uicc_sim_cic_profile)) {}

    static void updateStatusFlags(SimTrace2 &trace, uint32_t flags) {
        auto card  = trace.instance->chan->card;
        auto reset = std::optional<Reset>{std::nullopt};

        if ((flags & CEMU_STATUS_F_VCC_PRESENT) && (flags & CEMU_STATUS_F_CLK_ACTIVE) &&
            !(flags & CEMU_STATUS_F_RESET_ACTIVE)) {
            if (trace.lastStatusFlags & CEMU_STATUS_F_RESET_ACTIVE) {
                if (trace.lastStatusFlags & CEMU_STATUS_F_VCC_PRESENT)
                    reset = Reset::Warm;
                else
                    reset = Reset::Cold;
            } else if (!(trace.lastStatusFlags & CEMU_STATUS_F_VCC_PRESENT)) {
                reset = Reset::Cold;
            }
        } else if (flags == CEMU_STATUS_F_VCC_PRESENT && !(trace.lastStatusFlags & CEMU_STATUS_F_VCC_PRESENT)) {
            reset = Reset::Cold;
        }

        if (reset) {
            std::println("{} Resetting card in reader...", toString(*reset));
            osim_card_reset(card, reset == Reset::Cold ? true : false);
            osmo_st2_gsmtap_send_apdu(GSMTAP_SIM_ATR, card->atr, card->atr_len);
        }

        trace.lastStatusFlags = flags;
    }

    static auto cemuStatusFlagsToString(uint32_t flags) {
        auto reset    = (flags & CEMU_STATUS_F_RESET_ACTIVE) ? "RESET " : "";
        auto vcc      = (flags & CEMU_STATUS_F_VCC_PRESENT) ? "VCC " : "";
        auto clk      = (flags & CEMU_STATUS_F_CLK_ACTIVE) ? "CLK " : "";
        auto cardPres = (flags & CEMU_STATUS_F_CARD_INSERT) ? "CARD_PRES " : "";
        auto rcemu    = (flags & CEMU_STATUS_F_RCEMU_ACTIVE) ? "RCEMU " : "";
        return std::format("{}{}{}{}{}", reset, vcc, clk, cardPres, rcemu);
    }

    static auto processIrqStatus(SimTrace2 &trace, const uint8_t *buffer, int /* length */) {
        const cardemu_usb_msg_status *status = reinterpret_cast<const cardemu_usb_msg_status *>(buffer);

        auto flagsBuffer = cemuStatusFlagsToString(status->flags);

        std::println(std::cerr, "=> IRQ STATUS: flags: {:#04x}, fi: {}, di: {}, wi: {}, wtime: {} ({})", status->flags,
                     status->fi, status->di, status->wi, status->waiting_time, flagsBuffer);

        updateStatusFlags(trace, status->flags);

        return 0;
    }

    static auto processUsbMsgIrq(SimTrace2 &trace, const uint8_t *buffer, unsigned int length) {
        struct simtrace_msg_hdr *sh = (struct simtrace_msg_hdr *)buffer;

        buffer += sizeof(*sh);

        switch (sh->msg_type) {
            case SIMTRACE_MSGT_BD_CEMU_STATUS:
                return processIrqStatus(trace, buffer, length);
            default:
                std::println(std::cerr, "unknown SIMtrace message type {}", sh->msg_type);
                return -1;
        }
    }

    static auto usbIrqTransferCallback(struct libusb_transfer *transfer) {
        std::println("Called {}", std::source_location::current().function_name());
        auto trace = reinterpret_cast<SimTrace2 *>(transfer->user_data);

        switch (transfer->status) {
            case LIBUSB_TRANSFER_COMPLETED:
                processUsbMsgIrq(*trace, transfer->buffer, transfer->actual_length);
                break;
            case LIBUSB_TRANSFER_ERROR:
                std::println(std::cerr, "USB INT transfer error, trying resubmit");
                break;
            case LIBUSB_TRANSFER_NO_DEVICE:
                std::println(std::cerr, "USB device disappeared");
                exit(1);
                break;
            default:
                std::println(std::cerr, "USB INT transfer failed, status: {}",
                             static_cast<std::underlying_type_t<decltype(transfer->status)>>(transfer->status));
                exit(1);
                break;
        }

        /* re-submit the IN transfer */
        assert(libusb_submit_transfer(transfer));
    }

    auto allocateAndSubmitIrq() {
        libusb_transfer *transfer;

        transfer = libusb_alloc_transfer(0);
        assert(transfer);
        transfer->dev_handle = transport->usb_devh;
        transfer->flags      = 0;
        transfer->type       = LIBUSB_TRANSFER_TYPE_INTERRUPT;
        transfer->endpoint   = transport->usb_ep.irq_in;
        transfer->timeout    = 0;
        transfer->user_data  = this;
        transfer->length     = 64;

        transfer->buffer = libusb_dev_mem_alloc(transfer->dev_handle, transfer->length);
        assert(transfer->buffer);
        transfer->callback = usbIrqTransferCallback;

        /* submit the IN transfer */
        assert(libusb_submit_transfer(transfer) == 0);
    }

    static auto cemuDataFlagsToString(uint32_t flags) {
        auto header   = (flags & CEMU_DATA_F_TPDU_HDR) ? "HDR " : "";
        auto final    = (flags & CEMU_DATA_F_FINAL) ? "FINAL " : "";
        auto transfer = (flags & CEMU_DATA_F_PB_AND_TX) ? "PB_AND_TX " : "";
        auto receive  = (flags & CEMU_DATA_F_PB_AND_RX) ? "PB_AND_RX" : "";
        return std::format("{}{}{}{}", header, final, transfer, receive);
    }

    static int processDoStatus(SimTrace2 &trace, uint8_t *buffer, int /* length */) {
        struct cardemu_usb_msg_status *status = (struct cardemu_usb_msg_status *)buffer;

        auto flagsBuffer = cemuStatusFlagsToString(status->flags);

        std::println("=> STATUS: flags: {:#04x}, fi: {}, di: {}, wi: {}, wtime: {} ({})",
                     static_cast<uint32_t>(status->flags), status->fi, status->di, status->wi,
                     static_cast<uint32_t>(status->waiting_time), flagsBuffer);

        updateStatusFlags(trace, status->flags);

        return 0;
    }

    static int processDoPts(SimTrace2 & /* trace */, uint8_t *buffer, int /* length */) {
        struct cardemu_usb_msg_pts_info *pts;
        pts = (struct cardemu_usb_msg_pts_info *)buffer;

        std::println("=> PTS req: {}", osmo_hexdump(pts->req, pts->pts_len));

        return 0;
    }

    static int processDoReceiveData(SimTrace2 &trace, uint8_t *buffer, int /* length */) {
        static struct osmo_apdu_context ac;
        struct cardemu_usb_msg_rx_data *data;
        int                             rc;

        data = (struct cardemu_usb_msg_rx_data *)buffer;

        std::println("=> DATA: flags={:#04x} ({}), {}", static_cast<uint32_t>(data->flags),
                     cemuDataFlagsToString(data->flags), osmo_hexdump(data->data, data->data_len));

        rc = osmo_apdu_segment_in(&ac, data->data, data->data_len, data->flags & CEMU_DATA_F_TPDU_HDR);
        if (rc < 0) {
            std::println(std::cerr, "Failed to recognize APDU, terminating");
            exit(1);
        }

        if (rc & APDU_ACT_TX_CAPDU_TO_CARD) {
            struct msgb            *tmsg = msgb_alloc(1024, "TPDU");
            struct osim_reader_hdl *rh   = trace.instance->chan->card->reader;
            uint8_t                *cur;

            cur = msgb_put(tmsg, sizeof(ac.hdr));
            memcpy(cur, &ac.hdr, sizeof(ac.hdr));
            /* Copy D(c), if any */
            if (ac.lc.tot) {
                cur = msgb_put(tmsg, ac.lc.tot);
                memcpy(cur, ac.dc, ac.lc.tot);
            }
            /* send to actual card */
            tmsg->l3h = tmsg->tail;
            rc        = rh->ops->transceive(rh, tmsg);
            if (rc < 0) {
                std::println(std::cerr, "error during transceive: {}", rc);
                msgb_free(tmsg);
                return rc;
            }

            osmo_st2_gsmtap_send_apdu(GSMTAP_SIM_APDU, tmsg->data, msgb_length(tmsg));

            msgb_apdu_sw(tmsg) = msgb_get_u16(tmsg);
            ac.sw[0]           = msgb_apdu_sw(tmsg) >> 8;
            ac.sw[1]           = msgb_apdu_sw(tmsg) & 0xff;
            if (msgb_l3len(tmsg))
                osmo_st2_cardem_request_pb_and_tx(trace.instance.get(), ac.hdr.ins, tmsg->l3h, msgb_l3len(tmsg));
            osmo_st2_cardem_request_sw_tx(trace.instance.get(), ac.sw);
        } else if (ac.lc.tot > ac.lc.cur) {
            osmo_st2_cardem_request_pb_and_rx(trace.instance.get(), ac.hdr.ins, ac.lc.tot - ac.lc.cur);
        }
        return 0;
    }

    static int processUsbMessage(SimTrace2 &trace, uint8_t *buffer, int length) {
        auto simtrace_header = reinterpret_cast<simtrace_msg_hdr *>(buffer);

        buffer += sizeof(*simtrace_header);

        switch (simtrace_header->msg_type) {
            case SIMTRACE_MSGT_BD_CEMU_STATUS:
                return processDoStatus(trace, buffer, length);
            case SIMTRACE_MSGT_DO_CEMU_PTS:
                return processDoPts(trace, buffer, length);
            case SIMTRACE_MSGT_DO_CEMU_RX_DATA:
                return processDoReceiveData(trace, buffer, length);
            case SIMTRACE_MSGT_BD_CEMU_CONFIG:
                return 0;
            default:
                std::println("unknown SIMtrace message type {:#04x}", simtrace_header->msg_type);
                return -1;
        }
    }

    static void usbInTransferCallback(struct libusb_transfer *transfer) {
        std::println("Called {}", std::source_location::current().function_name());
        auto trace = reinterpret_cast<SimTrace2 *>(transfer->user_data);

        switch (transfer->status) {
            case LIBUSB_TRANSFER_COMPLETED:
                processUsbMessage(*trace, transfer->buffer, transfer->actual_length);
                break;
            case LIBUSB_TRANSFER_ERROR:
                std::println(std::cerr, "USB IN transfer error, trying resubmit");
                break;
            case LIBUSB_TRANSFER_NO_DEVICE:
                std::println(std::cerr, "USB device disappeared");
                exit(1);
                break;
            default:
                std::println(std::cerr, "USB IN transfer failed, status: {}",
                             static_cast<std::underlying_type_t<decltype(transfer->status)>>(transfer->status));
                exit(1);
                break;
        }

        /* re-submit the IN transfer */
        assert(libusb_submit_transfer(transfer) == 0);
    }

    auto allocateAndSubmitIn() {
        struct libusb_transfer *transfer;

        transfer = libusb_alloc_transfer(0);
        assert(transfer);
        transfer->dev_handle = transport->usb_devh;
        transfer->flags      = 0;
        transfer->type       = LIBUSB_TRANSFER_TYPE_BULK;
        transfer->endpoint   = transport->usb_ep.in;
        transfer->timeout    = 0;
        transfer->user_data  = this;
        transfer->length     = 16 * 256;

        transfer->buffer = libusb_dev_mem_alloc(transfer->dev_handle, transfer->length);
        assert(transfer->buffer);
        transfer->callback = usbInTransferCallback;

        assert(libusb_submit_transfer(transfer) == 0);
    }

    auto allocateAndSubmit() -> void {
        allocateAndSubmitIrq();
        for (auto i = 0; i < 4; ++i) {
            allocateAndSubmitIn();
        }
    }

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

    enum struct Reset {
        Cold,
        Warm,
    };

    static auto toString(Reset reset) -> std::string_view {
        using namespace std::literals;
        switch (reset) {
            case Reset::Cold:
                return "Cold"sv;
            case Reset::Warm:
                return "Warm"sv;
        }

        return "Unreachable"sv;
    }

    std::unique_ptr<osmo_st2_transport>   transport;
    std::unique_ptr<osmo_st2_slot>        slot;
    std::unique_ptr<osmo_st2_cardem_inst> instance;
    uint32_t                              lastStatusFlags = 0;
    bool                                  hasCard         = false;
    bool                                  moved           = false;
};

#endif
