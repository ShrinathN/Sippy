#include "ws2812b.h"

// private functions
static const rmt_symbol_word_t ws2812_zero = {
    .level0 = 0,
    .duration0 = 0.4 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T0H=0.3us
    .level1 = 1,
    .duration1 = 0.85 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T0L=0.9us
};

static const rmt_symbol_word_t ws2812_one = {
    .level0 = 0,
    .duration0 = 0.8 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T1H=0.9us
    .level1 = 1,
    .duration1 = 0.45 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T1L=0.3us
};

// reset defaults to 50uS
static const rmt_symbol_word_t ws2812_reset = {
    .level0 = 1,
    .duration0 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
    .level1 = 1,
    .duration1 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
};
static rmt_channel_handle_t led_chan = NULL;
static rmt_tx_channel_config_t tx_chan_config = ZEROS;
static rmt_encoder_handle_t simple_encoder = NULL;

static size_t encoder_callback(const void *data, size_t data_size,
                               size_t symbols_written, size_t symbols_free,
                               rmt_symbol_word_t *symbols, bool *done, void *arg)
{
    // We need a minimum of 8 symbol spaces to encode a byte. We only
    // need one to encode a reset, but it's simpler to simply demand that
    // there are 8 symbol spaces free to write anything.
    if (symbols_free < 8)
    {
        return 0;
    }

    if (symbols_written == 0)
    {
        symbols[0] = ws2812_reset;
        return 1; // we only wrote one symbol
    }
    if (symbols_written > 0)
    {
        // We can calculate where in the data we are from the symbol pos.
        // Alternatively, we could use some counter referenced by the arg
        // parameter to keep track of this.
        size_t data_pos = symbols_written / 8;
        uint8_t *data_bytes = (uint8_t *)data;
        if (data_pos < data_size)
        {
            // Encode a byte
            size_t symbol_pos = 0;
            for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1)
            {
                if (data_bytes[data_pos] & bitmask)
                {
                    symbols[symbol_pos++] = ws2812_one;
                }
                else
                {
                    symbols[symbol_pos++] = ws2812_zero;
                }
            }
            // We're done; we should have written 8 symbols.
            if (data_pos == data_size - 1)
                *done = 1; // Indicate end of the transaction.
            return symbol_pos;
        }
    }
    return -1;
}

void ws2812b_init()
{
    const char *TAG = __func__;

    tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT; // select source clock
    tx_chan_config.gpio_num = RMT_LED_STRIP_GPIO_NUM;
    tx_chan_config.mem_block_symbols = 64; // increase the block size can make the LED less flickering
    tx_chan_config.resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ;
    tx_chan_config.trans_queue_depth = 4; // set the number of transactions that can be pending in the background

    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));
    ESP_LOGI(TAG, "Create simple callback-based encoder");
    const rmt_simple_encoder_config_t simple_encoder_cfg = {
        .callback = encoder_callback
        // Note we don't set min_chunk_size here as the default of 64 is good enough.
    };
    ESP_ERROR_CHECK(rmt_new_simple_encoder(&simple_encoder_cfg, &simple_encoder));
    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));
}

void ws2812b_write(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t leds[] = {g, r, b};
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(rmt_transmit(led_chan, simple_encoder, leds, sizeof(leds), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
}
