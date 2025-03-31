#ifndef SIMTRACE_HH
#define SIMTRACE_HH

#include <libusb.h>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <source_location>
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
    constexpr SimTrace2() {
        transport = std::make_unique<osmo_st2_transport>();
        slot      = std::make_unique<osmo_st2_slot>(transport.get(), 0);
        instance  = std::make_unique<osmo_st2_cardem_inst>(slot.get(), &osim_uicc_sim_cic_profile);
    }

    constexpr auto connect() {
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

        allocateAndSubmit();

        osmo_st2_cardem_request_config(instance.get(), CEMU_FEAT_F_STATUS_IRQ);
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
    }

    constexpr ~SimTrace2() {
        libusb_release_interface(transport->usb_devh, 0);

        if (transport->usb_devh) {
            libusb_close(transport->usb_devh);
            transport->usb_devh = nullptr;
        }

        osmo_libusb_exit(nullptr);
    }

private:
    static void updateStatusFlags(SimTrace2 &trace, uint32_t flags) {
        struct osim_card_hdl *card  = trace.instance->chan->card;
        int                   reset = noReset;

        if ((flags & CEMU_STATUS_F_VCC_PRESENT) && (flags & CEMU_STATUS_F_CLK_ACTIVE) &&
            !(flags & CEMU_STATUS_F_RESET_ACTIVE)) {
            if (trace.lastStatusFlags & CEMU_STATUS_F_RESET_ACTIVE) {
                if (trace.lastStatusFlags & CEMU_STATUS_F_VCC_PRESENT)
                    reset = warmReset;
                else
                    reset = coldReset;
            } else if (!(trace.lastStatusFlags & CEMU_STATUS_F_VCC_PRESENT)) {
                /* power-up has just happened, perform cold reset */
                reset = coldReset;
            }
        } else if (flags == CEMU_STATUS_F_VCC_PRESENT && !(trace.lastStatusFlags & CEMU_STATUS_F_VCC_PRESENT)) {
            reset = coldReset;
        }

        if (reset) {
            std::println("{} Resetting card in reader...", reset == coldReset ? "Cold" : "Warm");
            osim_card_reset(card, reset == coldReset ? true : false);
            osmo_st2_gsmtap_send_apdu(GSMTAP_SIM_ATR, card->atr, card->atr_len);
        }

        trace.lastStatusFlags = flags;
    }

    static auto cemuStatusFlagsToString(char *out, unsigned int out_len, uint32_t flags) {
        snprintf(out, out_len, "%s%s%s%s%s", flags & CEMU_STATUS_F_RESET_ACTIVE ? "RESET " : "",
                 flags & CEMU_STATUS_F_VCC_PRESENT ? "VCC " : "", flags & CEMU_STATUS_F_CLK_ACTIVE ? "CLK " : "",
                 flags & CEMU_STATUS_F_CARD_INSERT ? "CARD_PRES " : "",
                 flags & CEMU_STATUS_F_RCEMU_ACTIVE ? "RCEMU " : "");
    }

    static auto processIrqStatus(SimTrace2 &trace, const uint8_t *buffer, int /* length */) {
        const struct cardemu_usb_msg_status *status = (struct cardemu_usb_msg_status *)buffer;
        char                                 flagsBuffer[80];

        cemuStatusFlagsToString(flagsBuffer, sizeof(flagsBuffer), status->flags);

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

    static const char *cemuDataFlagsToString(uint32_t flags) {
        static char out[64];
        snprintf(out, sizeof(out), "%s%s%s%s", flags & CEMU_DATA_F_TPDU_HDR ? "HDR " : "",
                 flags & CEMU_DATA_F_FINAL ? "FINAL " : "", flags & CEMU_DATA_F_PB_AND_TX ? "PB_AND_TX " : "",
                 flags & CEMU_DATA_F_PB_AND_RX ? "PB_AND_RX" : "");
        return out;
    }

    static int processDoStatus(SimTrace2 &trace, uint8_t *buffer, int /* length */) {
        struct cardemu_usb_msg_status *status = (struct cardemu_usb_msg_status *)buffer;
        char                           flagsBuffer[80];

        cemuStatusFlagsToString(flagsBuffer, sizeof(flagsBuffer), status->flags);

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
    static constexpr auto                                 noReset       = 0;
    static constexpr auto                                 coldReset     = 1;
    static constexpr auto                                 warmReset     = 2;

    std::unique_ptr<osmo_st2_cardem_inst> instance;
    std::unique_ptr<osmo_st2_slot>        slot;
    std::unique_ptr<osmo_st2_transport>   transport;
    uint32_t                              lastStatusFlags = 0;
    bool                                  hasCard         = false;
};

#endif
