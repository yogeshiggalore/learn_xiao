#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <errno.h>

/* Audio config */
#define SAMPLE_RATE_HZ      16000
#define PCM_WIDTH_BITS      16
#define CHANNELS            1

#define BLOCK_MS            20
#define SAMPLES_PER_BLOCK   ((SAMPLE_RATE_HZ * BLOCK_MS) / 1000)   /* 320 */
#define BYTES_PER_SAMPLE    (PCM_WIDTH_BITS / 8)                   /* 2   */
#define BLOCK_SIZE_BYTES    (SAMPLES_PER_BLOCK * BYTES_PER_SAMPLE * CHANNELS)
#define BLOCK_COUNT         64

K_MEM_SLAB_DEFINE(audio_slab, BLOCK_SIZE_BYTES, BLOCK_COUNT, 4);

/* ---------- TLV Types ---------- */
#define TLV_T_PCM_BLOCK     0x01   /* raw PCM bytes */
#define TLV_T_TIMESTAMP_MS  0x02   /* uint32 little-endian */
#define TLV_T_SYNC          0x7F   /* ASCII "SYNC" */

struct audio_item {
    void    *buf;
    uint16_t size;
};

#define AUDIO_Q_LEN  16
K_MSGQ_DEFINE(audio_q, sizeof(struct audio_item), AUDIO_Q_LEN, 4);

/* Threads */
#define CAPTURE_STACK_SIZE 2048
#define TX_STACK_SIZE      2048
#define CAPTURE_PRIO  3
#define TX_PRIO       5

K_THREAD_STACK_DEFINE(capture_stack, CAPTURE_STACK_SIZE);
K_THREAD_STACK_DEFINE(tx_stack, TX_STACK_SIZE);

static struct k_thread capture_thread_data;
static struct k_thread tx_thread_data;

static const struct device *uart_dev;

/* --- UART helpers (polling; simple + works everywhere) --- */
static void uart_send_bytes_poll(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uart_poll_out(uart_dev, data[i]);
    }
}

/* TLV: T(1) + L(2 LE) + V(L) */
static void uart_send_tlv(uint8_t type, const void *val, uint16_t len)
{
    uint8_t hdr[3];
    hdr[0] = type;
    hdr[1] = (uint8_t)(len & 0xFF);
    hdr[2] = (uint8_t)((len >> 8) & 0xFF);

    uart_send_bytes_poll(hdr, sizeof(hdr));
    if (len > 0 && val != NULL) {
        uart_send_bytes_poll((const uint8_t *)val, len);
    }
}

static void uart_send_u32_le(uint8_t type, uint32_t v)
{
    uint8_t b[4];
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)((v >> 8) & 0xFF);
    b[2] = (uint8_t)((v >> 16) & 0xFF);
    b[3] = (uint8_t)((v >> 24) & 0xFF);
    uart_send_tlv(type, b, 4);
}

/* ---------- Thread A: capture ---------- */
static void capture_thread(void *p1, void *p2, void *p3)
{
    const struct device *dmic = (const struct device *)p1;

    while (1) {
        void *buffer = NULL;
        size_t size = 0;

        /* Blocking read is OK: only this thread blocks */
        int ret = dmic_read(dmic, 0, &buffer, &size, 2000);
        if (ret) {
            printk("dmic_read err: %d\n", ret);
            continue;
        }

        struct audio_item item = {
            .buf  = buffer,
            .size = (uint16_t)size
        };

        /* If TX can't keep up, drop block safely */
        ret = k_msgq_put(&audio_q, &item, K_NO_WAIT);
        if (ret) {
            k_mem_slab_free(&audio_slab, buffer);
            /* optional: track drops */
            // printk("audio_q full, dropped\n");
        }
    }
}

/* ---------- Thread B: TX ---------- */
static void tx_thread(void *p1, void *p2, void *p3)
{
    (void)p1; (void)p2; (void)p3;

    /* Optional: send a sync marker once at start */
    const char sync_str[] = "SYNC";
    uart_send_tlv(TLV_T_SYNC, sync_str, (uint16_t)sizeof(sync_str) - 1);

    while (1) {
        struct audio_item item;
        k_msgq_get(&audio_q, &item, K_FOREVER);

        /* Optional timestamp before each block (ms since boot) */
        uint32_t ms = (uint32_t)k_uptime_get_32();
        uart_send_u32_le(TLV_T_TIMESTAMP_MS, ms);

        /* Send PCM block as TLV */
        uart_send_tlv(TLV_T_PCM_BLOCK, item.buf, item.size);

        /* Free buffer after sending */
        k_mem_slab_free(&audio_slab, item.buf);
    }
}

int main(void)
{
    const struct device *dmic = DEVICE_DT_GET(DT_ALIAS(appmic));
    if (!device_is_ready(dmic)) {
        printk("DMIC device not ready\n");
        return 0;
    }

    uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    if (!device_is_ready(uart_dev)) {
        printk("UART console not ready\n");
        return 0;
    }

    printk("DMIC + UART ready. %d Hz, %d-bit, %dch, block=%d bytes\n",
           SAMPLE_RATE_HZ, PCM_WIDTH_BITS, CHANNELS, BLOCK_SIZE_BYTES);

    static struct pcm_stream_cfg stream_cfg[1];
    struct dmic_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    memset(stream_cfg, 0, sizeof(stream_cfg));

    cfg.io.min_pdm_clk_freq = 1000000;
    cfg.io.max_pdm_clk_freq = 2500000;
    cfg.io.min_pdm_clk_dc   = 40;
    cfg.io.max_pdm_clk_dc   = 60;

    cfg.channel.req_num_streams = 1;
    cfg.channel.req_num_chan = CHANNELS;
    cfg.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
    cfg.channel.req_chan_map_hi = 0;

    cfg.streams = stream_cfg;
    cfg.streams[0].pcm_rate   = SAMPLE_RATE_HZ;
    cfg.streams[0].pcm_width  = PCM_WIDTH_BITS;
    cfg.streams[0].block_size = BLOCK_SIZE_BYTES;
    cfg.streams[0].mem_slab   = &audio_slab;

    int ret = dmic_configure(dmic, &cfg);
    if (ret) {
        printk("dmic_configure failed: %d\n", ret);
        return 0;
    }

    ret = dmic_trigger(dmic, DMIC_TRIGGER_START);
    if (ret) {
        printk("dmic start failed: %d\n", ret);
        return 0;
    }

    k_thread_create(&capture_thread_data, capture_stack, CAPTURE_STACK_SIZE,
                    capture_thread, (void *)dmic, NULL, NULL,
                    CAPTURE_PRIO, 0, K_NO_WAIT);

    k_thread_create(&tx_thread_data, tx_stack, TX_STACK_SIZE,
                    tx_thread, NULL, NULL, NULL,
                    TX_PRIO, 0, K_NO_WAIT);

    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
